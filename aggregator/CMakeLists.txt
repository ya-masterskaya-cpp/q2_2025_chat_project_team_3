cmake_minimum_required(VERSION 3.21)
project(MyProjectAggregatorSubproject LANGUAGES CXX)

add_executable(aggregator_app
    src/main.cpp
    src/DrogonServerRegistry.cpp
    src/ServerRegistry.cpp
)

target_include_directories(aggregator_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(aggregator_app PRIVATE
    common_lib
)

target_precompile_headers(aggregator_app PRIVATE
    "${CMAKE_SOURCE_DIR}/common/include/pch.h"
)

add_custom_command(TARGET aggregator_app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/config.json"
        "$<TARGET_FILE_DIR:aggregator_app>/config.json"
)

install(TARGETS aggregator_app
    DESTINATION . COMPONENT aggregator
)

if(WIN32)
    install(CODE "
        include(BundleUtilities)
        fixup_bundle(\"\${CMAKE_INSTALL_PREFIX}/aggregator_app.exe\" \"\" \"$<TARGET_FILE_DIR:aggregator_app>\")
    " COMPONENT aggregator)
endif()
