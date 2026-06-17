



cmake_minimum_required(VERSION 3.20)

project(myproject)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Network Widgets)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(myprogram ./apps/client/src/main.cpp)
add_executable(serverProgram ./apps/server/src/main.cpp)

target_link_libraries(myprogram Qt6::Widgets Qt6::Network)
target_link_libraries(serverProgram Qt6::Widgets Qt6::Network)