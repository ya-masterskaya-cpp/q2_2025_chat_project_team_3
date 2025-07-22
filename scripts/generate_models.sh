#!/bin/bash

./build/vcpkg_installed/x64-linux/tools/drogon/drogon_ctl create model server/src/models "$@"

# Loop over all .h files in ./server/src/models/
for header in ./server/src/models/*.h; do
  # Get the basename, e.g., "Rooms.h"
  header_file=$(basename "$header")
  # Get the stem, e.g., "Rooms"
  header_stem="${header_file%.h}"

  # For all .cc files, replace #include "HeaderFile.h" with #include <server/models/HeaderFile.h>
  for ccfile in ./server/src/models/*.cc; do
    sed -i "s|#include \"${header_file}\"|#include <server/models/${header_file}>|g" "$ccfile"
  done
done

# Move all .h files from .server/src/models/ to .server/include/server/models/ with overwrite
mkdir -p ./server/include/server/models/
mv -f ./server/src/models/*.h ./server/include/server/models/
