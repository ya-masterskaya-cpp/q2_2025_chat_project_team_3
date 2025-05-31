cmake_minimum_required(VERSION 3.18)
project(MyProjectCommonSubproject LANGUAGES CXX)

find_package(Drogon CONFIG REQUIRED)

add_library(common_lib STATIC
    src/dummy.cpp
)

target_include_directories(common_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(common_lib PUBLIC Drogon::Drogon)
