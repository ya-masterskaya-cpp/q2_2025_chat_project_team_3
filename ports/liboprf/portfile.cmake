vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO stef/liboprf
    REF v0.8.0
    SHA512 19e8036a13267de319b7603702d69ff0bda1b84f5aa8f8fcd37b2712d8ff3111ad9d99b695bce1174fbb800864e402e106262a595532156dd774a9b99ed8a2c8
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

# Install copyright file
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
