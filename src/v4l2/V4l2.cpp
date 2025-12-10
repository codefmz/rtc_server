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
    int fd = open(name, flag, 0);
    if (fd < 0) {
        PLOGE << "open " << name << " failed, errno : " << errno;
        return -1;
    }

    return fd;
}



int v4l2_close(int fd)
{
    if (close(fd)) {
        PLOGE << "close failed, errno : " << errno;
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
    struct v4l2_input input = { 0 };
    input.index = index;
    input.type = V4L2_INPUT_TYPE_CAMERA;

    if (ioctl(fd, VIDIOC_S_INPUT, &input) < 0) {
        PLOGE << "VIDIOC_S_INPUT, errno = " << errno;
        return -1;
    }

    return 0;
}



int v4l2_enum_fmt(int fd, unsigned int fmt, enum v4l2_buf_type type)
{
    struct v4l2_fmtdesc fmtdesc = { 0 };
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

int v4l2_s_fmt(int fd, unsigned int fmt, enum v4l2_buf_type type, int* width, int* height, int *planesNum)
{
    struct v4l2_format v4l2_fmt = { 0 };
    v4l2_fmt.type = type;
    if (type == V4L2_CAP_VIDEO_CAPTURE_MPLANE) {
        v4l2_fmt.fmt.pix_mp.width = *width;
        v4l2_fmt.fmt.pix_mp.height = *height;
        v4l2_fmt.fmt.pix_mp.pixelformat = fmt;
        v4l2_fmt.fmt.pix_mp.field = V4L2_FIELD_NONE;
    } else {
        v4l2_fmt.fmt.pix.width = *width;
        v4l2_fmt.fmt.pix.height = *height;
        v4l2_fmt.fmt.pix.pixelformat = fmt;
		v4l2_fmt.fmt.pix.field = V4L2_FIELD_NONE;
    }

    if (ioctl(fd, VIDIOC_S_FMT, &v4l2_fmt) < 0) {
        PLOGE << "VIDIOC_S_FMT, errno = " << errno;
        return -1;
    }

    if (type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
        *width = v4l2_fmt.fmt.pix_mp.width;
        *height = v4l2_fmt.fmt.pix_mp.height;
        if (ioctl(fd, VIDIOC_G_FMT, &v4l2_fmt) < 0) {
            PLOGE << "VIDIOC_G_FMT, errno = " << errno;
            return -1;
        }
        int num = v4l2_fmt.fmt.pix_mp.num_planes;
        *planesNum = v4l2_fmt.fmt.pix_mp.num_planes;
        PLOGI << "VIDIOC_G_FMT num_planes = " << (int)v4l2_fmt.fmt.pix_mp.num_planes << " planeNum = " << *planesNum << " num = " << num;
    } else {
        *width = v4l2_fmt.fmt.pix.width;
        *height = v4l2_fmt.fmt.pix.height;
    }

    PLOGI << " *width = " << *width << " *height = " << *height;
    return 0;
}

struct v4l2_buf* v4l2_reqbufs(int fd, enum v4l2_buf_type type, int nr_bufs)
{
    struct v4l2_requestbuffers req = { 0 };
    struct v4l2_buf* v4l2_buf;

    req.count = nr_bufs;
    req.type = type;
    req.memory = V4L2_MEMORY_MMAP;

    if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
        PLOGE << "ioctl VIDIOC_REQBUFS failed, errno = " << errno;
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
    struct v4l2_buf_unit* buf_unit;
    int i;

    for(i = 0; i < v4l2_buf->nr_bufs; ++i) {
        struct v4l2_plane planes[VIDEO_MAX_PLANES];
        struct v4l2_buffer buf = { 0 };
        memset(planes, 0, sizeof(planes));

        buf.type = v4l2_buf->type;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (v4l2_buf->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
            buf.m.planes = planes;
            buf.length = v4l2_buf->num_planes;
        }

        if (ioctl(fd , VIDIOC_QUERYBUF, &buf) < 0) {
            PLOGE << "VIDIOC_QUERYBUF, errno = " << errno;
            return -1;
        }

        buf_unit = &v4l2_buf->buf[i];
        buf_unit->index  = i;

        if (v4l2_buf->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
            for (int j = 0; j < v4l2_buf->num_planes; j++) {
                buf_unit->length[j] = planes[j].length;
                buf_unit->offset[j] = planes[j].m.mem_offset;
                buf_unit->start[j] = NULL;
                PLOGD << "plane[" << j << "] length:" << buf_unit->length[j] << ", offset:" << buf_unit->offset[j];
            }
        } else {
            buf_unit->length[0] = buf.length;
            buf_unit->offset[0] = buf.m.offset;
            buf_unit->start[0] = NULL;
        }
    }

    return 0;
}

int v4l2_mmap(int fd, struct v4l2_buf* v4l2_buf)
{
    int i = 0, j = 0;
    struct v4l2_buf_unit* buf_unit;

    for(i = 0; i < v4l2_buf->nr_bufs; ++i) {
        buf_unit = &v4l2_buf->buf[i];
        int num = v4l2_buf->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE ? v4l2_buf->num_planes : 1;
        for (j = 0; j < num; ++j) {
            buf_unit->start[j] = mmap(NULL, buf_unit->length[j], PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf_unit->offset[j]);
            if (buf_unit->start[j] == MAP_FAILED) {
                PLOGE << "mmap failed, errno:" << errno << ", error:" << strerror(errno);
                goto err;
            }
            PLOGD << "mmap success, buf_unit->start:" << buf_unit->start[j] << ", buf_unit->length:" << buf_unit->length[j]
                << ", buf_unit->offset:" << buf_unit->offset[j] << " j = " <<j;
        }
    }

    return 0;

err:
    while(--i >= 0) {
        buf_unit = &v4l2_buf->buf[i];
        while (--j >= 0) {
            munmap(buf_unit->start[j], buf_unit->length[j]);
            buf_unit->start[j] = NULL;
        }
        j = v4l2_buf->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE ? v4l2_buf->num_planes : 1;
    }

    return -1;
}

int v4l2_munmap(int fd, struct v4l2_buf* v4l2_buf)
{
    int i, j;
    struct v4l2_buf_unit* buf_unit;

    for(i = 0; i < v4l2_buf->nr_bufs; ++i) {
        buf_unit = &v4l2_buf->buf[i];
        int num = v4l2_buf->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE ? v4l2_buf->num_planes : 1;
        for (j = 0; j < num; ++j) {
            munmap(buf_unit->start[j], buf_unit->length[j]);
            buf_unit->start[j]= NULL;
        }
    }

    return 0;
}

int v4l2_relbufs(int fd, struct v4l2_buf* v4l2_buf)
{
    struct v4l2_requestbuffers req;

    req.count = 0; // 让内核释放缓冲区
    req.type = v4l2_buf->type;
    req.memory = V4L2_MEMORY_MMAP;

    if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
        PLOGE << "VIDIOC_REQBUFS failed, errno: " << errno << ", " << strerror(errno);
        return -1;
    }

    return 0;
}

int v4l2_streamon(int fd, enum v4l2_buf_type type)
{
    if (ioctl(fd, VIDIOC_STREAMON, &type) < 0) {
        PLOGE << "v4l2_streamon fail, errno: " << errno << ", " << strerror(errno);
        return -1;
    }

    if (v4l2_poll(fd) < 0) {
        PLOGE << "v4l2_poll fail, errno: " << errno << ", " << strerror(errno);
        return -1;
    }


    return 0;
}

int v4l2_streamoff(int fd, enum v4l2_buf_type type)
{
    if (ioctl(fd, VIDIOC_STREAMOFF, &type) < 0) {
        PLOGE << "v4l2_streamoff fail, errno: " << errno << ", " << strerror(errno);
        return -1;
    }

    return 0;
}

int v4l2_qbuf(int fd, struct v4l2_buf_unit* buf, struct v4l2_buf *v4l2Buf)
{
    struct v4l2_buffer buffer = { 0 };
    struct v4l2_plane planes[VIDEO_MAX_PLANES];
    memset(planes, 0, sizeof(planes));

    buffer.type = v4l2Buf->type;
    buffer.memory = V4L2_MEMORY_MMAP;
    buffer.index = buf->index;

    if (v4l2Buf->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
        buffer.m.planes = planes;
        buffer.length = v4l2Buf->num_planes;
    }

    if (ioctl(fd, VIDIOC_QBUF, &buffer) < 0) {
        PLOGE << "111 vidioc qbuf fail, index = " << buf->index << " , errno = " << errno;
        return -1;
    }

    return 0;
}

int v4l2_qbuf_all(int fd, struct v4l2_buf* v4l2_buf)
{
    int i;

    for(i = 0; i < v4l2_buf->nr_bufs; ++i) {
        if (v4l2_qbuf(fd, &v4l2_buf->buf[i], v4l2_buf)) {
            PLOGE << "qbuf fail, i = " << i << " errno = " << errno;
            return -1;
        }
    }

    return 0;
}

struct v4l2_buf_unit* v4l2_dqbuf(int fd, struct v4l2_buf* v4l2_buf)
{
    struct v4l2_buffer buf = { 0 };
    struct v4l2_plane planes[MAX_PLANES];
    memset(planes, 0, sizeof(planes));

    buf.type = v4l2_buf->type;
    buf.memory = V4L2_MEMORY_MMAP;

    if (v4l2_buf->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
        buf.length = v4l2_buf->num_planes;
        buf.m.planes = planes;
    }

    if (ioctl(fd, VIDIOC_DQBUF, &buf) < 0) {
        PLOGE << "VIDIOC_DQBUF failed, errno = " << errno;
        return NULL;
    }

    int idx = buf.index;
    v4l2_buf->buf[idx].bytesused = buf.bytesused;

    return &v4l2_buf->buf[idx];
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
    streamparm->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    if (ioctl(fd, VIDIOC_G_PARM, streamparm) < 0) {
        return -1;
    }

    return 0;
}

int v4l2_s_parm(int fd, struct v4l2_streamparm *streamparm)
{
    if (ioctl(fd, VIDIOC_S_PARM, streamparm) < 0) {
        PLOGE << "VIDIOC_S_PARM failed, errno: " << errno;
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

    ret = poll(poll_fds, 1, 3000);
    if (ret < 0) {
        return -1;
    }

    if (ret == 0) {
        return -1;
    }

    return 0;
}