#ifndef _LIBV4L2_H_
#define _LIBV4L2_H_

#include <linux/videodev2.h>
#include <stdint.h>

struct v4l2_buf_unit {
    int                index;
    void*              start;
    uint32_t           length;
    uint32_t           offset;
};

struct v4l2_buf {
    struct v4l2_buf_unit* buf;
    int nr_bufs;
    enum v4l2_buf_type type;
};

int v4l2_open(const char* name, int flag);

int v4l2_close(int fd);

int v4l2_querycap(int fd, struct v4l2_capability* cap);

int v4l2_enuminput(int fd, int index, char* name);

int v4l2_s_input(int fd, int index);

int v4l2_enum_fmt(int fd, unsigned int fmt, enum v4l2_buf_type type);

int v4l2_s_fmt(int fd, int* width, int* height, unsigned int fmt, enum v4l2_buf_type type);

struct v4l2_buf* v4l2_reqbufs(int fd, enum v4l2_buf_type type, int nr_bufs);

int v4l2_querybuf(int fd, struct v4l2_buf* v4l2_buf);

int v4l2_mmap(int fd, struct v4l2_buf* v4l2_buf);

int v4l2_munmap(int fd, struct v4l2_buf* v4l2_buf);

int v4l2_relbufs(struct v4l2_buf* v4l2_buf);

int v4l2_streamon(int fd);

int v4l2_streamoff(int fd);

int v4l2_qbuf(int fd, struct v4l2_buf_unit* buf);

int v4l2_qbuf_all(int fd, struct v4l2_buf* v4l2_buf);

struct v4l2_buf_unit* v4l2_dqbuf(int fd, struct v4l2_buf* v4l2_buf);

int v4l2_g_ctrl(int fd, unsigned int id);

int v4l2_s_ctrl(int fd, unsigned int id, unsigned int value);

int v4l2_g_parm(int fd, struct v4l2_streamparm* streamparm);

int v4l2_s_parm(int fd, struct v4l2_streamparm *streamparm);

int v4l2_poll(int fd);

#endif //_LIBV4L2_H_
