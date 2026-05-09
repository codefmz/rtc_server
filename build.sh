TOP_DIR=$(cd "$(dirname "$0")"; pwd)
echo "TOP_DIR=$TOP_DIR"

platform=""
target=""
debug=""
rebuild=""

print_usage() {
    echo "Usage: sh $(basename "$0") [-p <platform>] [-t <target>] [-d <debug>] [-r <rebuild>]"
    echo "Supported platform : win, linux"
    echo "Supported target: winMedia_ut"
    echo "eg: sh $(basename "$0") -p win -t winMedia_ut"
}

# 解析命令行参数
ARGS=$(getopt -o p:t:d:r --name "$0" -- "$@")
if [ $? -ne 0 ]; then
    echo "Error in command line arguments."
    print_usage
    exit 1
fi

eval set -- "$ARGS"
while true; do
    case "$1" in
        -p)
            platform="$2"
            shift 2
            ;;
        -t)
            target="$2"
            shift 2
            ;;
        -d)
            debug="$2"
            shift 2
            ;;
        -r)
            rebuild="y"
            shift 1
            ;;
        --)
            shift
            break
            ;;
        *)
            echo "Invalid command."
            print_usage
            exit 1
            ;;
    esac
done

cmake_params=""
if [ "$debug" == "y" ]; then
    cmake_params="-DCMAKE_BUILD_TYPE=Debug"
else
    cmake_params="-DCMAKE_BUILD_TYPE=Release"
fi

if [ "$target" != "" ]; then
    cmake_params="$cmake_params -DTARGET=$target"
fi

if [ "$rebuild" == "y" ]; then
    rm -rf build
    cmake -B build --preset=default $cmake_params
fi

echo "compile target = ${target}, platform = ${platform}"
cmake --build build -- -j$(nproc)