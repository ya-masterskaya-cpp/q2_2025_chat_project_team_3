cmake_minimum_required(VERSION 3.12)
project(liboprf C)

# --- 1. Find Dependencies ---
find_package(unofficial-sodium CONFIG REQUIRED)

# --- 2. Build Internal Vendored Code as an OBJECT Library ---
# This remains the best way to handle this. It compiles noise_xk's sources
# and links them directly into liboprf without creating an intermediate library.
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

# --- 4. Define Links and Includes ---

# PUBLIC: liboprf's public headers (oprf.h, toprf.h) include sodium.h,
# so any consumer of liboprf MUST also link to libsodium.
target_link_libraries(liboprf PUBLIC unofficial-sodium::sodium)

# PRIVATE: For building liboprf itself, it needs to find all its internal headers,
# including those from noise_xk. These paths are NOT exposed to the user.
target_include_directories(liboprf PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/noise_xk/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/noise_xk/include/karmel"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/noise_xk/include/karmel/minimal"
)

# PUBLIC: For the user, we only expose the main 'include' directory.
# This is clean and simple.
target_include_directories(liboprf PUBLIC
    $<INSTALL_INTERFACE:include>
)

# --- 5. Installation Rules ---
install(TARGETS liboprf
    EXPORT liboprf-targets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# Install the intended public headers of liboprf.
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

# Install the CMake configuration files.
install(EXPORT liboprf-targets
    FILE liboprf-config.cmake
    NAMESPACE oprf::
    DESTINATION share/liboprf
)
