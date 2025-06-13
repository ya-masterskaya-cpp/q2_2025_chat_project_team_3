cmake_minimum_required(VERSION 3.21)
project(MyProjectV2 LANGUAGES CXX VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_SERVER "Build the server application" ON)
option(BUILD_CLIENT "Build the client application" ON)

# Force-disable server on macOS
if(APPLE)
    if(BUILD_SERVER)
        message(WARNING "Building the server on macOS is not supported. Disabling BUILD_SERVER.")
    endif()
    set(BUILD_SERVER OFF)
endif()

add_subdirectory(common)

if(BUILD_SERVER)
    add_subdirectory(server)
endif()

if(BUILD_CLIENT)
    add_subdirectory(client)
endif()

set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")

# Set the generator and filename based on the platform
if(WIN32)
    set(CPACK_GENERATOR "ZIP")
    set(CPACK_PACKAGE_FILENAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-win-x64")
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_PACKAGE_FILE_EXTENSION "dmg")
    set(CPACK_PACKAGE_FILENAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-osx-x64")
    # Optional: Customize DMG appearance (background, icon size, etc.)
    # set(CPACK_DMG_BACKGROUND_IMAGE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/Resources/dmg_background.tif")
    # set(CPACK_DMG_APPLICATIONS_DISABLED TRUE) # Don't show Applications shortcut
    # set(CPACK_DMG_HIDE_MICROSOFT_APPLICATIONS TRUE)
    # set(CPACK_DMG_HIDE_MAKEMUSIC_APPLICATIONS TRUE)
    # set(CPACK_DMG_DEFAULT_ICON_SIZE 128)
    # set(CPACK_DMG_VOLUME_NAME "${CPACK_PACKAGE_NAME} ${CPACK_PACKAGE_VERSION}") # Name of the mounted volume
else() # Assume Linux/Unix if not WIN32 or APPLE
    set(CPACK_GENERATOR "TGZ")
    set(CPACK_PACKAGE_FILENAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-linux-x64")
endif()

# This tells CPack to put all installed files into a root folder inside the archive.
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_NAME}")

# Include the CPack module AFTER all CPACK_* variables are set.
# This automatically creates the 'package' build target.
include(CPack)
