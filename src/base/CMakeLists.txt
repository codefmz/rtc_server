file(GLOB SRC "./*.cpp")

add_library(base OBJECT ${SRC})
target_include_directories(base PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})