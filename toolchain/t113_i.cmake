set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(PLATFORM_NAME  "t113_i")
set(SDK_DIR  "/opt/V2.0/cxsw_sdk/t113_i/gcc/toolchain-sunxi-glibc-gcc-830/toolchain")
set(TOOL_CHAIN_PATH "${SDK_DIR}/bin")
set(TOOLCHAIN_PREFIX "arm-openwrt-linux-gnueabi-")
set(LIB_DIR "/opt/V2.0/cxsw_sdk/t113_i/sysroot/usr/lib")
set(INCLUDE_DIR "/opt/V2.0/cxsw_sdk/t113_i/sysroot/usr/include")

set(CMAKE_C_COMPILER "${TOOL_CHAIN_PATH}/${TOOLCHAIN_PREFIX}gcc")
set(CMAKE_CXX_COMPILER "${TOOL_CHAIN_PATH}/${TOOLCHAIN_PREFIX}g++")
set(CMAKE_AR "${TOOL_CHAIN_PATH}/arm-openwrt-linux-gnueabi-ar")
set(CMAKE_RANLIB "${TOOL_CHAIN_PATH}/arm-openwrt-linux-gnueabi-ranlib")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -D_GLIBCXX_USE_CXX11_ABI=0：使用旧版 C++11 ABI,因为 lib  -Who-psabi 忽略abi 警告
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-psabi -D_GLIBCXX_USE_CXX11_ABI=0")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-psabi")