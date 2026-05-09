#!/usr/bin/env bash

export VCPKG_ROOT="E:/3rd/vcpkg"

SDK_VER="10.0.26100.0"

VS_ROOT="D:/instal_package/visual studio"

MSVC_VER="14.43.34808"

WIN_KITS_ROOT="C:/Program Files (x86)/Windows Kits/10"

STRAWBERRY_ROOT="/c/Strawberry"
STRAWBERRY_PERL_BIN="${STRAWBERRY_ROOT}/perl/bin"

export PERL="${STRAWBERRY_PERL_BIN}/perl.exe"

export PATH="${STRAWBERRY_PERL_BIN}:/d/instal_package/cmake/bin:/d/instal_package/visual studio/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja:/d/instal_package/visual studio/VC/Tools/MSVC/${MSVC_VER}/bin/Hostx64/x64:/c/Program Files (x86)/Windows Kits/10/bin/${SDK_VER}/x64:/c/Program Files (x86)/Windows Kits/10/bin/x64:$PATH"

export RC="${WIN_KITS_ROOT}/bin/${SDK_VER}/x64/rc.exe"

export CMAKE_MT="${WIN_KITS_ROOT}/bin/${SDK_VER}/x64/mt.exe"

# Keep Windows SDK headers ahead of MSVC headers so COM/RPC headers match.

export INCLUDE="${WIN_KITS_ROOT}/Include/${SDK_VER}/ucrt;${WIN_KITS_ROOT}/Include/${SDK_VER}/shared;${WIN_KITS_ROOT}/Include/${SDK_VER}/um;${WIN_KITS_ROOT}/Include/${SDK_VER}/winrt;${WIN_KITS_ROOT}/Include/${SDK_VER}/cppwinrt;${VS_ROOT}/VC/Tools/MSVC/${MSVC_VER}/include"

export LIB="${WIN_KITS_ROOT}/Lib/${SDK_VER}/ucrt/x64;${WIN_KITS_ROOT}/Lib/${SDK_VER}/um/x64;${VS_ROOT}/VC/Tools/MSVC/${MSVC_VER}/lib/x64"

export LIBPATH="${WIN_KITS_ROOT}/UnionMetadata/${SDK_VER};${WIN_KITS_ROOT}/References/${SDK_VER};${VS_ROOT}/VC/Tools/MSVC/${MSVC_VER}/lib/x64"

export VCPKG_KEEP_ENV_VARS="PATH;LIB;LIBPATH;INCLUDE;RC;CMAKE_MT;PERL"

echo "where cl:"

cmd //c where cl

echo "where rc:"

cmd //c where rc

echo "where mt:"

cmd //c where mt

echo "where link:"

cmd //c where link

echo "where perl:"

cmd //c where perl
