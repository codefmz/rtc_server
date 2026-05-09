#include <string.h>
#include <iostream>
#include <stdexcept>
#include "WinCapMediaSource.h"
#include "plog/Log.h"
#include "rtc_utils.h"

namespace {

winrt::Direct3D11::IDirect3DDevice CreateDirect3DDevice(IDXGIDevice* dxgiDevice)
{
    winrt::com_ptr<::IInspectable> device;
    winrt::check_hresult(CreateDirect3D11DeviceFromDXGIDevice(dxgiDevice, device.put()));
    return device.as<winrt::Direct3D11::IDirect3DDevice>();
}

template <typename T>
winrt::com_ptr<T> GetDXGIInterfaceFromObject(winrt::Direct3D11::IDirect3DSurface const& surface)
{
    auto access = surface.as<::Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>();
    winrt::com_ptr<T> result;
    winrt::check_hresult(access->GetInterface(winrt::guid_of<T>(), result.put_void()));
    return result;
}

ID3D11Texture2D* ConvertBgraToNv12Gpu(GpuVideoProcessor& converter, ID3D11Texture2D* bgraTexture)
{
    winrt::com_ptr<ID3D11VideoProcessorInputView> inputView;
    D3D11_VIDEO_PROCESSOR_INPUT_VIEW_DESC inputDesc{};
    inputDesc.ViewDimension = D3D11_VPIV_DIMENSION_TEXTURE2D;
    inputDesc.Texture2D.MipSlice = 0;
    inputDesc.Texture2D.ArraySlice = 0;
    winrt::check_hresult(converter.videoDevice->CreateVideoProcessorInputView(
        bgraTexture, converter.enumerator.get(), &inputDesc, inputView.put()));

    winrt::com_ptr<ID3D11VideoProcessorOutputView> outputView;
    D3D11_VIDEO_PROCESSOR_OUTPUT_VIEW_DESC outputDesc{};
    outputDesc.ViewDimension = D3D11_VPOV_DIMENSION_TEXTURE2D;
    outputDesc.Texture2D.MipSlice = 0;
    winrt::check_hresult(converter.videoDevice->CreateVideoProcessorOutputView(
        converter.nv12Texture.get(), converter.enumerator.get(), &outputDesc, outputView.put()));

    D3D11_VIDEO_PROCESSOR_STREAM stream{};
    stream.Enable = TRUE;
    stream.pInputSurface = inputView.get();
    winrt::check_hresult(converter.videoContext->VideoProcessorBlt(
        converter.processor.get(), outputView.get(), 0, 1, &stream));

    return converter.nv12Texture.get();
}

} // namespace

WinCapMediaSource::WinCapMediaSource(std::shared_ptr<ThreadPool> pool, int fps) : MediaSource(pool)
{
    setFps(fps);
    for (int i = 0; i < DEFAULT_FRAME_NUM; ++i) {
        mFrameInputQueue.push(&mFrames[i]);
    }

    videoInit();
}

WinCapMediaSource::~WinCapMediaSource()
{
    videoExit();
}

void WinCapMediaSource::startCapture()
{
    mSession.StartCapture();
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

void WinCapMediaSource::SetVideoTypeSizeAndRate(IMFMediaType* type, UINT32 width, UINT32 height, UINT32 fps)
{
    CheckHr(MFSetAttributeSize(type, MF_MT_FRAME_SIZE, width, height), "MFSetAttributeSize(MF_MT_FRAME_SIZE)");
    CheckHr(MFSetAttributeRatio(type, MF_MT_FRAME_RATE, fps, 1), "MFSetAttributeRatio(MF_MT_FRAME_RATE)");
    CheckHr(MFSetAttributeRatio(type, MF_MT_PIXEL_ASPECT_RATIO, 1, 1), "MFSetAttributeRatio(MF_MT_PIXEL_ASPECT_RATIO)");
    CheckHr(type->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive), "SetUINT32(MF_MT_INTERLACE_MODE)");
}


bool WinCapMediaSource::CreateRawH264Encoder(UINT32 width, UINT32 height, UINT32 fps, IMFDXGIDeviceManager* deviceManager, winrt::com_ptr<IMFTransform> &transform)
{
    bool isSupportDxgiInput = false;

    winrt::com_ptr<IMFTransform> encoder;
    CheckHr(CoCreateInstance(CLSID_CMSH264EncoderMFT, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(encoder.put())),
        "CoCreateInstance(CLSID_CMSH264EncoderMFT)");

    // Give the D3D11 device manager to the encoder so input samples can carry
    // DXGI surfaces directly. If the encoder does not support it, fall back to
    // GPU BGRA-to-NV12 conversion plus CPU readback.
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

void WinCapMediaSource::CheckHr(HRESULT hr, const char* step)
{
    if (FAILED(hr)) {
        char message[256]{};
        sprintf_s(message, "%s failed, HRESULT=0x%08X", step, static_cast<unsigned int>(hr));
        throw std::runtime_error(message);
    }
}

void WinCapMediaSource::WriteNv12TextureToRawH264(IMFTransform* encoder, ID3D11Texture2D* nv12Texture, UINT64 frameIndex)
{
    winrt::com_ptr<IMFMediaBuffer> buffer;
    /* 直接把 NV12 的 D3D11 纹理包装成 MFMediaBuffer，避免读回 CPU */
    CheckHr(MFCreateDXGISurfaceBuffer(__uuidof(ID3D11Texture2D), nv12Texture, 0, FALSE, buffer.put()),
        "MFCreateDXGISurfaceBuffer(NV12)");

    winrt::com_ptr<IMFSample> sample;
    /* 创建输入样本 */
    CheckHr(MFCreateSample(sample.put()), "MFCreateSample(DXGI input)");
    /* 样本携带 NV12 纹理的 GPU buffer */
    CheckHr(sample->AddBuffer(buffer.get()), "IMFSample::AddBuffer(DXGI input)");

    LONGLONG sampleDuration = 10'000'000LL / mFps;
    /* 设置样本时间戳和持续时间，单位是 100 纳秒 */
    CheckHr(sample->SetSampleTime(frameIndex * sampleDuration), "IMFSample::SetSampleTime(DXGI input)");
    CheckHr(sample->SetSampleDuration(sampleDuration), "IMFSample::SetSampleDuration(DXGI input)");

    /* 处理输入样本 */
    HRESULT hr = encoder->ProcessInput(0, sample.get(), 0);
    while (hr == MF_E_NOTACCEPTING) { // 编码器输入队列满了，先处理一下输出
        ProcessH264Output(encoder);
        hr = encoder->ProcessInput(0, sample.get(), 0);
    }
    CheckHr(hr, "IMFTransform::ProcessInput(DXGI)");

    ProcessH264Output(encoder);
}

void WinCapMediaSource::WriteSampleBytesToFile(IMFSample* sample)
{
    winrt::com_ptr<IMFMediaBuffer> buffer;
    /* 将样本转换为连续缓冲区 */
    CheckHr(sample->ConvertToContiguousBuffer(buffer.put()), "IMFSample::ConvertToContiguousBuffer");

    BYTE* data = nullptr;
    DWORD maxLength = 0;
    DWORD currentLength = 0;
    /* 锁定缓冲区，获取指向编码后 H.264 数据的指针和数据长度 */
    CheckHr(buffer->Lock(&data, &maxLength, &currentLength), "IMFMediaBuffer::Lock(output)");

    if (currentLength <= 0) {
        CheckHr(buffer->Unlock(), "IMFMediaBuffer::Unlock(output)");
        return;
    }

    Utils::dumpToFile("test.264", data, currentLength);

    std::lock_guard<std::mutex> lock(mMutex);
    if (mFrameInputQueue.empty()) {
        PLOGE << "V4l2MediaSource::readFrame mAVFrameInputQueue.empty()";
        CheckHr(buffer->Unlock(), "IMFMediaBuffer::Unlock(output)");
        return;
    }

    Frame* frame = mFrameInputQueue.front();
    parseH264(frame, data, currentLength);
    mFrameInputQueue.pop();
    mFrameOutputQueue.push(frame);
    CheckHr(buffer->Unlock(), "IMFMediaBuffer::Unlock(output)");
}

void WinCapMediaSource::ProcessH264Output(IMFTransform* encoder)
{
    MFT_OUTPUT_STREAM_INFO streamInfo{};
    /* 查询输出流信息，了解编码器输出缓冲区的要求 */
    CheckHr(encoder->GetOutputStreamInfo(0, &streamInfo), "IMFTransform::GetOutputStreamInfo");

    while (true) {
        winrt::com_ptr<IMFSample> outputSample;
        MFT_OUTPUT_DATA_BUFFER output{};
        output.dwStreamID = 0;

        /* 如果输出流不提供样本，则需要手动创建输出缓冲区 */
        if ((streamInfo.dwFlags & MFT_OUTPUT_STREAM_PROVIDES_SAMPLES) == 0) {
            winrt::com_ptr<IMFMediaBuffer> outputBuffer;
            /* 创建一个符合编码器要求大小的输出缓冲区 */
            CheckHr(MFCreateMemoryBuffer(streamInfo.cbSize, outputBuffer.put()), "MFCreateMemoryBuffer(output)");
            /* 创建输出样本 */
            CheckHr(MFCreateSample(outputSample.put()), "MFCreateSample(output)");
            /* 样本携带输出缓冲区 */
            CheckHr(outputSample->AddBuffer(outputBuffer.get()), "IMFSample::AddBuffer(output)");
            /* 设置输出样本 */
            output.pSample = outputSample.get();
        }

        DWORD status = 0;
        HRESULT hr = encoder->ProcessOutput(0, 1, &output, &status);
        /* 处理完输出样本后，编码器会在 output 结构里返回一些状态信息，调用者需要根据这些信息判断下一步怎么做 */
        if (output.pEvents != nullptr) {
            output.pEvents->Release();
        }
        /* 根据 HRESULT 的值进行不同的处理, 需要更多输入 */
        if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) {
            break;
        }

        /* 某些编码器在输出格式发生变化时会返回 MF_E_TRANSFORM_STREAM_CHANGE，这时需要重新查询输出流信息，调整输出缓冲区大小 */
        if (hr == MF_E_TRANSFORM_STREAM_CHANGE) {
            winrt::com_ptr<IMFMediaType> changedOutputType;
            CheckHr(encoder->GetOutputAvailableType(0, 0, changedOutputType.put()), "IMFTransform::GetOutputAvailableType(stream change)");
            CheckHr(encoder->SetOutputType(0, changedOutputType.get(), 0), "IMFTransform::SetOutputType(stream change)");
            continue;
        }

        CheckHr(hr, "IMFTransform::ProcessOutput");

        if (output.pSample != nullptr) {
            WriteSampleBytesToFile(output.pSample);
        }
    }
}

int WinCapMediaSource::videoInit()
{
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
    winrt::check_hresult(mD3dDevice->QueryInterface(__uuidof(IDXGIDevice), dxgiDevice.put_void()));
    mDevice = CreateDirect3DDevice(dxgiDevice.get());

    // 创建帧池和捕获会话。
    /* CreateFreeThreaded 表示 FrameArrived 回调不依赖当前线程的 dispatcher，
    适合控制台测试程序。第二个参数指定帧格式为 32-bit BGRA，第三个参数 2
    表示帧池最多缓存 2 帧，避免回调处理稍慢时立即丢帧 */
    mFramePool = winrt::Direct3D11CaptureFramePool::CreateFreeThreaded(mDevice,
        winrt::DirectXPixelFormat::B8G8R8A8UIntNormalized, 2, size);
    mSession = mFramePool.CreateCaptureSession(mItem);

    UINT32 encodedWidth = static_cast<UINT32>(size.Width) & ~1U;
    UINT32 encodedHeight = static_cast<UINT32>(size.Height) & ~1U;

    auto dxgiDeviceManager = CreateDxgiDeviceManager(mD3dDevice.get());
    auto gpuConverter = CreateGpuVideoProcessor(mD3dDevice.get(), mContext.get(), encodedWidth, encodedHeight);
    auto isSupportDxgiInput = CreateRawH264Encoder(encodedWidth, encodedHeight, mFps, dxgiDeviceManager.get(), mTransform);
    UINT64 frameIndex = 0;

    // Convert each captured BGRA frame to NV12 on the GPU, then pass the NV12
    // DXGI surface to the H.264 encoder when supported.
    mFramePool.FrameArrived([this, gpuConverter, isSupportDxgiInput, encodedWidth, encodedHeight, frameIndex = UINT64{ 0 }](auto& sender, auto&) mutable {
        // TryGetNextFrame 取出帧池中的下一帧；frame 持有捕获表面和时间戳等信息。
        auto frame = sender.TryGetNextFrame();
        auto surface = frame.Surface();

        // WinRT surface 本质上包着 DXGI/D3D 资源，这里取回底层 ID3D11Texture2D，
        // 方便使用 D3D11 API 复制、读取或交给编码器。
        auto tex = GetDXGIInterfaceFromObject<ID3D11Texture2D>(surface);

        std::lock_guard lock(mEncoderMutex);
        ID3D11Texture2D* nv12Texture = ConvertBgraToNv12Gpu(gpuConverter, tex.get());
        if (isSupportDxgiInput) {
            WriteNv12TextureToRawH264(mTransform.get(), nv12Texture, frameIndex++);
        } else {
            WriteNv12TextureToRawH264ByReadback(mTransform.get(),
                mD3dDevice.get(), mContext.get(), nv12Texture, encodedWidth,
                encodedHeight, frameIndex++);
        }
    });

    return 0;
}

void WinCapMediaSource::WriteNv12TextureToRawH264ByReadback(IMFTransform* encoder,
    ID3D11Device* device, ID3D11DeviceContext* context, ID3D11Texture2D* nv12Texture,
    UINT32 width, UINT32 height, UINT64 frameIndex)
{
    D3D11_TEXTURE2D_DESC desc{};
    /* 获取 NV12 纹理的描述 */
    nv12Texture->GetDesc(&desc);

    /* 创建 staging 纹理描述 */
    D3D11_TEXTURE2D_DESC stagingDesc = desc;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    stagingDesc.MiscFlags = 0;

    winrt::com_ptr<ID3D11Texture2D> staging;
    /* 创建 staging 纹理 */
    CheckHr(device->CreateTexture2D(&stagingDesc, nullptr, staging.put()), "ID3D11Device::CreateTexture2D(NV12 staging)");

    /* 把 NV12 纹理数据复制到 staging 纹理，准备读回 CPU */
    context->CopyResource(staging.get(), nv12Texture);

    D3D11_MAPPED_SUBRESOURCE mapped{};
    /* 映射 staging 纹理，获取指向 NV12 数据的 CPU 可读指针 */
    CheckHr(context->Map(staging.get(), 0, D3D11_MAP_READ, 0, &mapped), "ID3D11DeviceContext::Map(NV12 staging)");

    /* 计算 NV12 数据的大小 */
    DWORD ySize = width * height;
    DWORD uvSize = width * height / 2;
    DWORD bufferSize = ySize + uvSize;

    winrt::com_ptr<IMFMediaBuffer> buffer;
    /* 创建一个 MFMediaBuffer 来存放 NV12 数据，准备交给编码器 */
    CheckHr(MFCreateMemoryBuffer(bufferSize, buffer.put()), "MFCreateMemoryBuffer(NV12 readback)");

    BYTE* destination = nullptr;
    DWORD maxLength = 0;
    DWORD currentLength = 0;
    /* 锁定 MFMediaBuffer，获取指向缓冲区的指针和长度 */
    CheckHr(buffer->Lock(&destination, &maxLength, &currentLength), "IMFMediaBuffer::Lock(NV12 readback)");

    /*  NV12 格式
        +--------------------+
        | Y plane            |
        | W * H bytes        |
        +--------------------+
        | UV plane           |
        | W * H / 2 bytes    |
        +--------------------+
    */
    /* 从 staging 纹理复制 NV12 数据到 MFMediaBuffer。
       NV12 格式的前 ySize 字节是 Y 平面，后 uvSize 字节是交错的 UV 平面。
       每行数据可能有额外的 padding，所以需要按行复制 */
    BYTE* source = reinterpret_cast<BYTE*>(mapped.pData);
    for (UINT32 y = 0; y < height; ++y) {
        memcpy(destination + y * width, source + y * mapped.RowPitch, width);
    }

    BYTE* destinationUv = destination + ySize;
    BYTE* sourceUv = source + mapped.RowPitch * height;
    for (UINT32 y = 0; y < height / 2; ++y) {
        memcpy(destinationUv + y * width, sourceUv + y * mapped.RowPitch, width);
    }

    CheckHr(buffer->Unlock(), "IMFMediaBuffer::Unlock(NV12 readback)");
    CheckHr(buffer->SetCurrentLength(bufferSize), "IMFMediaBuffer::SetCurrentLength(NV12 readback)");
    context->Unmap(staging.get(), 0);

    winrt::com_ptr<IMFSample> sample;
    CheckHr(MFCreateSample(sample.put()), "MFCreateSample(NV12 readback input)");
    CheckHr(sample->AddBuffer(buffer.get()), "IMFSample::AddBuffer(NV12 readback input)");

    LONGLONG sampleDuration = 10'000'000LL / mFps;
    CheckHr(sample->SetSampleTime(frameIndex * sampleDuration), "IMFSample::SetSampleTime(NV12 readback input)");
    CheckHr(sample->SetSampleDuration(sampleDuration), "IMFSample::SetSampleDuration(NV12 readback input)");

    HRESULT hr = encoder->ProcessInput(0, sample.get(), 0);
    if (hr == MF_E_NOTACCEPTING) {
        ProcessH264Output(encoder);
        hr = encoder->ProcessInput(0, sample.get(), 0);
    }
    CheckHr(hr, "IMFTransform::ProcessInput(NV12 readback)");
    ProcessH264Output(encoder);
}

int WinCapMediaSource::videoExit()
{
    // 显式关闭会话和帧池，释放系统捕获资源和 D3D 纹理缓存。
    mSession.Close();
    mFramePool.Close();

    {
        std::lock_guard lock(mEncoderMutex);
        CheckHr(mTransform->ProcessMessage(MFT_MESSAGE_COMMAND_DRAIN, 0), "IMFTransform::ProcessMessage(COMMAND_DRAIN)");
        ProcessH264Output(mTransform.get());
        CheckHr(mTransform->ProcessMessage(MFT_MESSAGE_NOTIFY_END_OF_STREAM, 0), "IMFTransform::ProcessMessage(NOTIFY_END_OF_STREAM)");
        CheckHr(mTransform->ProcessMessage(MFT_MESSAGE_NOTIFY_END_STREAMING, 0), "IMFTransform::ProcessMessage(NOTIFY_END_STREAMING)");
    }

    // 与 MFStartup 成对调用，释放 Media Foundation 全局资源。
    MFShutdown();

    return 0;
}

/* 将h264 数据解析为单个NALU单元， Webrtc 只支持传送NALU 单元 */
void WinCapMediaSource::parseH264(Frame* frame, uint8_t *h264Data, int length)
{
    int i = 0;
    int before = 0;
    int num = 0;
    int offset = 0;

    if (Utils::startCode4(h264Data)) {
        offset = 4;
    } else if (Utils::startCode3(h264Data)) {
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
