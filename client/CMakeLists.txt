cmake_minimum_required(VERSION 3.21)
project(MyProjectClientSubproject LANGUAGES CXX)

find_package(wxWidgets CONFIG REQUIRED)

add_executable(client_app
    src/main.cpp
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


# --- Application Installation and Bundling ---

if(APPLE)
    # This is all that is needed for a statically linked app.
    # It creates the .app structure, and the executable inside is already
    # self-contained because of static linking. No fixup is required.
    set_target_properties(client_app PROPERTIES MACOSX_BUNDLE TRUE)
    install(TARGETS client_app BUNDLE DESTINATION . COMPONENT client)
else()
    install(TARGETS client_app DESTINATION . COMPONENT client)
endif()

if(WIN32)
    # The Windows fixup is needed because vcpkg defaults to
    # dynamic linking on Windows.
    install(CODE "
        include(BundleUtilities)
        fixup_bundle(\"\${CMAKE_INSTALL_PREFIX}/client_app.exe\" \"\" \"$<TARGET_FILE_DIR:client_app>\")
    " COMPONENT client)
endif()
