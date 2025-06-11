cmake_minimum_required(VERSION 3.12)
project(opaque C)

# Find ALL direct dependencies
find_package(unofficial-sodium CONFIG REQUIRED)
find_package(liboprf CONFIG REQUIRED)

add_library(opaque STATIC src/opaque.c src/common.c)
add_library(opaque::opaque ALIAS opaque) # Add an alias for modern CMake usage

target_include_directories(opaque PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
)

# Link dependencies as PUBLIC so downstream projects know about them.
# oprf::liboprf already brings in sodium transitively
target_link_libraries(opaque PUBLIC oprf::liboprf)

install(TARGETS opaque EXPORT opaque-targets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES src/opaque.h DESTINATION include)

# Install the CMake configuration files
install(EXPORT opaque-targets
    FILE opaque-config.cmake
    NAMESPACE opaque::
    DESTINATION share/opaque
)
