TOP_DIR=$(cd "$(dirname "$0")"; pwd)
echo "TOP_DIR=$TOP_DIR"

platform=""
target=""
toolchain_file=""

print_usage() {
    echo "Usage: sh $(basename "$0") [-p <platform>] [-t <target>]"
    echo "Supported platform : t113_i, pc"
    echo "Supported target: , , , "
    echo "eg: sh $(basename "$0") -p t113_i -t gcode_param_ut"
}

get_toolchain_file() {
    case "$1" in
        t113_i)
            echo "toolchains/t113_i.toolchain.cmake"
            ;;    
        pc)
            echo ""
            ;;
        *)
            echo ""
            ;;
    esac
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
            toolchain_file=$(get_toolchain_file "$platform")
            if [ -z "$toolchain_file" ] && [ "$platform" != "pc" ]; then
                echo "Error: Unsupported chip name '$platform'."
                print_usage
                exit 1
            fi
            shift 2
            ;;
        -t)
            target="$2"
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

echo "compile target = ${target}, platform = ${platform}"

if [ "$platform" != "pc" ]; then
    export STAGING_DIR="/opt/cxsw_sdk/${platform}/sysroot/"
    echo "/opt/cxsw_sdk/${platform}/sysroot/"
fi

cmake -DCMAKE_TOOLCHAIN_FILE="./toolchain/${platform}.cmake" -S . -B build
cmake --build build --target ${target} -- -j$(nproc)

if [ "$platform" != "pc" ]; then
    unset STAGING_DIR
    echo "unsetenv=>STAGING_DIR: $STAGING_DIR"
fi