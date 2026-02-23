#!/usr/bin/env bash
# Build Linux .deb and .rpm packages inside an Ubuntu 24.04 Docker container.
# Produces in build-linux/:
#   jq-gunk-1.0.0-Linux.deb
#   jq-gunk-1.0.0-Linux.rpm
#
# Usage: ./docker-build-linux.sh

set -euo pipefail

IMAGE="jqgunk-linux-build"
SRC_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SRC_DIR/build-linux"

# Build the image if not already present
if ! docker image inspect "$IMAGE" &>/dev/null; then
    echo "==> Building Docker image..."
    docker build -t "$IMAGE" -f "$SRC_DIR/docker/Dockerfile.linux-build" "$SRC_DIR/docker"
fi

mkdir -p "$BUILD_DIR"

echo "==> Configuring, building and packaging inside container..."
docker run --rm \
    -v "$SRC_DIR":/src \
    -v "$BUILD_DIR":/build \
    -v "${HOME}/.ccache":/root/.ccache \
    -e CCACHE_DIR=/root/.ccache \
    "$IMAGE" \
    bash -c '
        set -e
        cd /src

        cmake -S . -B /build \
            -G Ninja \
            -DCMAKE_BUILD_TYPE=Release

        cmake --build /build

        cd /build
        cpack -G DEB
        cpack -G RPM

        echo ""
        echo "==> Packages written to build-linux/:"
        ls -lh /build/*.deb /build/*.rpm
    '
