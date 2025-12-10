#ifndef _LIBV4L2_H_
#define _LIBV4L2_H_

#include <linux/videodev2.h>
#include <stdint.h>

#define MAX_PLANES 3   // NV12=2，YUV420M=3，一般最大3即可

struct v4l2_buf_unit {
    uint32_t index;
    uint32_t bytesused;                   // 本 plane 实际有效数据
    void* start[MAX_PLANES];              // mmap 地址
    uint32_t length[MAX_PLANES];          // mmap size  
    uint32_t offset[MAX_PLANES];          // offset from QUERYBUF
};

struct v4l2_buf {
    struct v4l2_buf_unit* buf;  // 数组
    int nr_bufs;                // buffer 数量
    int num_planes;
    enum v4l2_buf_type type;
};

int v4l2_open(const char* name, int flag);

int v4l2_close(int fd);

int v4l2_querycap(int fd, struct v4l2_capability* cap);

int v4l2_enuminput(int fd, int index, char* name);

int v4l2_s_input(int fd, int index);

int v4l2_enum_fmt(int fd, unsigned int fmt, enum v4l2_buf_type type);

int v4l2_s_fmt(int fd, unsigned int fmt, enum v4l2_buf_type type, int* width, int* height, int *planesNum);

struct v4l2_buf* v4l2_reqbufs(int fd, enum v4l2_buf_type type, int nr_bufs);

int v4l2_querybuf(int fd, struct v4l2_buf* v4l2_buf);

int v4l2_mmap(int fd, struct v4l2_buf* v4l2_buf);

int v4l2_munmap(int fd, struct v4l2_buf* v4l2_buf);

int v4l2_relbufs(int fd, struct v4l2_buf* v4l2_buf);

int v4l2_streamon(int fd, enum v4l2_buf_type type);

int v4l2_streamoff(int fd, enum v4l2_buf_type type);

int v4l2_qbuf(int fd, struct v4l2_buf_unit* buf, struct v4l2_buf *v4l2Buf);

int v4l2_qbuf_all(int fd, struct v4l2_buf* v4l2_buf);

struct v4l2_buf_unit* v4l2_dqbuf(int fd, struct v4l2_buf* v4l2_buf);

int v4l2_g_ctrl(int fd, unsigned int id);

int v4l2_s_ctrl(int fd, unsigned int id, unsigned int value);

int v4l2_g_parm(int fd, struct v4l2_streamparm* streamparm);

int v4l2_s_parm(int fd, struct v4l2_streamparm *streamparm);

int v4l2_poll(int fd);

#endif //_LIBV4L2_H_
