# ── Build stage ───────────────────────────────────────────────────────────────
FROM debian:bookworm-slim AS builder

# Install only the packages required to compile the C backend.
# libcurl4-openssl-dev supplies libcurl + its headers.
# ca-certificates is needed at runtime for HTTPS calls to the GitHub API.
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        make \
        libcurl4-openssl-dev \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build

# Copy the full project. .dockerignore keeps this layer lean by excluding
# frontend/node_modules, frontend/dist, .git, and pre-built object files.
COPY . .

# Compile both binaries. The Makefile vendors cJSON, so no system libcjson
# package is required. pkg-config will supply libcurl flags on Debian.
RUN make clean && make

# ── Runtime stage ─────────────────────────────────────────────────────────────
FROM debian:bookworm-slim

# libcurl4 (shared library) and ca-certificates are needed at runtime.
RUN apt-get update && apt-get install -y --no-install-recommends \
        libcurl4 \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy only the compiled HTTP server binary from the build stage.
# The MCP server (repoonboard-mcp) is not included in the public web service.
COPY --from=builder /build/backend/bin/repoonboard ./backend/bin/repoonboard

# PORT defaults to 10000 for cloud deployment platforms (e.g. Render).
# Override with -e PORT=<n> or an environment variable at runtime.
ENV PORT=10000

EXPOSE 10000

CMD PORT="${PORT:-10000}" ./backend/bin/repoonboard
