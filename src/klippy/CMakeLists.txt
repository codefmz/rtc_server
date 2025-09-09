file(GLOB SRC "./*.cpp")

add_library(klipper STATIC ${SRC})
target_link_libraries(klipper PUBLIC jsoncpp poller packet)
target_include_directories(klipper PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR})