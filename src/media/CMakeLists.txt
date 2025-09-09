file(GLOB SRC "./*.cpp")

add_library(media STATIC ${SRC})
target_link_libraries(media PUBLIC threadpool v4l2 base)
target_include_directories(media PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})