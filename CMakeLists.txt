cmake_minimum_required(VERSION 3.21.0)
project(webrtc VERSION 1.0.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message("PLATFORM_NAME: ${PLATFORM_NAME}")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/${PLATFORM_NAME})

find_package(GTest CONFIG REQUIRED)
find_package(plog CONFIG REQUIRED)

add_subdirectory(src)
add_subdirectory(test)

# add_executable(rtc_server main.cpp)
# target_link_libraries(rtc_server webrtc ssl crypto dl v4l2 nozzlecalib) # ssl crypto dl 是 werbrtc 库libdatachannel依赖