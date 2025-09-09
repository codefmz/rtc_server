#ifndef _V4L2_MEDIA_SOURCE_H_
#define _V4L2_MEDIA_SOURCE_H_
#include <string>
#include <queue>
#include <stdint.h>
#include <memory>
#include "MediaSource.h"
#include "V4l2.h"

class V4l2MediaSource : public MediaSource
{
public:
    V4l2MediaSource(std::shared_ptr<ThreadPool> pool, const std::string& dev);
    virtual ~V4l2MediaSource();

    void start();
protected:
    virtual void readFrame();

private:
    bool videoInit();
    bool videoExit();
    void parseH264(Frame* frame, uint8_t* h264Data, int length);

private:
    std::shared_ptr<ThreadPool> mPool;
    std::string mDev;
    int mFd;
    int mWidth;
    int mHeight;
    struct v4l2_buf* mV4l2Buf;
    struct v4l2_buf_unit* mV4l2BufUnit;
};

#endif //_V4L2_MEDIA_SOURCE_H_