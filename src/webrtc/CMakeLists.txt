file(GLOB SRC "./*.cpp")

add_library(webrtc STATIC ${SRC})
target_link_libraries(webrtc PUBLIC datachannel rtp packet)
target_include_directories(webrtc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})