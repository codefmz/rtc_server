file(GLOB SRC "./*.cpp")

add_library(base OBJECT ${SRC})
target_link_libraries(base PUBLIC plog::plog)
target_include_directories(base PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})