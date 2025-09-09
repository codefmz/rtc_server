if [ $# != 2 ];
then
    echo "Invalid argument!"
    exit 1
fi

script_dir="$(cd "$(dirname "$0")" && pwd)"
echo "Script is located in: $script_dir"

target=$1

platform=$2

echo "compile target = ${target}, platform = ${platform}"

# echo "/opt/cxsw_sdk/${platform}/sysroot/"
export STAGING_DIR="/opt/cxsw_sdk/${platform}/sysroot/"

cmake -DCMAKE_TOOLCHAIN_FILE="./toolchain/${platform}.cmake" -S . -B build
cmake --build build --target ${target} -- -j$(nproc)