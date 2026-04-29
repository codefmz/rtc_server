file(GLOB SRC "./*.cpp")

add_library(threadpool STATIC ${SRC})
find_package(Threads REQUIRED)
target_link_libraries(threadpool PUBLIC Threads::Threads)
target_include_directories(threadpool PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})