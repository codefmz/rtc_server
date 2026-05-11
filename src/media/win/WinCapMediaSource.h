#ifndef _V4L2_MEDIA_SOURCE_H_
#define _V4L2_MEDIA_SOURCE_H_
#include <string>
#include <queue>
#include <stdint.h>
#include <memory>
#include <mutex>
#include "MediaSource.h"
#include "unordered_map"

#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>

#include <mfapi.h>
#include <mfidl.h>
#include <mfobjects.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <codecapi.h>
#include <wmcodecdsp.h>

#include <d3d11.h>
#include <dxgi1_2.h>
#include <windows.graphics.capture.h>
#include <windows.graphics.capture.interop.h>
#include <windows.graphics.directx.direct3d11.interop.h>
#include <wincodec.h>
#include <Windows.h>

namespace winrt {
    using namespace winrt::Windows::Graphics::Capture;
    using namespace winrt::Windows::Graphics::DirectX;
}

struct GpuVideoProcessor {
    winrt::com_ptr<ID3D11VideoDevice> videoDevice;
    winrt::com_ptr<ID3D11VideoContext> videoContext;
    winrt::com_ptr<ID3D11VideoProcessorEnumerator> enumerator;
    winrt::com_ptr<ID3D11VideoProcessor> processor;
    winrt::com_ptr<ID3D11Texture2D> nv12Texture;
    UINT32 width = 0;
    UINT32 height = 0;
};

class WinCapMediaSource : public MediaSource {
public:
    WinCapMediaSource(std::shared_ptr<ThreadPool> pool, int fps = 30);
    virtual ~WinCapMediaSource();

    void startCapture();

protected:
    virtual void readFrame() {
    };

private:
    int videoInit();
    int videoExit();
    void parseH264(Frame* frame, uint8_t* h264Data, int length);
    /*  */
    winrt::com_ptr<IMFDXGIDeviceManager> CreateDxgiDeviceManager(ID3D11Device* device);

    /* 将 NV12 纹理写入原始 H.264 编码器 */
    void WriteNv12TextureToRawH264(IMFTransform* encoder, ID3D11Texture2D* nv12Texture, UINT64 frameIndex);

    /* 检查 HRESULT, 打印错误信息 */
    void CheckHr(HRESULT hr, const char* step);

    /* 处理 H.264 输出 */
    void ProcessH264Output(IMFTransform* encoder);

    /* 将样本字节写入文件 */
    void WriteSampleBytesToFile(IMFSample* sample);

    /* 将 NV12 纹理通过 CPU 读回写入原始 H.264 编码器 */
    void WriteNv12TextureToRawH264ByReadback(IMFTransform* encoder,
        ID3D11Device* device, ID3D11DeviceContext* context, ID3D11Texture2D* nv12Texture,
        UINT32 width, UINT32 height, UINT64 frameIndex);

    /* 创建 GPU 视频处理器 */
    GpuVideoProcessor CreateGpuVideoProcessor(ID3D11Device* device, ID3D11DeviceContext* context, UINT32 width, UINT32 height);

    /* 创建原始 H.264 编码器 */
    bool CreateRawH264Encoder(UINT32 width, UINT32 height, UINT32 fps, IMFDXGIDeviceManager* deviceManager,
        winrt::com_ptr<IMFTransform> &transform);

    /* 设置图像格式与帧率 */
    void SetVideoTypeSizeAndRate(IMFMediaType* type, UINT32 width, UINT32 height, UINT32 fps);

private:
    std::mutex mEncoderMutex;
    winrt::GraphicsCaptureSession mSession {nullptr};
    winrt::Direct3D11CaptureFramePool mFramePool{nullptr};
    HMONITOR mMonitor = nullptr;
    winrt::GraphicsCaptureItem mItem{nullptr};
    winrt::com_ptr<ID3D11Device> mD3dDevice{nullptr};
    winrt::com_ptr<ID3D11DeviceContext> mContext{nullptr};
    winrt::com_ptr<IMFTransform> mTransform{nullptr};
    winrt::Direct3D11::IDirect3DDevice mDevice{nullptr};

    uint32_t mFrameIndex = 0;
    uint64_t mStartTime = 0;
};

#endif //_V4L2_MEDIA_SOURCE_H_
