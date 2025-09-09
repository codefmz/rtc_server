file(GLOB SRC "./*.cpp")

add_library(threadpool STATIC ${SRC})
target_link_libraries(threadpool PUBLIC pthread)
target_include_directories(threadpool PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})