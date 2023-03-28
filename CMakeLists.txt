cmake_minimum_required(VERSION 3.19)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(http_client)

set(CMAKE_CXX_STANDARD 20)

add_executable(http_client client.cpp)
target_link_libraries(http_client crypto pthread ssl boost_regex)
file(MAKE_DIRECTORY pages)