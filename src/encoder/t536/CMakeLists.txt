file(GLOB SRC "./*.cpp")

add_library(encoder STATIC ${SRC})
target_include_directories(encoder PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(encoder PUBLIC vencoder MemAdapter pthread dl m utils)

