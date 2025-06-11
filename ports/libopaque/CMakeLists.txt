cmake_minimum_required(VERSION 3.12)
project(libopaque VERSION 1.0.1 LANGUAGES C)

include(CMakePackageConfigHelpers)

# Find ALL direct dependencies
find_package(liboprf CONFIG REQUIRED)

add_library(libopaque STATIC src/opaque.c src/common.c)

target_include_directories(libopaque PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
)

# Link dependencies as PUBLIC so downstream projects know about them.
target_link_libraries(libopaque PUBLIC liboprf)

install(TARGETS libopaque EXPORT libopaque-targets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES src/opaque.h DESTINATION include)

# Install the CMake configuration files
install(EXPORT libopaque-targets
    FILE libopaque-targets.cmake
    DESTINATION lib/cmake/libopaque
)

# Create and install the main config files using the template
configure_package_config_file(config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/libopaque-config.cmake"
    INSTALL_DESTINATION lib/cmake/libopaque
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/libopaque-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/libopaque-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/libopaque-config-version.cmake"
    DESTINATION lib/cmake/libopaque
)
