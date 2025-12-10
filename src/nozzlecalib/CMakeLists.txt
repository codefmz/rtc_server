file(GLOB SRC "./*.cpp")

add_library(nozzlecalib STATIC ${SRC})
target_link_libraries(nozzlecalib PUBLIC utils klipper)
target_include_directories(nozzlecalib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})