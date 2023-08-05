add_library(ServerLib epoller.cpp webserver.cpp)

target_link_libraries(ServerLib
        PUBLIC
        LogLib
        TimerLib
        PoolLib
        HttpLib
)

target_include_directories(ServerLib
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)
