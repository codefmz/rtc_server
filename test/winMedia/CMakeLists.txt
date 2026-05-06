file(GLOB SRC "./*.cpp")

add_executable(winMedia_ut ${SRC})
target_link_libraries(winMedia_ut PUBLIC GTest::gtest plog::plog winMedia)
target_include_directories(winMedia_ut PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})