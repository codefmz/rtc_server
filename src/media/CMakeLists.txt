if(WIN32)
    add_subdirectory(win)
else()
    add_subdirectory(v4l2)
endif()

file(GLOB SRC "./*.cpp")

add_library(media STATIC ${SRC})
target_link_libraries(media PUBLIC threadpool utils)
target_include_directories(media PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})