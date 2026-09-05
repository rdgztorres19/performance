#!/usr/bin/env bash
#
# Launcher for the performance demos.
#
# Reads .env, works out which topics are enabled, and runs only those.
# Everything is disabled by default, so a fresh checkout starts nothing.
#
#   ./run.sh list                 show every topic and whether it is enabled
#   ./run.sh up                   build and run every enabled topic
#   ./run.sh up random-io         run one topic, ignoring its .env switch
#   ./run.sh logs                 follow logs of running demos
#   ./run.sh down                 stop and remove containers
#   ./run.sh build                build the two images without running
#
set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")"

ENV_FILE=".env"
COMPOSE_DIR="docker/compose"
PROJECT="perf-demos"

if [[ ! -f "$ENV_FILE" ]]; then
  echo "No .env found. Creating one from .env.example (everything disabled)."
  cp .env.example "$ENV_FILE"
fi

# Load .env without executing anything unexpected: only KEY=VALUE lines.
while IFS='=' read -r key value; do
  [[ "$key" =~ ^[A-Z_][A-Z0-9_]*$ ]] || continue
  value="${value%%#*}"                       # strip trailing comment
  value="$(echo -n "$value" | tr -d '[:space:]')"
  export "$key=$value"
done < <(grep -E '^[A-Z_][A-Z0-9_]*=' "$ENV_FILE" || true)

RUN_PYTHON="${RUN_PYTHON:-true}"
RUN_NODE="${RUN_NODE:-true}"
RUN_C="${RUN_C:-true}"
RUN_INCORRECT="${RUN_INCORRECT:-true}"
RUN_CORRECT="${RUN_CORRECT:-true}"

is_true() {
  local v
  v="$(printf '%s' "${1:-}" | tr '[:upper:]' '[:lower:]')"
  [[ "$v" == "true" || "$v" == "1" || "$v" == "yes" ]]
}

# Map a topic directory name to its ENABLE_ variable.
env_var_for() {
  local dir="$1"
  echo "ENABLE_$(echo "$dir" | tr 'a-z-' 'A-Z_')"
}

# Collect -f flags and --profile flags for the topics that should run.
COMPOSE_FILES=()
PROFILES=()
ENABLED_TOPICS=()

collect() {
  local only="${1:-}"

  for file in "$COMPOSE_DIR"/*.yml; do
    local dir base
    base="$(basename "$file" .yml)"          # e.g. 09-random-io
    dir="$base"
    local var; var="$(env_var_for "$dir")"
    local enabled="${!var:-false}"

    if [[ -n "$only" ]]; then
      # An explicit topic argument overrides the .env switch.
      [[ "$dir" == *"$only"* ]] || continue
      enabled="true"
    fi

    is_true "$enabled" || continue

    ENABLED_TOPICS+=("$dir")
    COMPOSE_FILES+=(-f "$file")

    # Turn on only the variants the global switches allow.
    for lang in PY JS C; do
      case "$lang" in
        PY) is_true "$RUN_PYTHON" || continue ;;
        JS) is_true "$RUN_NODE"   || continue ;;
        C)  is_true "$RUN_C"      || continue ;;
      esac

      for variant in INCORRECT CORRECT; do
        if [[ "$variant" == "INCORRECT" ]]; then
          is_true "$RUN_INCORRECT" || continue
        else
          is_true "$RUN_CORRECT" || continue
        fi
        PROFILES+=(--profile "${var}_${lang}_${variant}")
      done
    done
  done
}

# Build the two runner images once. Every service reuses them, so building
# per-service would race on the shared image name.
build_images() {
  local quiet="${1:-}"
  local out=/dev/stdout
  [[ "$quiet" == "quiet" ]] && out=/dev/null

  echo "Building runner images (python, node, c)..."
  docker build -f docker/Dockerfile.python -t perf-demo-python:local . > "$out" 2>&1
  docker build -f docker/Dockerfile.node   -t perf-demo-node:local   . > "$out" 2>&1
  # The C image compiles every .c demo at build time, so this one is slower.
  docker build -f docker/Dockerfile.c      -t perf-demo-c:local      . > "$out" 2>&1
  echo "Images ready."
  echo
}

cmd_list() {
  printf "%-34s %-10s %s\n" "TOPIC" "ENABLED" "LANGUAGES"
  printf "%-34s %-10s %s\n" "----------------------------------" "----------" "---------"
  local total=0 on=0
  for file in "$COMPOSE_DIR"/*.yml; do
    local dir; dir="$(basename "$file" .yml)"
    local var; var="$(env_var_for "$dir")"
    local enabled="${!var:-false}"
    local langs=""
    grep -q 'perf-demo-python:local' "$file" && langs="python"
    grep -q 'perf-demo-node:local' "$file" && langs="${langs:+$langs, }node"
    grep -q 'perf-demo-c:local' "$file" && langs="${langs:+$langs, }C"
    total=$((total+1))
    if is_true "$enabled"; then
      on=$((on+1))
      printf "\033[32m%-34s %-10s %s\033[0m\n" "$dir" "yes" "$langs"
    else
      printf "%-34s %-10s %s\n" "$dir" "no" "$langs"
    fi
  done
  echo
  echo "$on of $total topics enabled. Edit .env to change this."
}

cmd_up() {
  collect "${1:-}"
  if [[ ${#ENABLED_TOPICS[@]} -eq 0 ]]; then
    echo "No topics enabled."
    echo "Set a topic to true in .env, or run: ./run.sh up <topic>"
    echo "See what is available with: ./run.sh list"
    exit 0
  fi

  echo "Running ${#ENABLED_TOPICS[@]} topic(s): ${ENABLED_TOPICS[*]}"
  echo "Duration per demo: ${DURATION_SEC:-20}s"
  echo

  build_images quiet

  # Helper servers (the network topics) run forever by design, so we cannot
  # simply wait for every container to exit. Start everything detached, wait
  # only on the demo containers, stream their logs, then tear the stack down.
  docker compose -p "$PROJECT" "${COMPOSE_FILES[@]}" "${PROFILES[@]}" \
    up --detach --remove-orphans

  # Names of the demo containers, excluding the helper servers.
  local demos=()
  while read -r name; do
    [[ -n "$name" ]] && demos+=("$name")
  done < <(docker ps -a \
             --filter "label=com.docker.compose.project=$PROJECT" \
             --format '{{.Names}}' | grep -v -- '-server$' || true)

  # Follow the logs until the demos finish.
  docker compose -p "$PROJECT" "${COMPOSE_FILES[@]}" "${PROFILES[@]}" \
    logs --follow --no-log-prefix=false &
  local logs_pid=$!

  local failed=0
  for c in "${demos[@]}"; do
    local code
    code="$(docker wait "$c" 2>/dev/null || echo 1)"
    [[ "$code" != "0" ]] && { echo "Container $c exited with code $code"; failed=1; }
  done

  # Give the log stream a moment to flush the final lines, then stop it.
  sleep 1
  kill "$logs_pid" 2>/dev/null || true
  wait "$logs_pid" 2>/dev/null || true

  echo
  echo "All demos finished. Stopping helper containers..."
  docker compose -p "$PROJECT" "${COMPOSE_FILES[@]}" "${PROFILES[@]}" \
    down --remove-orphans >/dev/null 2>&1 || true

  return $failed
}

cmd_build() {
  collect "${1:-}"
  if [[ ${#ENABLED_TOPICS[@]} -eq 0 ]]; then
    echo "No topics enabled; nothing to build."
    exit 0
  fi
  build_images
}

cmd_down() {
  collect ""
  # Always tear down by project name so nothing is left behind.
  docker compose -p "$PROJECT" ${COMPOSE_FILES[@]+"${COMPOSE_FILES[@]}"} down --remove-orphans 2>/dev/null || \
    docker compose -p "$PROJECT" down --remove-orphans
}

cmd_logs() {
  collect "${1:-}"
  docker compose -p "$PROJECT" "${COMPOSE_FILES[@]}" "${PROFILES[@]}" logs -f
}

case "${1:-list}" in
  list)  cmd_list ;;
  up)    cmd_up "${2:-}" ;;
  build) cmd_build "${2:-}" ;;
  down)  cmd_down ;;
  logs)  cmd_logs "${2:-}" ;;
  *)
    sed -n '2,15p' "$0" | sed 's/^# \?//'
    exit 1
    ;;
esac
