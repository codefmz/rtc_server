file(GLOB SRC "./*.cpp")

add_library(v4l2 STATIC ${SRC})
target_link_libraries(v4l2 PUBLIC base)
target_include_directories(v4l2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})