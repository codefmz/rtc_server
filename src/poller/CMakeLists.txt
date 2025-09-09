file(GLOB SRC "./*.cpp")

add_library(poller STATIC ${SRC})
target_link_libraries(poller PUBLIC event)
target_include_directories(poller PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})