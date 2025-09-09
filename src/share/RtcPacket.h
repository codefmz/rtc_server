#pragma once

#include <stdint.h>

enum RTC_CMD {
    CMD_SHOW_VIDEO = 0,
    CMD_STOP_VIDEO,
    CMD_KLIPPY_MOVE,
    CMD_KLIPPY_XY,
    CMD_KLIPPY_Z,
    CMD_BUTT
};

enum RTC_RET {
    RET_OK = 0,
    RET_ERR,
    RET_KLIPPY_POS,
};

constexpr int DEFAULT_BUF_SIZE = 1024 * 1024;
constexpr int INPUT_HEADER_SIZE = sizeof(RTC_CMD) + sizeof(uint32_t);
constexpr int OUTPUT_HEADER_SIZE = sizeof(RTC_RET) + sizeof(uint32_t);

typedef struct {
    RTC_CMD cmd;
    uint32_t len;
    uint8_t buf[DEFAULT_BUF_SIZE];
} RTC_Input;

typedef struct {
    RTC_RET ret;
    uint32_t len;
    uint8_t buf[DEFAULT_BUF_SIZE];
} RTC_Output;

typedef struct {
    double x;
    double y;
    double z;
} RTC_Pos_Param;
