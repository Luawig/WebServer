add_library(BufferLib buffer.cpp)

target_include_directories(BufferLib
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)
