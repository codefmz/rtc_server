file(GLOB SRC "./*.cpp")

add_library(winMedia STATIC ${SRC})
target_link_libraries(winMedia PUBLIC media threadpool base utils)
target_include_directories(winMedia PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})