if(WIN32)
    add_subdirectory(win)
else()
    add_subdirectory(linux)
endif()


file(GLOB SRC "./*.cpp")

add_library(timer STATIC ${SRC})
target_link_libraries(timer PUBLIC event)
target_include_directories(timer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})