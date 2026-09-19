cmake_minimum_required(VERSION 3.16)
project(ok_api LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED)
find_package(Threads REQUIRED)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/GameSession.cpp
    src/UdpServer.cpp
    src/TcpServer.cpp
    src/TcpConnection.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)

if(TARGET Boost::headers)
    target_link_libraries(${PROJECT_NAME} PRIVATE Boost::headers)
else()
    target_include_directories(${PROJECT_NAME} PRIVATE ${Boost_INCLUDE_DIRS})
endif()