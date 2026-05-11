file(GLOB SRC "./*.cpp")

add_executable(linux_timer_ut ${SRC})
target_link_libraries(linux_timer_ut PUBLIC GTest::gtest timerManager)
target_include_directories(linux_timer_ut PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})