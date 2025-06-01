cmake_minimum_required(VERSION 3.18)
project(MyProjectV2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_SERVER "Build the server application" ON)
option(BUILD_CLIENT "Build the client application" ON)

add_subdirectory(common)

if(BUILD_SERVER)
    add_subdirectory(server)
endif()

if(BUILD_CLIENT)
    add_subdirectory(client)
endif()
