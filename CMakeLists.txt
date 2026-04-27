cmake_minimum_required(VERSION 3.21.0)
project(webrtc VERSION 1.0.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message("PLATFORM_NAME: ${PLATFORM_NAME}")


include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/${PLATFORM_NAME}
    ${INCLUDE_DIR}
)

link_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/${PLATFORM_NAME}
)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/${PLATFORM_NAME})

add_subdirectory(src)
add_subdirectory(test)

add_executable(rtc_server main.cpp)
target_link_libraries(rtc_server webrtc ssl crypto dl v4l2 nozzlecalib) # ssl crypto dl 是 werbrtc 库libdatachannel依赖