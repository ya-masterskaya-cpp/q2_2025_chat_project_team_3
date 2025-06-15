cmake_minimum_required(VERSION 3.21)
project(MyProjectServerSubproject LANGUAGES CXX)

add_executable(server_app
    src/main.cpp
    src/chat/ChatRoomManager.cpp
    src/chat/DrogonRoomService.cpp
    src/chat/HttpCtrl.cpp
    src/db/migrations.cpp
    src/models/Migrations.cc
    src/models/Users.cc
    src/models/Rooms.cc
    src/models/Messages.cc
)

target_include_directories(server_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/extern/utf8cpp/source
)

target_link_libraries(server_app PRIVATE
    common_lib
)

target_compile_definitions(server_app PRIVATE
    UTF_CPP_CPLUSPLUS=202302L
)

target_precompile_headers(server_app PRIVATE
    "${CMAKE_SOURCE_DIR}/common/include/pch.h"
)

add_custom_command(TARGET server_app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/config.json"
        "$<TARGET_FILE_DIR:server_app>/config.json"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/db/main.sql"
        "$<TARGET_FILE_DIR:server_app>/db/main.sql"
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/db/migrations"
        "$<TARGET_FILE_DIR:server_app>/db/migrations"
)

install(TARGETS server_app
    DESTINATION . COMPONENT server
)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/config.json
    DESTINATION . COMPONENT server
)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/db/
    DESTINATION ./db COMPONENT server
)

if(WIN32)
    install(CODE "
        include(BundleUtilities)
        fixup_bundle(\"\${CMAKE_INSTALL_PREFIX}/server_app.exe\" \"\" \"$<TARGET_FILE_DIR:server_app>\")
    " COMPONENT server)
endif()
