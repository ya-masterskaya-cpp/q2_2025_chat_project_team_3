cmake_minimum_required(VERSION 3.18)
project(MyDrogonApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Option to control sanitizer activation, set by CMakePresets.json
# Possible values: OFF, ASAN_UBSAN, TSAN
set(MYPROJECT_ENABLE_SANITIZERS "OFF" CACHE STRING "Enable sanitizers: OFF, ASAN_UBSAN, TSAN")
set_property(CACHE MYPROJECT_ENABLE_SANITIZERS PROPERTY STRINGS OFF ASAN_UBSAN TSAN)

find_package(Drogon CONFIG REQUIRED)

add_executable(main
    src/main.cpp
    src/models/Migrations.cc
    src/models/Users.cc
    src/models/Rooms.cc
)
target_include_directories(main PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(main PRIVATE Drogon::Drogon)

if(NOT MYPROJECT_ENABLE_SANITIZERS STREQUAL "OFF")
    if(CMAKE_BUILD_TYPE STREQUAL "Release") # Only apply project sanitizers to Release builds
        set(SPECIFIC_SANITIZER_FLAG "")

        if(MYPROJECT_ENABLE_SANITIZERS STREQUAL "ASAN_UBSAN")
            set(SPECIFIC_SANITIZER_FLAG "-fsanitize=address,undefined")
        elseif(MYPROJECT_ENABLE_SANITIZERS STREQUAL "TSAN")
            set(SPECIFIC_SANITIZER_FLAG "-fsanitize=thread")
        endif()

        if(NOT SPECIFIC_SANITIZER_FLAG STREQUAL "")
            set(PROJECT_SANITIZER_COMPILE_FLAGS "-gline-tables-only" ${SPECIFIC_SANITIZER_FLAG} "-fno-omit-frame-pointer")
            set(PROJECT_SANITIZER_LINK_FLAGS ${SPECIFIC_SANITIZER_FLAG})

            target_compile_options(main PRIVATE ${PROJECT_SANITIZER_COMPILE_FLAGS})
            target_link_options(main PRIVATE ${PROJECT_SANITIZER_LINK_FLAGS})
        endif()
    endif()
endif()

add_custom_command(TARGET main POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/config.json"
        "$<TARGET_FILE_DIR:main>/config.json"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/db/main.sql"
        "$<TARGET_FILE_DIR:main>/db/main.sql"
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/db/migrations"
        "$<TARGET_FILE_DIR:main>/db/migrations"
)
