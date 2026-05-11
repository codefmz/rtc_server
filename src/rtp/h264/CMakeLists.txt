file(GLOB SRC "./*.cpp")

add_library(h264rtp STATIC ${SRC})
target_link_libraries(h264rtp PUBLIC rtp timer media)
target_include_directories(h264rtp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
