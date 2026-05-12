file(GLOB SRC "./*.cpp")

add_library(timerManager STATIC ${SRC})
target_link_libraries(timerManager PUBLIC timer event Threads::Threads)
target_include_directories(timerManager PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
