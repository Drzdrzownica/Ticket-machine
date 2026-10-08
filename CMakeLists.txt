
cmake_minimum_required(VERSION 3.20)

project(myproject)

set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/dist" CACHE PATH "" FORCE)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

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

    ./libs/client/src/UI/ErrorStatePage.cpp
    ./libs/client/include/client/UI/ErrorStatePage.h


    ./libs/client/src/UI/ErrorTryAgainPage.cpp
    ./libs/client/include/client/UI/ErrorTryAgainPage.h
    
    ./libs/client/src/UI/LoginPage.cpp
    ./libs/client/include/client/UI/LoginPage.h

    ./libs/client/src/UI/MainPage.cpp
    ./libs/client/include/client/UI/MainPage.h

    ./libs/client/src/UI/LanguagesPage.cpp
    ./libs/client/include/client/UI/LanguagesPage.h

    ./libs/client/src/UI/ChooseTicketPage.cpp
    ./libs/client/include/client/UI/ChooseTicketPage.h

    ./libs/client/src/UI/PaymentPage.cpp
    ./libs/client/include/client/UI/PaymentPage.h

    ./libs/client/src/UI/InputPersonalDataPage.cpp
    ./libs/client/include/client/UI/InputPersonalDataPage.h

    ./libs/client/src/UI/DebugEditCoinsPage.cpp
    ./libs/client/include/client/UI/DebugEditCoinsPage.h

    ./libs/client/src/UI/ReturningMoneyPage.cpp
    ./libs/client/include/client/UI/ReturningMoneyPage.h

    ./libs/client/src/UI/PrintingPage.cpp
    ./libs/client/include/client/UI/PrintingPage.h
)

target_include_directories(serverProgram PRIVATE ${CMAKE_SOURCE_DIR}/libs/server/include)
target_include_directories(serverProgram PRIVATE ${CMAKE_SOURCE_DIR}/libs/protocol/include)
target_include_directories(clientProgram PRIVATE ${CMAKE_SOURCE_DIR}/libs/client/include)
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