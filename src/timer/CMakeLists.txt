add_library(TimerLib heaptimer.cpp)

target_include_directories(TimerLib
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)
