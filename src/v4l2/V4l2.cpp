#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "plog/Log.h"
#include "V4l2.h"
#include "Base.h"

static int get_pixel_depth(unsigned int fmt)
{
    int depth = 0;

    switch (fmt) {
        case V4L2_PIX_FMT_NV12:
        case V4L2_PIX_FMT_NV21:
        case V4L2_PIX_FMT_YUV420:
        case V4L2_PIX_FMT_YVU420:
            depth = 12;
            break;

        case V4L2_PIX_FMT_RGB565:
        case V4L2_PIX_FMT_YUYV:
        case V4L2_PIX_FMT_YVYU:
        case V4L2_PIX_FMT_UYVY:
        case V4L2_PIX_FMT_VYUY:
        case V4L2_PIX_FMT_NV16:
        case V4L2_PIX_FMT_NV61:
        case V4L2_PIX_FMT_YUV422P:
            depth = 16;
            break;

        case V4L2_PIX_FMT_RGB32:
            depth = 32;
            break;
    }

    return depth;
}

int v4l2_open(const char* name, int flag)
{
    int fd = open(name, flag);
    if (fd < 0) {
        return -1;
    }

    return fd;
}



int v4l2_close(int fd)
{
    if (close(fd)) {
        return -1;
    }

    return 0;
}

int v4l2_querycap(int fd, struct v4l2_capability* cap)
{
    if (ioctl(fd, VIDIOC_QUERYCAP, cap) < 0) {
        return -1;
    }

    return 0;
}

int v4l2_enuminput(int fd, int index, char* name)
{
    struct v4l2_input input;
    int found = 0;

    input.index = 0;
    while (!ioctl(fd, VIDIOC_ENUMINPUT, &input)) {
        if (input.index == index) {
            found = 1;
            strcpy(name, (char*)input.name);
        }

        ++input.index;
    }

    if (!found) {
        return -1;
    }

    return 0;
}

int v4l2_s_input(int fd, int index)
{
    struct v4l2_input input;
    input.index = index;

    if (ioctl(fd, VIDIOC_S_INPUT, &input) < 0) {
        return -1;
    }

    return 0;
}



int v4l2_enum_fmt(int fd, unsigned int fmt, enum v4l2_buf_type type)
{
    struct v4l2_fmtdesc fmtdesc;
    int found = 0;

    fmtdesc.type = type;
    fmtdesc.index = 0;

    while (!ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc)) {
        if (fmtdesc.pixelformat == fmt) {
            found = 1;
            break;
        }

        fmtdesc.index++;
    }

    if (!found) {
        return -1;
    }

    return 0;
}



int v4l2_s_fmt(int fd, int* width, int* height, unsigned int fmt, enum v4l2_buf_type type)
{
    struct v4l2_format v4l2_fmt;
    struct v4l2_pix_format pixfmt;

    memset(&v4l2_fmt, 0, sizeof(struct v4l2_format));
    v4l2_fmt.type = type;

    memset(&pixfmt, 0, sizeof(pixfmt));

    pixfmt.width = *width;
    pixfmt.height = *height;
    pixfmt.pixelformat = fmt;

    // pixfmt.sizeimage = (*width * *height * get_pixel_depth(fmt)) / 8;
    // pixfmt.sizeimage = 0;

    pixfmt.field = V4L2_FIELD_NONE;

    v4l2_fmt.fmt.pix = pixfmt;

    if (ioctl(fd, VIDIOC_S_FMT, &v4l2_fmt) < 0) {
        return -1;
    }

    *width = v4l2_fmt.fmt.pix.width;
    *height = v4l2_fmt.fmt.pix.height;

    return 0;
}



struct v4l2_buf* v4l2_reqbufs(int fd, enum v4l2_buf_type type, int nr_bufs)
{
    struct v4l2_requestbuffers req;
    struct v4l2_buf* v4l2_buf;
    int i;

    req.count = nr_bufs;
    req.type = type;
    req.memory = V4L2_MEMORY_MMAP;

    if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
        return NULL;
    }

    v4l2_buf = (struct v4l2_buf*)malloc(sizeof(struct v4l2_buf));
    v4l2_buf->nr_bufs = req.count;
    v4l2_buf->buf = (struct v4l2_buf_unit*)malloc(sizeof(struct v4l2_buf_unit) * v4l2_buf->nr_bufs);
    v4l2_buf->type = type;

    memset(v4l2_buf->buf, 0, sizeof(struct v4l2_buf_unit) * v4l2_buf->nr_bufs);

    return v4l2_buf;
}



int v4l2_querybuf(int fd, struct v4l2_buf* v4l2_buf)
{
    struct v4l2_buffer buf;
    struct v4l2_buf_unit* buf_unit;
    int i;

    for(i = 0; i < v4l2_buf->nr_bufs; ++i) {
        buf.type = v4l2_buf->type;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (ioctl(fd , VIDIOC_QUERYBUF, &buf) < 0) {
            return -1;
        }

        buf_unit = &v4l2_buf->buf[i];
        buf_unit->index  = i;
        buf_unit->offset = buf.m.offset;
        buf_unit->length = buf.length;
        buf_unit->start = NULL;
    }

    return 0;
}



int v4l2_mmap(int fd, struct v4l2_buf* v4l2_buf)
{
    int i;
    struct v4l2_buf_unit* buf_unit;

    for(i = 0; i < v4l2_buf->nr_bufs; ++i) {
        buf_unit = &v4l2_buf->buf[i];
        buf_unit->start = mmap(0, buf_unit->length, PROT_READ | PROT_WRITE, MAP_SHARED,
                                fd, buf_unit->offset);

        if(buf_unit->start == MAP_FAILED) {
            goto err;
        }
    }

    return 0;

err:
    while(--i >= 0)
    {
        buf_unit = &v4l2_buf->buf[i];
        munmap(buf_unit->start, buf_unit->length);
        buf_unit->start = NULL;
    }

    return -1;
}

int v4l2_munmap(int fd, struct v4l2_buf* v4l2_buf)
{
    int i;
    struct v4l2_buf_unit* buf_unit;

    for(i = 0; i < v4l2_buf->nr_bufs; ++i) {
        buf_unit = &v4l2_buf->buf[i];
        munmap(buf_unit->start, buf_unit->length);
        buf_unit->start = NULL;
    }

    return 0;
}



int v4l2_relbufs(struct v4l2_buf* v4l2_buf)
{
    int i = 0;

    free(v4l2_buf->buf);
    free(v4l2_buf);

    return 0;
}

int v4l2_streamon(int fd)
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (ioctl(fd, VIDIOC_STREAMON, &type) < 0) {
        return -1;
    }

    if(v4l2_poll(fd) < 0) {
        return -1;
    }

    return 0;
}

int v4l2_streamoff(int fd)
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (ioctl(fd, VIDIOC_STREAMOFF, &type) < 0) {
        return -1;
    }

    return 0;
}

int v4l2_qbuf(int fd, struct v4l2_buf_unit* buf)
{
    struct v4l2_buffer v4l2_buf;

    v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_buf.memory = V4L2_MEMORY_MMAP;
    v4l2_buf.index = buf->index;

    if (ioctl(fd, VIDIOC_QBUF, &v4l2_buf) < 0) {
        return -1;
    }

    return 0;
}

int v4l2_qbuf_all(int fd, struct v4l2_buf* v4l2_buf)
{
    int i;

    for(i = 0; i < v4l2_buf->nr_bufs; ++i) {
        //不知道为什么，在t113 或者 x2600上这里需要占着cpu等一会，去掉后会出现 errno = 51 或者 53
        LOG_DEBUG << "v4l2_qbuf, i = " << i;
        if(v4l2_qbuf(fd, &v4l2_buf->buf[i])) {
            return -1;
        }
    }

    return 0;
}

struct v4l2_buf_unit* v4l2_dqbuf(int fd, struct v4l2_buf* v4l2_buf)
{
    struct v4l2_buffer buffer;

    buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buffer.memory = V4L2_MEMORY_MMAP;

    if (ioctl(fd, VIDIOC_DQBUF, &buffer) < 0) {
        return NULL;
    }
    v4l2_buf->buf[buffer.index].length = buffer.bytesused;
    return &v4l2_buf->buf[buffer.index];
}

int v4l2_g_ctrl(int fd, unsigned int id)
{
    struct v4l2_control ctrl;

    ctrl.id = id;

    if (ioctl(fd, VIDIOC_G_CTRL, &ctrl) < 0) {
        return -1;
    }

    return ctrl.value;

}

int v4l2_s_ctrl(int fd, unsigned int id, unsigned int value)
{
    struct v4l2_control ctrl;

    ctrl.id = id;
    ctrl.value = value;

    if (ioctl(fd, VIDIOC_S_CTRL, &ctrl) < 0) {
        return -1;
    }

    return ctrl.value;
}

int v4l2_g_parm(int fd, struct v4l2_streamparm* streamparm)
{
    streamparm->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_G_PARM, streamparm) < 0) {
        return -1;
    }

    return 0;
}

int v4l2_s_parm(int fd, struct v4l2_streamparm *streamparm)
{
    streamparm->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (ioctl(fd, VIDIOC_S_PARM, streamparm) < 0) {
        return -1;
    }

    return 0;
}

int v4l2_poll(int fd)
{
    int ret;
    struct pollfd poll_fds[1];

    poll_fds[0].fd = fd;
    poll_fds[0].events = POLLIN;

    ret = poll(poll_fds, 1, 100000);
    if (ret < 0) {
        return -1;
    }

    if (ret == 0) {
        return -1;
    }

    return 0;
}