cmake_minimum_required(VERSION 3.12)
project(liboprf C)

# --- 1. Find Dependencies ---
find_package(unofficial-sodium CONFIG REQUIRED)

# --- 2. Build Internal Object Library ---
# This section is correct and does not need changes.
add_library(oprf-noiseXK-internal OBJECT
    src/noise_xk/src/Noise_XK.c
    src/noise_xk/src/XK.c
)
target_include_directories(oprf-noiseXK-internal PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/noise_xk/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/noise_xk/include/karmel>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/noise_xk/include/karmel/minimal>
)
target_link_libraries(oprf-noiseXK-internal PUBLIC unofficial-sodium::sodium)


# --- 3. Build The Main Library ---
add_library(liboprf
    src/oprf.c
    src/toprf.c
    src/dkg.c
    src/dkg-vss.c
    src/utils.c
    src/tp-dkg.c
    src/mpmult.c
    src/stp-dkg.c
    src/toprf-update.c
    $<TARGET_OBJECTS:oprf-noiseXK-internal>
)
add_library(oprf::liboprf ALIAS liboprf)
target_compile_definitions(liboprf PRIVATE HAVE_SODIUM_HKDF=1)

# --- 4. Define All Required Include Paths and Links ---

# Make libsodium a PUBLIC dependency so anyone using liboprf knows about it.
target_link_libraries(liboprf PUBLIC unofficial-sodium::sodium)

# For the BUILD: Tell liboprf where to find ALL the headers it needs to compile itself.
target_include_directories(liboprf PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/noise_xk/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/noise_xk/include/karmel"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/noise_xk/include/karmel/minimal"
)

# For the USER: Tell projects using this library where to find ALL required headers.
# This makes sure that when a user includes <oprf/dkg.h>, the compiler can find "XK.h" and its dependencies.
target_include_directories(liboprf PUBLIC
    $<INSTALL_INTERFACE:include>
    $<INSTALL_INTERFACE:include/oprf/noiseXK/include>
    $<INSTALL_INTERFACE:include/oprf/noiseXK/include/karmel>
    $<INSTALL_INTERFACE:include/oprf/noiseXK/include/karmel/minimal>
)


# --- 5. Installation Rules ---
# These rules create the directory structure that the PUBLIC includes above depend on.
install(TARGETS liboprf
    EXPORT liboprf-targets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# Install the main public headers
install(FILES
    src/oprf.h
    src/toprf.h
    src/toprf-update.h
    src/dkg.h
    src/tp-dkg.h
    src/stp-dkg.h
    src/utils.h
    DESTINATION include/oprf
)

# Install the internal headers that the public headers depend on.
# The trailing slash on the source directory is important.
install(
    DIRECTORY src/noise_xk/include/
    DESTINATION include/oprf/noiseXK/include
)

# Install the CMake configuration files.
install(EXPORT liboprf-targets
    FILE liboprf-config.cmake
    NAMESPACE oprf::
    DESTINATION share/liboprf
)
