
cmake_minimum_required(VERSION 3.20)

project(myproject)

set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/dist" CACHE PATH "" FORCE)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Network Widgets)

qt_standard_project_setup()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(serverProgram 
    ./libs/protocol/src/serialization.cpp
    ./libs/server/src/TicketStore.cpp
    ./libs/server/src/ClientSession.cpp
    ./apps/server/src/main.cpp
    )
add_executable(clientProgram 
    ./libs/protocol/src/serialization.cpp
    ./apps/client/src/main.cpp
)

target_include_directories(serverProgram PRIVATE ${CMAKE_SOURCE_DIR}/libs/server/include)
target_include_directories(serverProgram PRIVATE ${CMAKE_SOURCE_DIR}/libs/protocol/include)
target_include_directories(clientProgram PRIVATE ${CMAKE_SOURCE_DIR}/libs/protocol/include)

target_link_libraries(clientProgram Qt6::Network Qt6::Widgets)
target_link_libraries(serverProgram Qt6::Network)

if(WIN32)
    install(TARGETS serverProgram clientProgram
        RUNTIME DESTINATION bin
    )

    qt_generate_deploy_app_script(
        TARGET serverProgram
        OUTPUT_SCRIPT server_deploy_script
    )

    qt_generate_deploy_app_script(
        TARGET clientProgram
        OUTPUT_SCRIPT clientProgram_deploy_script
    )

    install(SCRIPT ${server_deploy_script})
    install(SCRIPT ${clientProgram_deploy_script})
endif()