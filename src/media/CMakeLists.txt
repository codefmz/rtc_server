
add_subdirectory(win)
# add_subdirectory(v4l2)

file(GLOB SRC "./*.cpp")

add_library(media STATIC ${SRC})
target_link_libraries(media PUBLIC threadpool utils)
target_include_directories(media PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})