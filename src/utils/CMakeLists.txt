file(GLOB SRC "./*.cpp")

add_library(utils STATIC ${SRC})
target_include_directories(utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(utils PUBLIC base)