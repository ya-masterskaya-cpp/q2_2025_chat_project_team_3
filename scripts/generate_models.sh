#!/bin/bash

./build/linux-debug/vcpkg_installed/x64-linux/tools/drogon/drogon_ctl create model server/src/models "$@"

# Loop over all .cc files in .server/src/models/
for file in .server/src/models/*.cc; do
  # Get the filename excluding .cc
  name=$(basename "$file" .cc)

  # Replace the include string
  sed -i "s|#include \"${name}.h\"|#include <server/models/${name}.h>|g" "$file"
done

# Move all .h files from .server/src/models/ to .server/include/server/models/ with overwrite
mkdir -p .server/include/server/models/
mv -f .server/src/models/*.h .server/include/server/models/
