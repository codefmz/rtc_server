file(GLOB SRC "./*.cpp")

add_executable(rtpsink_ut ${SRC})
target_link_libraries(rtpsink_ut PUBLIC GTest::gtest linux_timer media plog::plog winMedia)