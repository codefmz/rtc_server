set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

SET(PLATFORM_NAME  "t536")
SET(SDK_PATH "/opt/V2.0/cxsw_sdk/t536")
SET(SDK_DIR "${SDK_PATH}/toolchains/gcc-arm-10.3-2021.07-x86_64-aarch64-none-linux-gnu/")
SET(TOOL_CHAIN_PATH "${SDK_DIR}bin/")
SET(TOOLCHAIN_PREFIX "aarch64-none-linux-gnu-")
set(LIB_DIR "/opt/V2.0/cxsw_sdk/t536/sysroot/usr/lib")
set(ALLWINNER_DIR "/opt/V2.0/cxsw_sdk/t536/sysroot/usr/include/allwinner/include")
# 环境变量只有cmake生效，make不生效
set(ENV{STAGING_DIR} "${SDK_PATH}/sysroot/")
message("STAGING_DIR=$ENV{STAGING_DIR}")

set(STAGING_DIR "${SDK_PATH}/sysroot/")
message("STAGING_DIR=${STAGING_DIR}")

set(CMAKE_C_COMPILER "${TOOL_CHAIN_PATH}${TOOLCHAIN_PREFIX}gcc")
set(CMAKE_CXX_COMPILER "${TOOL_CHAIN_PATH}${TOOLCHAIN_PREFIX}g++")

set(CMAKE_FIND_ROOT_PATH "${SDK_PATH}/sysroot/")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath-link,/opt/V2.0/cxsw_sdk/t536/sysroot/usr/lib")