cmake_minimum_required(VERSION 3.5)

project(WebServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(src)

add_executable(WebServer src/main.cpp)

target_link_libraries(WebServer
        PUBLIC
        BufferLib
        HttpLib
        LogLib
        PoolLib
        ServerLib
        TimerLib
)

target_include_directories(WebServer
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
