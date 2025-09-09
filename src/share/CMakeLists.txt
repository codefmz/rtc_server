file(GLOB SRC "./*.cpp")

add_library(packet STATIC ${SRC})
target_include_directories(packet PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})