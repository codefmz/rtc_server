file(GLOB SRC "./*.cpp")

add_executable(threadpool_ut ${SRC})
target_link_libraries(threadpool_ut PUBLIC gtest threadpool)
target_include_directories(threadpool_ut PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})