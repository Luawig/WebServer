add_library(LogLib log.cpp)

target_link_libraries(LogLib
        PUBLIC
        BufferLib
)

target_include_directories(LogLib
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)
