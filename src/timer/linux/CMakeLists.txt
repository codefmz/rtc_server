file(GLOB SRC "./*.cpp")

add_library(linux_timer STATIC ${SRC})
target_link_libraries(linux_timer PUBLIC timer event poller utils)
target_include_directories(linux_timer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})