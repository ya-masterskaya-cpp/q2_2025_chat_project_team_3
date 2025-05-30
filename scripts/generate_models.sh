#!/bin/bash

./build/linux-debug/vcpkg_installed/x64-linux/tools/drogon/drogon_ctl create model src/models "$@"

# Loop over all .cc files in ./src/models/
for file in ./src/models/*.cc; do
  # Get the filename excluding .cc
  name=$(basename "$file" .cc)

  # Replace the include string
  sed -i "s|#include \"${name}.h\"|#include <models/${name}.h>|g" "$file"
done

# Move all .h files from ./src/models/ to ./include/models/ with overwrite
mkdir -p ./include/models/
mv -f ./src/models/*.h ./include/models/
