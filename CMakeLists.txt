cmake_minimum_required(VERSION 3.21.0)
project(webrtc VERSION 1.0.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message("PLATFORM_NAME: ${PLATFORM_NAME}")

if (PLATFORM_NAME STREQUAL "pc")
    find_package(jsoncpp REQUIRED CONFIG)
    # 兼容其他环境，config 模式下，使用目标 jsoncpp_lib, 其他环境使用 jsoncpp
    add_library(jsoncpp INTERFACE IMPORTED)
    set_target_properties(jsoncpp PROPERTIES
        INTERFACE_LINK_LIBRARIES jsoncpp_lib
    )
else()
    set(ENV{PKG_CONFIG_PATH} "/opt/V2.0/cxsw_sdk/t536/sysroot/usr/lib/pkgconfig")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(JSONCPP REQUIRED jsoncpp)

    message("JSONCPP_INCLUDE_DIRS: ${JSONCPP_INCLUDE_DIRS}")
    message("JSONCPP_LIBRARIES: ${JSONCPP_LIBRARIES}")
    # set(jsoncpp ${JSONCPP_LIBRARIES})

endif()

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/${PLATFORM_NAME}
    ${INCLUDE_DIR}
    ${ALLWINNER_DIR}
)

link_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/${PLATFORM_NAME}
    ${LIB_DIR}
)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/${PLATFORM_NAME})

add_subdirectory(src)
add_subdirectory(test)

add_executable(rtc_server main.cpp)
target_link_libraries(rtc_server webrtc ssl crypto dl v4l2 nozzlecalib) # ssl crypto dl 是 werbrtc 库libdatachannel依赖