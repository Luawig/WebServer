add_subdirectory(buffer)
add_subdirectory(http)
add_subdirectory(log)
add_subdirectory(pool)
add_subdirectory(server)
add_subdirectory(timer)

add_executable(WebServer main.cpp)

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