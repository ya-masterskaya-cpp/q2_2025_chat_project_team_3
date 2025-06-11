
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO stef/libopaque
    REF v1.0.1
    SHA512 ffdf18cef4560547f9cf095aa85c43f9d7a91d2be4ebd0116512d505d556cf7913b6244882ab60de46a24a7dcad452107dc1c601f32f79a0fecff49be8f3a2fd
    HEAD_REF master
)

# Replace the original build system with our cross-platform CMake file.
file(COPY "${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt" DESTINATION "${SOURCE_PATH}")

vcpkg_configure_cmake(
    SOURCE_PATH ${SOURCE_PATH}
)

vcpkg_install_cmake()

# Handle the pkg-config file
vcpkg_fixup_pkgconfig()

# Clean up redundant debug directories
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")

# Install copyright file
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
