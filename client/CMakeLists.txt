cmake_minimum_required(VERSION 3.21)
project(SlightlyPrettyChatClient LANGUAGES CXX)

find_package(wxWidgets CONFIG REQUIRED)

add_executable(client_app
    src/main.cpp
    src/graphicsContextManager.cpp
    src/cachedColorText.cpp
    src/appConfig.cpp
    src/authPanel.cpp
    src/chatPanel.cpp
    src/roomsPanel.cpp
    src/mainWidget.cpp
    src/wsClient.cpp
    src/messageWidget.cpp
    src/userListPanel.cpp
    src/textUtil.cpp
    src/userNameWidget.cpp
    src/messageView.cpp
    src/passwordUtil.cpp
    src/initialPanel.cpp
    src/serversPanel.cpp
)

target_include_directories(client_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(client_app PRIVATE
    common_lib
    wx::core
    wx::base
)

target_precompile_headers(client_app PRIVATE
    "${CMAKE_SOURCE_DIR}/common/include/pch.h"
)

if(MSVC)
    target_link_options(client_app PRIVATE "/SUBSYSTEM:WINDOWS")
endif()

if(NOT APPLE)
    # This command is for local execution on Windows/Linux, not for macOS bundles.
    add_custom_command(
        TARGET client_app POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/fonts"
                "$<TARGET_FILE_DIR:client_app>/fonts"
        COMMENT "Copying fonts for local execution..."
    )
endif()

file(GLOB FONT_FILES "${CMAKE_CURRENT_SOURCE_DIR}/fonts/*.ttf")

# --- Application Installation and Bundling ---

if(APPLE)
    # On macOS, we bundle resources directly into the .app package.
    # First, add the font files as resource-type sources to the target.
    target_sources(client_app PRIVATE ${FONT_FILES})

    # Tell CMake to place these specific source files into the
    # "Contents/Resources/fonts" directory inside the .app bundle.
    set_source_files_properties(${FONT_FILES}
        PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/Fonts"
    )

    # This creates the .app structure and automatically includes the
    # resources we just configured.
    set_target_properties(client_app PROPERTIES
        MACOSX_BUNDLE TRUE

        # Sets CFBundleName from the top-level CPack variable
        MACOSX_BUNDLE_BUNDLE_NAME "${CPACK_PACKAGE_NAME}"

        # Sets CFBundleIdentifier. This must be a unique string.
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.yourname.slightlyprettychat"

        # Sets CFBundleVersion from the top-level CPack variable
        MACOSX_BUNDLE_BUNDLE_VERSION "${CPACK_PACKAGE_VERSION}"

        # Sets CFBundleShortVersionString from the top-level CPack variable
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${CPACK_PACKAGE_VERSION}"
    )
    install(TARGETS client_app BUNDLE DESTINATION . COMPONENT client)
else()
    # For Windows and Linux, we install the executable...
    install(TARGETS client_app DESTINATION . COMPONENT client)
    # ...and then install the fonts directory alongside it.
    install(
        DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/fonts/"
        DESTINATION "fonts"
        COMPONENT client
    )
endif()

if(WIN32)
    # The Windows fixup is needed because vcpkg defaults to
    # dynamic linking on Windows.
    install(CODE "
        include(BundleUtilities)
        fixup_bundle(\"\${CMAKE_INSTALL_PREFIX}/client_app.exe\" \"\" \"$<TARGET_FILE_DIR:client_app>\")
    " COMPONENT client)
endif()
