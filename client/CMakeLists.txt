cmake_minimum_required(VERSION 3.18)
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
)

target_include_directories(client_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(client_app PRIVATE
    common_lib
    wx::core
    wx::base
)
