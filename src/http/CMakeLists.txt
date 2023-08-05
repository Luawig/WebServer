add_library(HttpLib httpConnection.cpp httpRequest.cpp httpResponse.cpp)

target_link_libraries(HttpLib
        PUBLIC
        LogLib
        BufferLib
        PoolLib
)

target_include_directories(HttpLib
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)
