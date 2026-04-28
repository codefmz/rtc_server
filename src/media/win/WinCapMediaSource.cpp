#include <string.h>
#include <iostream>
#include <stdexcept>
#include "WinCapMediaSource.h"
#include "plog/Log.h"
#include "rtc_utils.h"

WinCapMediaSource::WinCapMediaSource(std::shared_ptr<ThreadPool> pool) : MediaSource(pool)
{
    setFps(param.fps);
    for (int i = 0; i < DEFAULT_FRAME_NUM; ++i) {
        mFrameInputQueue.push(&mFrames[i]);
    }

    mPool->createThreads();

    /* 初始化 WinRT apartment。Windows.Graphics.Capture 是 WinRT API，使用前需要先完成 apartment 初始化 */
    winrt::init_apartment();

    /* 创建 D3D11 设备 和上下文 */
    /* Windows.Graphics.Capture 产出的帧最终会通过 Direct3D 纹理传递，因此整个捕获、保存、未来编码流程都围绕同一个 D3D11 device 进行 */
    D3D11CreateDevice(
        nullptr,                    // 使用默认显卡适配器
        D3D_DRIVER_TYPE_HARDWARE,   // 使用硬件 D3D11 驱动
        nullptr,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_VIDEO_SUPPORT,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        mD3dDevice.put(),          // 输出 ID3D11Device
        nullptr,
        mContext.put()             // 输出 immediate context
    );

    /* 选择主显示器作为捕获目标， */
    mMonitor = MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTOPRIMARY);

    auto interop_factory = winrt::get_activation_factory<winrt::GraphicsCaptureItem, IGraphicsCaptureItemInterop>();
    winrt::check_hresult(interop_factory->CreateForMonitor(mMonitor, winrt::guid_of<winrt::GraphicsCaptureItem>(), winrt::put_abi(mItem)));
    auto size = mItem.Size();
    std::cout << "Capture size: " << size.Width << "x" << size.Height << std::endl;

    /* Windows.Graphics.Capture 需要WinRT 的IDirect3DDevice, 这里先从D3D11 device查询 IDXGIDevice，
    再通过 interop helper 包装成WinRT 可识别的 Direct3D device */
    winrt::com_ptr<IDXGIDevice> dxgiDevice;
    winrt::check_hresult(g_d3dDevice->QueryInterface(__uuidof(IDXGIDevice), dxgiDevice.put_void()));
    mDevice = CreateDirect3DDevice(dxgiDevice.get());

    // 创建帧池和捕获会话。
    /* CreateFreeThreaded 表示 FrameArrived 回调不依赖当前线程的 dispatcher，
    适合控制台测试程序。第二个参数指定帧格式为 32-bit BGRA，第三个参数 2
    表示帧池最多缓存 2 帧，避免回调处理稍慢时立即丢帧 */
    auto framePool = winrt::Direct3D11CaptureFramePool::CreateFreeThreaded(mDevice,
        winrt::DirectXPixelFormat::B8G8R8A8UIntNormalized, 2, size);
    auto session = framePool.CreateCaptureSession(mItem);

    constexpr UINT32 fps = 30;
    UINT32 encodedWidth = static_cast<UINT32>(size.Width) & ~1U;
    UINT32 encodedHeight = static_cast<UINT32>(size.Height) & ~1U;

    auto dxgiDeviceManager = CreateDxgiDeviceManager(mD3dDevice.get());
    auto gpuConverter = CreateGpuVideoProcessor(mDevice.get(), mContext.get(), encodedWidth, encodedHeight);
    winrt::com_ptr<IMFTransform> transform;
    auto isSupportDxgiInput = CreateRawH264Encoder(encodedWidth, encodedHeight, fps, dxgiDeviceManager.get(), transform);
    std::mutex encoderMutex;
    UINT64 frameIndex = 0;

    /* 每当系统捕获到新帧时触发。这里用 D3D11 VideoProcessor 在 GPU 上把 BGRA
        捕获帧转成 NV12，再把 NV12 DXGI surface 直接交给 H.264 Encoder MFT。*/
    framePool.FrameArrived([&](auto& sender, auto&) {
        // TryGetNextFrame 取出帧池中的下一帧；frame 持有捕获表面和时间戳等信息。
        auto frame = sender.TryGetNextFrame();
        auto surface = frame.Surface();

        // WinRT surface 本质上包着 DXGI/D3D 资源，这里取回底层 ID3D11Texture2D，
        // 方便使用 D3D11 API 复制、读取或交给编码器。
        auto tex = GetDXGIInterfaceFromObject<ID3D11Texture2D>(surface);

        std::lock_guard lock(encoderMutex);
        ID3D11Texture2D* nv12Texture = ConvertBgraToNv12Gpu(gpuConverter, tex.get());
        if (encoder.supportsDxgiInput) {
            WriteNv12TextureToRawH264(encoder.transform.get(), nv12Texture, frameIndex++, fps);
        } else {
            // WriteNv12TextureToRawH264ByReadback(
            //     encoder.transform.get(),
            //     h264File,
            //     g_d3dDevice.get(),
            //     g_context.get(),
            //     nv12Texture,
            //     encodedWidth,
            //     encodedHeight,
            //     frameIndex++,
            //     fps);
        }
    });

    // 开始捕获。之后 FrameArrived 会在后台线程中持续触发，直到用户按 Enter。
    session.StartCapture();
    std::wcout << L"Capturing raw H.264 to " << filename << L"... Press Enter to stop\n";
    std::cin.get();

    // 显式关闭会话和帧池，释放系统捕获资源和 D3D 纹理缓存。
    session.Close();
    framePool.Close();

    {
        std::lock_guard lock(encoderMutex);
        DrainRawH264Encoder(encoder.transform.get(), h264File);
        fclose(h264File);
        h264File = nullptr;
    }

    // 与 MFStartup 成对调用，释放 Media Foundation 全局资源。
    MFShutdown();

}

WinCapMediaSource::~WinCapMediaSource()
{
    mPool->cancelThreads();
    videoExit();
}

winrt::com_ptr<IMFDXGIDeviceManager> WinCapMediaSource::CreateDxgiDeviceManager(ID3D11Device* device)
{
    UINT resetToken = 0;
    winrt::com_ptr<IMFDXGIDeviceManager> deviceManager;
    CheckHr(MFCreateDXGIDeviceManager(&resetToken, deviceManager.put()), "MFCreateDXGIDeviceManager");
    CheckHr(deviceManager->ResetDevice(device, resetToken), "IMFDXGIDeviceManager::ResetDevice");
    return deviceManager;
}

GpuVideoProcessor WinCapMediaSource::CreateGpuVideoProcessor(ID3D11Device* device, ID3D11DeviceContext* context, UINT32 width, UINT32 height)
{
    GpuVideoProcessor converter{};
    converter.width = width;
    converter.height = height;

    CheckHr(device->QueryInterface(__uuidof(ID3D11VideoDevice), converter.videoDevice.put_void()),
        "ID3D11Device::QueryInterface(ID3D11VideoDevice)");
    CheckHr(context->QueryInterface(__uuidof(ID3D11VideoContext), converter.videoContext.put_void()),
        "ID3D11DeviceContext::QueryInterface(ID3D11VideoContext)");

    D3D11_VIDEO_PROCESSOR_CONTENT_DESC contentDesc{};
    contentDesc.InputFrameFormat = D3D11_VIDEO_FRAME_FORMAT_PROGRESSIVE;
    contentDesc.InputWidth = width;
    contentDesc.InputHeight = height;
    contentDesc.OutputWidth = width;
    contentDesc.OutputHeight = height;
    contentDesc.Usage = D3D11_VIDEO_USAGE_PLAYBACK_NORMAL;

    CheckHr(converter.videoDevice->CreateVideoProcessorEnumerator(&contentDesc, converter.enumerator.put()),
        "ID3D11VideoDevice::CreateVideoProcessorEnumerator");
    CheckHr(converter.videoDevice->CreateVideoProcessor(converter.enumerator.get(), 0, converter.processor.put()),
        "ID3D11VideoDevice::CreateVideoProcessor");

    UINT flags = 0;
    CheckHr(converter.enumerator->CheckVideoProcessorFormat(DXGI_FORMAT_B8G8R8A8_UNORM, &flags),
        "ID3D11VideoProcessorEnumerator::CheckVideoProcessorFormat(BGRA)");
    if ((flags & D3D11_VIDEO_PROCESSOR_FORMAT_SUPPORT_INPUT) == 0) {
        throw std::runtime_error("D3D11 VideoProcessor does not support BGRA input");
    }

    CheckHr(converter.enumerator->CheckVideoProcessorFormat(DXGI_FORMAT_NV12, &flags),
        "ID3D11VideoProcessorEnumerator::CheckVideoProcessorFormat(NV12)");
    if ((flags & D3D11_VIDEO_PROCESSOR_FORMAT_SUPPORT_OUTPUT) == 0) {
        throw std::runtime_error("D3D11 VideoProcessor does not support NV12 output");
    }

    D3D11_TEXTURE2D_DESC nv12Desc{};
    nv12Desc.Width = width;
    nv12Desc.Height = height;
    nv12Desc.MipLevels = 1;
    nv12Desc.ArraySize = 1;
    nv12Desc.Format = DXGI_FORMAT_NV12;
    nv12Desc.SampleDesc.Count = 1;
    nv12Desc.Usage = D3D11_USAGE_DEFAULT;
    nv12Desc.BindFlags = D3D11_BIND_RENDER_TARGET;

    CheckHr(device->CreateTexture2D(&nv12Desc, nullptr, converter.nv12Texture.put()),
        "ID3D11Device::CreateTexture2D(NV12)");

    RECT rect{ 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
    converter.videoContext->VideoProcessorSetOutputTargetRect(converter.processor.get(), TRUE, &rect);
    converter.videoContext->VideoProcessorSetStreamSourceRect(converter.processor.get(), 0, TRUE, &rect);
    converter.videoContext->VideoProcessorSetStreamDestRect(converter.processor.get(), 0, TRUE, &rect);

    return converter;
}

bool WinCapMediaSource::CreateRawH264Encoder(UINT32 width, UINT32 height, UINT32 fps, IMFDXGIDeviceManager* deviceManager, winrt::com_ptr<IMFTransform> &transform)
{
    bool isSupportDxgiInput = false;

    winrt::com_ptr<IMFTransform> encoder;
    CheckHr(CoCreateInstance(CLSID_CMSH264EncoderMFT, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(encoder.put())),
        "CoCreateInstance(CLSID_CMSH264EncoderMFT)");

    /* 把 D3D11 device manager 交给编码器后，输入样本可以直接携带 DXGI surface，
       避免先把 NV12 纹理读回 CPU 再重新上传给硬件编码器。
       不是所有系统 H.264 Encoder MFT 都实现这条 D3D11 输入路径；如果返回
       E_NOTIMPL，后面仍然使用 GPU 做 BGRA->NV12，只把 NV12 结果读回 CPU。*/
    HRESULT setManagerHr = encoder->ProcessMessage(MFT_MESSAGE_SET_D3D_MANAGER, reinterpret_cast<ULONG_PTR>(deviceManager));
    if (setManagerHr == S_OK) {
        isSupportDxgiInput = true;
    } else if (setManagerHr == E_NOTIMPL) {
        isSupportDxgiInput = false;
        std::cout << "H.264 Encoder MFT does not support DXGI input; fallback to NV12 readback.\n";
    } else {
        CheckHr(setManagerHr, "IMFTransform::ProcessMessage(SET_D3D_MANAGER)");
    }

    winrt::com_ptr<IMFMediaType> outputType;
    CheckHr(MFCreateMediaType(outputType.put()), "MFCreateMediaType(raw h264 output)");
    CheckHr(outputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video), "raw output SetGUID(MF_MT_MAJOR_TYPE)");
    CheckHr(outputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264), "raw output SetGUID(MF_MT_SUBTYPE H264)");
    CheckHr(outputType->SetUINT32(MF_MT_AVG_BITRATE, 8'000'000), "raw output SetUINT32(MF_MT_AVG_BITRATE)");
    CheckHr(outputType->SetUINT32(MF_MT_MPEG2_PROFILE, eAVEncH264VProfile_Main), "raw output SetUINT32(MF_MT_MPEG2_PROFILE)");
    SetVideoTypeSizeAndRate(outputType.get(), width, height, fps);
    CheckHr(encoder->SetOutputType(0, outputType.get(), 0), "IMFTransform::SetOutputType(H264)");

    winrt::com_ptr<IMFMediaType> inputType;
    CheckHr(MFCreateMediaType(inputType.put()), "MFCreateMediaType(raw h264 input)");
    CheckHr(inputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video), "raw input SetGUID(MF_MT_MAJOR_TYPE)");
    CheckHr(inputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12), "raw input SetGUID(MF_MT_SUBTYPE NV12)");
    SetVideoTypeSizeAndRate(inputType.get(), width, height, fps);
    CheckHr(encoder->SetInputType(0, inputType.get(), 0), "IMFTransform::SetInputType(NV12)");

    // 新创建并刚设置好媒体类型的编码器还没有积压输入数据，不需要 flush。
    // 某些系统自带 H.264 Encoder MFT 在这个状态下收到 COMMAND_FLUSH 会返回 E_FAIL。
    CheckHr(encoder->ProcessMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, 0), "IMFTransform::ProcessMessage(NOTIFY_BEGIN_STREAMING)");
    CheckHr(encoder->ProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, 0), "IMFTransform::ProcessMessage(NOTIFY_START_OF_STREAM)");

    transform = std::move(encoder);
    return isSupportDxgiInput;
}

void WinCapMediaSource::WriteNv12TextureToRawH264(IMFTransform* encoder, ID3D11Texture2D* nv12Texture, UINT64 frameIndex, UINT32 fps)
{
    winrt::com_ptr<IMFMediaBuffer> buffer;
    CheckHr(MFCreateDXGISurfaceBuffer(__uuidof(ID3D11Texture2D), nv12Texture, 0, FALSE, buffer.put()),
        "MFCreateDXGISurfaceBuffer(NV12)");

    winrt::com_ptr<IMFSample> sample;
    CheckHr(MFCreateSample(sample.put()), "MFCreateSample(DXGI input)");
    CheckHr(sample->AddBuffer(buffer.get()), "IMFSample::AddBuffer(DXGI input)");

    LONGLONG sampleDuration = 10'000'000LL / fps;
    CheckHr(sample->SetSampleTime(frameIndex * sampleDuration), "IMFSample::SetSampleTime(DXGI input)");
    CheckHr(sample->SetSampleDuration(sampleDuration), "IMFSample::SetSampleDuration(DXGI input)");

    HRESULT hr = encoder->ProcessInput(0, sample.get(), 0);
    if (hr == MF_E_NOTACCEPTING) {
        ProcessH264Output(encoder);
        hr = encoder->ProcessInput(0, sample.get(), 0);
    }
    CheckHr(hr, "IMFTransform::ProcessInput(DXGI)");

    ProcessH264Output(encoder);
}

// void WriteSampleBytesToFile(IMFSample* sample, FILE* file)
// {
//     winrt::com_ptr<IMFMediaBuffer> buffer;
//     CheckHr(sample->ConvertToContiguousBuffer(buffer.put()), "IMFSample::ConvertToContiguousBuffer");

//     BYTE* data = nullptr;
//     DWORD maxLength = 0;
//     DWORD currentLength = 0;
//     CheckHr(buffer->Lock(&data, &maxLength, &currentLength), "IMFMediaBuffer::Lock(output)");

//     if (currentLength > 0) {
//         fwrite(data, 1, currentLength, file);
//     }

//     CheckHr(buffer->Unlock(), "IMFMediaBuffer::Unlock(output)");
// }

void WinCapMediaSource::ProcessH264Output(IMFTransform* encoder)
{
    MFT_OUTPUT_STREAM_INFO streamInfo{};
    CheckHr(encoder->GetOutputStreamInfo(0, &streamInfo), "IMFTransform::GetOutputStreamInfo");

    while (true) {
        winrt::com_ptr<IMFSample> outputSample;
        MFT_OUTPUT_DATA_BUFFER output{};
        output.dwStreamID = 0;

        if ((streamInfo.dwFlags & MFT_OUTPUT_STREAM_PROVIDES_SAMPLES) == 0) {
            winrt::com_ptr<IMFMediaBuffer> outputBuffer;
            CheckHr(MFCreateMemoryBuffer(streamInfo.cbSize, outputBuffer.put()), "MFCreateMemoryBuffer(output)");
            CheckHr(MFCreateSample(outputSample.put()), "MFCreateSample(output)");
            CheckHr(outputSample->AddBuffer(outputBuffer.get()), "IMFSample::AddBuffer(output)");
            output.pSample = outputSample.get();
        }

        DWORD status = 0;
        HRESULT hr = encoder->ProcessOutput(0, 1, &output, &status);

        if (output.pEvents != nullptr) {
            output.pEvents->Release();
        }

        if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) {
            break;
        }

        if (hr == MF_E_TRANSFORM_STREAM_CHANGE) {
            winrt::com_ptr<IMFMediaType> changedOutputType;
            CheckHr(encoder->GetOutputAvailableType(0, 0, changedOutputType.put()), "IMFTransform::GetOutputAvailableType(stream change)");
            CheckHr(encoder->SetOutputType(0, changedOutputType.get(), 0), "IMFTransform::SetOutputType(stream change)");
            continue;
        }

        CheckHr(hr, "IMFTransform::ProcessOutput");

        if (output.pSample != nullptr) {
            WriteSampleBytesToFile(output.pSample, file);
        }
    }
}

void WriteTextureToRawH264(
    IMFTransform* encoder,
    FILE* file,
    ID3D11Device* device,
    ID3D11DeviceContext* context,
    ID3D11Texture2D* srcTexture,
    UINT32 width,
    UINT32 height,
    UINT64 frameIndex,
    UINT32 fps)
{
    D3D11_TEXTURE2D_DESC desc{};
    srcTexture->GetDesc(&desc);

    D3D11_TEXTURE2D_DESC stagingDesc = desc;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    stagingDesc.MiscFlags = 0;

    winrt::com_ptr<ID3D11Texture2D> staging;
    CheckHr(device->CreateTexture2D(&stagingDesc, nullptr, staging.put()), "ID3D11Device::CreateTexture2D(staging)");

    context->CopyResource(staging.get(), srcTexture);

    D3D11_MAPPED_SUBRESOURCE mapped{};
    CheckHr(context->Map(staging.get(), 0, D3D11_MAP_READ, 0, &mapped), "ID3D11DeviceContext::Map(staging)");

    DWORD bufferSize = width * height * 3 / 2;
    winrt::com_ptr<IMFMediaBuffer> buffer;
    CheckHr(MFCreateMemoryBuffer(bufferSize, buffer.put()), "MFCreateMemoryBuffer(NV12)");

    BYTE* destination = nullptr;
    DWORD maxLength = 0;
    DWORD currentLength = 0;
    CheckHr(buffer->Lock(&destination, &maxLength, &currentLength), "IMFMediaBuffer::Lock(NV12)");
    ConvertBgraToNv12(destination, width, height, reinterpret_cast<BYTE*>(mapped.pData), mapped.RowPitch);
    CheckHr(buffer->Unlock(), "IMFMediaBuffer::Unlock(NV12)");
    CheckHr(buffer->SetCurrentLength(bufferSize), "IMFMediaBuffer::SetCurrentLength(NV12)");

    context->Unmap(staging.get(), 0);

    winrt::com_ptr<IMFSample> sample;
    CheckHr(MFCreateSample(sample.put()), "MFCreateSample(input)");
    CheckHr(sample->AddBuffer(buffer.get()), "IMFSample::AddBuffer(input)");

    LONGLONG sampleDuration = 10'000'000LL / fps;
    CheckHr(sample->SetSampleTime(frameIndex * sampleDuration), "IMFSample::SetSampleTime(input)");
    CheckHr(sample->SetSampleDuration(sampleDuration), "IMFSample::SetSampleDuration(input)");

    HRESULT hr = encoder->ProcessInput(0, sample.get(), 0);
    if (hr == MF_E_NOTACCEPTING) {
        ProcessH264Output(encoder, file);
        hr = encoder->ProcessInput(0, sample.get(), 0);
    }
    CheckHr(hr, "IMFTransform::ProcessInput");

    ProcessH264Output(encoder, file);
}

int WinCapMediaSource::init(const std::string &dev)
{

}

int WinCapMediaSource::deInit()
{
    setFd(-1);
    return 0;
}

// void WinCapMediaSource::readFrame()
// {
//     std::lock_guard<std::mutex> lock(mMutex);
//     if (mFrameInputQueue.empty()) {
//         PLOGE << "V4l2MediaSource::readFrame mAVFrameInputQueue.empty()";
//         return;
//     }

//     Frame* frame = mFrameInputQueue.front();




//     mFrameInputQueue.pop();
//     mFrameOutputQueue.push(frame);
// }

int WinCapMediaSource::videoInit(const std::string &dev, int &fd)
{

}

int WinCapMediaSource::videoExit()
{
    int ret;

    return ret;
}

/* 将h264 数据解析为单个NALU单元， Webrtc 只支持传送NALU 单元 */
void WinCapMediaSource::parseH264(Frame* frame, uint8_t *h264Data, int length)
{
    int i = 0;
    int before = 0;
    int num = 0;
    int offset = 0;

    if (startCode4(h264Data)) {
        offset = 4;
    } else if (startCode3(h264Data)) {
        offset = 3;
    }

    while (i + offset <= length) {
        if (h264Data[i] != 0) {
            i++;
            continue;
        }

        if (offset == 3) {
            if (h264Data[i + 1] == 0 && h264Data[i + 2] == 1) {
                if (i - before - offset > 0 && i - before - offset <= H264_FRAME_MAX_NALU_SIZE && num < H264_FRAME_MAX_NALU_NUM) {
                    memcpy(frame->mBufferArr[num].data(), h264Data + before + offset, i - before - offset);
                    frame->mSizeArr.push_back(i - before - offset);
                    num++;
                } else {
                    PLOGE << "h264Data error, length = " << length << " i - before - offset = "  << i - before - offset << " num = " << num;
                }
                before = i;
            }
        } else if (offset == 4) {
            if (h264Data[i + 1] == 0 && h264Data[i + 2] == 0 && h264Data[i + 3] == 1) {
                if (i - before - offset > 0 && i - before - offset <= H264_FRAME_MAX_NALU_SIZE && num < H264_FRAME_MAX_NALU_NUM) {
                    memcpy(frame->mBufferArr[num].data(), h264Data + before + offset, i - before - offset);
                    frame->mSizeArr.push_back(i - before - offset);
                    num++;
                } else if (num != 0){
                    PLOGE << "h264Data error, length = " << length << " i - before - offset = "  << i - before - offset << " num = " << num;
                }
                before = i;
            }
        }
        i++;
    }

    if (i - before > 0 && i - before - offset <= H264_FRAME_MAX_NALU_SIZE && num < H264_FRAME_MAX_NALU_NUM) {
        memcpy(frame->mBufferArr[num].data(), h264Data + before + offset, i - before);
        frame->mSizeArr.push_back(i - before);
        num++;
    }  else if (num != 0){
        PLOGE << "h264Data error, length = " << length << " i - before - offset = "  << i - before - offset << " num = " << num;
    }
}

void WinCapMediaSource::setFd(int fd)
{
    std::lock_guard<std::mutex> lock(mMutex);
    mFd = fd;
}
