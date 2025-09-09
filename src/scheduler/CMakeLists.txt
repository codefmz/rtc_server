file(GLOB SRC "./*.cpp")

add_library(scheduler STATIC ${SRC})
target_link_libraries(scheduler PUBLIC poller timer)
target_include_directories(scheduler PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})