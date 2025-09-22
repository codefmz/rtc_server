#ifndef _V4L2_MEDIA_SOURCE_H_
#define _V4L2_MEDIA_SOURCE_H_
#include <string>
#include <queue>
#include <stdint.h>
#include <memory>
#include "MediaSource.h"
#include "V4l2.h"
#include "unordered_map"

class V4l2MediaSource : public MediaSource
{
public:
    V4l2MediaSource(std::shared_ptr<ThreadPool> pool);
    virtual ~V4l2MediaSource();

    int init(const std::string &dev);

    int deInit();
protected:
    virtual void readFrame();

private:
    int videoInit(const std::string &dev);
    int videoExit();
    void parseH264(Frame* frame, uint8_t* h264Data, int length);

private:
    std::shared_ptr<ThreadPool> mPool;
    int mFd;
    int mWidth;
    int mHeight;
    struct v4l2_buf* mV4l2Buf;
    struct v4l2_buf_unit* mV4l2BufUnit;
    std::unordered_map<std::string, int> devFdMap;

};

#endif //_V4L2_MEDIA_SOURCE_H_