file(GLOB SRC "./*.cpp")

add_library(win_timer STATIC ${SRC})
find_package(Threads REQUIRED)
target_link_libraries(win_timer PUBLIC timer event Threads::Threads)
target_include_directories(win_timer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
