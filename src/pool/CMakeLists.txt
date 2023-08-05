add_library(PoolLib sqlconnpool.cpp threadpool.cpp)

target_link_libraries(PoolLib
        PUBLIC
        LogLib
        mysqlclient
)

target_include_directories(PoolLib
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}../include
)
