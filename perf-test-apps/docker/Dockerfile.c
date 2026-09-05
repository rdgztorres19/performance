# C runner for the low-level performance demos.
#
# These topics (branch prediction, false sharing, cache layout, memory
# ordering) cannot be demonstrated in Python or JavaScript: the interpreter,
# the GIL, and the garbage collector dominate and hide the hardware effect.
# C compiles to machine code with no runtime in between.
FROM gcc:13-bookworm AS build

WORKDIR /src
COPY . /src

# Compile every demo ahead of time so the run container starts instantly.
# -O2 is essential: these demos are about what the CPU does with optimized
# code. BENCH_CONSUME in bench.h keeps -O2 from deleting the measured loops.
RUN set -eux; \
    mkdir -p /out; \
    for f in [0-9][0-9]-*/incorrect.c [0-9][0-9]-*/correct.c; do \
        [ -e "$f" ] || continue; \
        topic="$(dirname "$f")"; \
        variant="$(basename "$f" .c)"; \
        mkdir -p "/out/$topic"; \
        gcc -O2 -std=gnu11 -D_GNU_SOURCE -Wall -Wextra -pthread \
            "$f" -o "/out/$topic/$variant" -lm; \
    done; \
    find /out -type f | sort

# Runtime image: just the compiled binaries plus the tools to measure them.
FROM debian:bookworm-slim

# linux-perf gives perf stat for hardware counters, which is how these topics
# are actually proven. procps provides vmstat/pidstat for the scheduler demos.
RUN apt-get update \
    && apt-get install -y --no-install-recommends procps time \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=build /out /app

RUN mkdir -p /data
ENV TMPDIR=/data

CMD ["/bin/sh", "-c", "ls /app"]
