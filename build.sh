TOP_DIR=$(cd "$(dirname "$0")"; pwd)
echo "TOP_DIR=$TOP_DIR"

platform=""
target=""
debug=""

print_usage() {
    echo "Usage: sh $(basename "$0") [-p <platform>] [-t <target>] [-d <debug>]"
    echo "Supported platform : win, linux"
    echo "Supported target: "
    echo "eg: sh $(basename "$0") -p -t "
}

# 解析命令行参数
ARGS=$(getopt -o p:t: --name "$0" -- "$@")
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


echo "compile target = ${target}, platform = ${platform}"

cmake -S . -B build $cmake_params
cmake --build build -- -j$(nproc)