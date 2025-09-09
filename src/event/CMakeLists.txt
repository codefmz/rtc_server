file(GLOB SRC "./*.cpp")

add_library(event STATIC ${SRC})
target_include_directories(event PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})