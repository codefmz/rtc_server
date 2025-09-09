file(GLOB SRC "./*.cpp")

add_library(timer STATIC ${SRC})
target_link_libraries(timer PUBLIC poller)
target_include_directories(timer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})