file(GLOB SRC "./*.cpp")

add_executable(win_timer_ut ${SRC})
target_link_libraries(win_timer_ut PUBLIC GTest::gtest timerManager)
target_include_directories(win_timer_ut PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
