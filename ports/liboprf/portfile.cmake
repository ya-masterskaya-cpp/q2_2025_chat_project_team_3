vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO stef/liboprf
    REF v0.8.0
    SHA512 19e8036a13267de319b7603702d69ff0bda1b84f5aa8f8fcd37b2712d8ff3111ad9d99b695bce1174fbb800864e402e106262a595532156dd774a9b99ed8a2c8
    HEAD_REF master
)

# Replace the original build system with our cross-platform CMake file.
file(COPY "${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt" "${CMAKE_CURRENT_LIST_DIR}/config.cmake.in" DESTINATION "${SOURCE_PATH}")

vcpkg_configure_cmake(
    SOURCE_PATH ${SOURCE_PATH}
)

vcpkg_install_cmake()

# This command will automatically fix up the config files we just created.
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/liboprf)

# Handle the pkg-config file
vcpkg_fixup_pkgconfig()

# Clean up redundant debug directories
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")

# Install copyright file
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
