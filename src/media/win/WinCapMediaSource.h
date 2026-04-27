#ifndef _V4L2_MEDIA_SOURCE_H_
#define _V4L2_MEDIA_SOURCE_H_
#include <string>
#include <queue>
#include <stdint.h>
#include <memory>
#include "MediaSource.h"
#include "V4l2.h"
#include "unordered_map"
#include "Encoder.h"

class WinCapMediaSourceParam {
public:
    int width;
    int height;
    int fps;
    int driverType;
    int fmt;
};

class WinCapMediaSource : public MediaSource {
public:
    WinCapMediaSource(std::shared_ptr<ThreadPool> pool, WinCapMediaSourceParam &param);
    virtual ~WinCapMediaSource();

    int init(const std::string &dev);

    int deInit();

protected:
    virtual void readFrame();

private:
    int videoInit(const std::string &dev, int &fd);
    int videoExit();
    void parseH264(Frame* frame, uint8_t* h264Data, int length);
    void setFd(int fd);

private:
    int mFd = -1;
    int mWidth;
    int mHeight;
    int mDriverType;
    int mFmt;
    struct v4l2_buf* mV4l2Buf;
    struct v4l2_buf_unit* mV4l2BufUnit;
    std::unordered_map<std::string, int> devFdMap;
    std::unordered_map<int, struct v4l2_buf*> mFdV4l2BufMap;
    std::unique_ptr<Encoder> mEncoder;
};

#endif //_V4L2_MEDIA_SOURCE_H_