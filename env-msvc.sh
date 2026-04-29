#!/usr/bin/env bash

export VCPKG_ROOT="E:/3rd/vcpkg"

export PATH="/d/instal_package/cmake/bin:/d/instal_package/visual studio/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja:/d/instal_package/visual studio/VC/Tools/MSVC/14.43.34808/bin/Hostx64/x64:/c/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64:/c/Program Files (x86)/Windows Kits/10/bin/x64:$PATH"

export RC="C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/rc.exe"
export CMAKE_MT="C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/mt.exe"

export INCLUDE="D:/instal_package/visual studio/VC/Tools/MSVC/14.43.34808/include;C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0/ucrt;C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0/shared;C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0/um;C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0/winrt;C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0/cppwinrt;$INCLUDE"

export LIB="D:/instal_package/visual studio/VC/Tools/MSVC/14.43.34808/lib/x64;C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/ucrt/x64;C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/um/x64;$LIB"

export LIBPATH="D:/instal_package/visual studio/VC/Tools/MSVC/14.43.34808/lib/x64;C:/Program Files (x86)/Windows Kits/10/UnionMetadata/10.0.26100.0;C:/Program Files (x86)/Windows Kits/10/References/10.0.26100.0;$LIBPATH"

export VCPKG_KEEP_ENV_VARS="PATH;LIB;LIBPATH;INCLUDE;RC;CMAKE_MT"

echo "where cl:"
cmd //c where cl
echo "where rc:"
cmd //c where rc
echo "where mt:"
cmd //c where mt
echo "where link:"
cmd //c where link
