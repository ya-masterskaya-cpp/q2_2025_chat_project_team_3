cmake_minimum_required(VERSION 3.21)
project(SlightlyPrettyChat LANGUAGES CXX VERSION 0.0.6)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_SERVER "Build the server application" ON)
option(BUILD_CLIENT "Build the client application" ON)
option(TREAT_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)

set(COMMON_CXX_WARNING_FLAGS "")
set(ERROR_CXX_FLAG "")

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    list(APPEND COMMON_CXX_WARNING_FLAGS "-Wall" "-Wextra" "-Wpedantic" "-Wformat=2")
    set(ERROR_CXX_FLAG "-Werror")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    list(APPEND COMMON_CXX_WARNING_FLAGS "/W4" "/permissive-")
    set(ERROR_CXX_FLAG "/WX")
endif()

if(TREAT_WARNINGS_AS_ERRORS)
    list(APPEND COMMON_CXX_WARNING_FLAGS "${ERROR_CXX_FLAG}")
endif()

foreach(FLAG ${COMMON_CXX_WARNING_FLAGS})
    string(APPEND CMAKE_CXX_FLAGS_DEBUG " ${FLAG}")
    string(APPEND CMAKE_CXX_FLAGS_RELEASE " ${FLAG}")
endforeach()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    add_compile_options("$<$<CONFIG:Debug>:-fsanitize=address,undefined;-fno-omit-frame-pointer>")
    add_link_options("$<$<CONFIG:Debug>:-fsanitize=address,undefined>")
endif()

set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")

# Force-disable server on macOS
if(APPLE)
    if(BUILD_SERVER)
        message(WARNING "Building the server on macOS is not supported. Disabling BUILD_SERVER.")
    endif()
    set(BUILD_SERVER OFF)
endif()

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL TRUE)

add_subdirectory(common)

if(BUILD_SERVER)
    add_subdirectory(server)
    add_subdirectory(aggregator)
endif()

if(BUILD_CLIENT)
    add_subdirectory(client)
endif()

# Set the generator and filename based on the platform
if(WIN32)
    set(CPACK_GENERATOR "ZIP")
    set(CPACK_PACKAGE_FILENAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-win-x64")
    set(CPACK_COMPONENTS_ALL client server aggregator)
    set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_PACKAGE_FILE_EXTENSION "dmg")
    set(CPACK_PACKAGE_FILENAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-osx-x64")
    set(CPACK_COMPONENTS_ALL client)
else() # Linux
    set(CPACK_GENERATOR "TGZ")
    set(CPACK_PACKAGE_FILENAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-linux-x64")
    set(CPACK_COMPONENTS_ALL server aggregator)
    set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
endif()

# This tells CPack to put all installed files into a root folder inside the archive.
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_NAME}")

message(STATUS "Platform: WIN32=${WIN32}, APPLE=${APPLE}, LINUX=${LINUX}")
message(STATUS "Final CPACK_COMPONENTS_ALL: ${CPACK_COMPONENTS_ALL}")
message(STATUS "CPACK_GENERATOR: ${CPACK_GENERATOR}")

# Include the CPack module AFTER all CPACK_* variables are set.
# This automatically creates the 'package' build target.
include(CPack)
