#!/bin/bash
set -xe

echo "Starting dev container initialization script..."

if [ -z "${VCPKG_ROOT}" ]; then
  echo "Error: VCPKG_ROOT is not set. Cannot copy triplets."
  exit 1
fi
echo "VCPKG_ROOT is set to: ${VCPKG_ROOT}"

TRIPLET_DIR="${VCPKG_ROOT}/triplets/community"
echo "Ensuring community triplet directory exists: ${TRIPLET_DIR}"
mkdir -p "${TRIPLET_DIR}"

echo "Copying x64-linux-release-sanitized.cmake triplet..."
cp "/workspace/.devcontainer/x64-linux-release-sanitized.cmake" "${TRIPLET_DIR}/x64-linux-release-sanitized.cmake"
echo "Copying x64-linux-release-tsan.cmake triplet..."
cp "/workspace/.devcontainer/x64-linux-release-tsan.cmake" "${TRIPLET_DIR}/x64-linux-release-tsan.cmake"

echo "Custom triplets copied."

git config --global --add safe.directory /workspace
sudo DEBIAN_FRONTEND=noninteractive apt-get update -y
sudo DEBIAN_FRONTEND=noninteractive apt-get install -y bison flex autoconf ninja-build
echo "Dev container initialization script finished."
