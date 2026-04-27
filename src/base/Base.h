#ifndef _BSSE_H_
#define _BSSE_H_

#include <stdint.h>
#include <string>
#include <vector>

constexpr uint16_t FPS = 30;
constexpr uint32_t FRAME_WIDTH = 1920;
constexpr uint32_t FRAME_HEIGHT = 1080;

constexpr uint16_t DEFAULT_FRAME_NUM = 2; // 缓存帧数
constexpr uint16_t H264_FRAME_MAX_NALU_NUM = 6; // 一次h264流最多包含的NALU个数
constexpr uint32_t H264_FRAME_MAX_NALU_SIZE = 1024 * 1024; // 一次h264流最大的NALU大小

constexpr const char * CAMERA_NAME = "CCX2F3298";
constexpr const char * KLIPPY_UNIX_SOCKET_PATH = "/tmp/klippy_uds";

#endif
