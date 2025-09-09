set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR mips32el)

SET(PLATFORM_NAME  "x2600")
SET(SDK_DIR  "/opt/cxsw_sdk/x2600/toolchains/mips-gcc720-glibc229")

SET(TOOL_CHAIN_PATH "${SDK_DIR}/bin")
SET(TOOLCHAIN_PREFIX "mips-linux-gnu-")
set(LIB_DIR "/opt/cxsw_sdk/x2600/sysroot/usr/lib")
set(INCLUDE_DIR "/opt/cxsw_sdk/x2600/sysroot/usr/include")

set(CMAKE_C_COMPILER "${TOOL_CHAIN_PATH}/${TOOLCHAIN_PREFIX}gcc")
set(CMAKE_CXX_COMPILER "${TOOL_CHAIN_PATH}/${TOOLCHAIN_PREFIX}g++")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=xburst2 -mtune=xburst2 -mfp64 -mnan=2008")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=xburst2 -mtune=xburst2 -mfp64 -mnan=2008 -D_GLIBCXX_USE_CXX11_ABI=0")