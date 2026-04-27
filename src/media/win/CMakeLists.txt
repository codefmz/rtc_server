file(GLOB SRC "./*.cpp")

add_library(v4l2_media STATIC ${SRC})
target_link_libraries(v4l2_media PUBLIC threadpool v4l2 base utils encoder)
target_include_directories(v4l2_media PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})