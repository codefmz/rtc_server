#ifndef _V4L2_MEDIA_SOURCE_H_
#define _V4L2_MEDIA_SOURCE_H_
#include <string>
#include <queue>
#include <stdint.h>
#include <memory>
#include "MediaSource.h"
#include "unordered_map"
#include "Encoder.h"

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
    WinCapMediaSource(std::shared_ptr<ThreadPool> pool);
    virtual ~WinCapMediaSource();

    int init();

    int deInit();

protected:
    virtual void readFrame();

private:
    int videoInit();
    int videoExit();
    void parseH264(Frame* frame, uint8_t* h264Data, int length);
    void setFd(int fd);
    winrt::com_ptr<IMFDXGIDeviceManager> CreateDxgiDeviceManager(ID3D11Device* device);

private:
    HMONITOR mMonitor;
    winrt::GraphicsCaptureItem mItem;
    winrt::com_ptr<ID3D11Device> mD3dDevice;
    winrt::com_ptr<ID3D11DeviceContext> mContext;
    winrt::Direct3D11::IDirect3DDevice mDevice;
};

#endif //_V4L2_MEDIA_SOURCE_H_