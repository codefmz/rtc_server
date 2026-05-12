file(GLOB SRC "./*.cpp")

add_executable(rtpsink_ut ${SRC})
target_link_libraries(rtpsink_ut PUBLIC GTest::gtest timerManager media plog::plog winMedia h264rtp Ws2_32)