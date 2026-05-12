file(GLOB SRC "./*.cpp")

add_library(select_poll STATIC ${SRC})
target_link_libraries(select_poll PUBLIC poll event)
target_include_directories(select_poll PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})