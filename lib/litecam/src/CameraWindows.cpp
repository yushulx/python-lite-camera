#include "Camera.h"
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <set>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

using Microsoft::WRL::ComPtr;

unsigned char clamp(double value, double min, double max)
{
    if (value < min)
        return static_cast<unsigned char>(min);
    if (value > max)
        return static_cast<unsigned char>(max);
    return static_cast<unsigned char>(value);
}

void ConvertYUY2ToRGB(const unsigned char *yuy2Data, unsigned char *rgbData, int width, int height)
{
    int rgbIndex = 0;
    for (int i = 0; i < width * height * 2; i += 4)
    {
        // Extract YUV values
        unsigned char y1 = yuy2Data[i];
        unsigned char u = yuy2Data[i + 1];
        unsigned char y2 = yuy2Data[i + 2];
        unsigned char v = yuy2Data[i + 3];

#ifdef _WIN32
        // Convert first pixel (Y1, U, V) to RGB
        rgbData[rgbIndex++] = clamp(y1 + 1.772 * (u - 128), 0.0, 255.0);
        rgbData[rgbIndex++] = clamp(y1 - 0.344136 * (u - 128) - 0.714136 * (v - 128), 0.0, 255.0);
        rgbData[rgbIndex++] = clamp(y1 + 1.402 * (v - 128), 0.0, 255.0);

        // Convert second pixel (Y2, U, V) to RGB
        rgbData[rgbIndex++] = clamp(y2 + 1.772 * (u - 128), 0.0, 255.0);
        rgbData[rgbIndex++] = clamp(y2 - 0.344136 * (u - 128) - 0.714136 * (v - 128), 0.0, 255.0);
        rgbData[rgbIndex++] = clamp(y2 + 1.402 * (v - 128), 0.0, 255.0);
#else
        // Convert first pixel (Y1, U, V) to RGB
        rgbData[rgbIndex++] = clamp(y1 + 1.402 * (v - 128), 0.0, 255.0);
        rgbData[rgbIndex++] = clamp(y1 - 0.344136 * (u - 128) - 0.714136 * (v - 128), 0.0, 255.0);
        rgbData[rgbIndex++] = clamp(y1 + 1.772 * (u - 128), 0.0, 255.0);

        // Convert second pixel (Y2, U, V) to RGB
        rgbData[rgbIndex++] = clamp(y2 + 1.402 * (v - 128), 0.0, 255.0);
        rgbData[rgbIndex++] = clamp(y2 - 0.344136 * (u - 128) - 0.714136 * (v - 128), 0.0, 255.0);
        rgbData[rgbIndex++] = clamp(y2 + 1.772 * (u - 128), 0.0, 255.0);
#endif
    }
}

struct GUIDComparator
{
    bool operator()(const GUID &lhs, const GUID &rhs) const
    {
        return memcmp(&lhs, &rhs, sizeof(GUID)) < 0;
    }
};

const wchar_t *GetSubtypeName(const GUID &subType)
{
    static const std::map<GUID, const wchar_t *, GUIDComparator> knownSubtypes = {
        {MFVideoFormat_YUY2, L"MFVideoFormat_YUY2"},
        {MFVideoFormat_NV12, L"MFVideoFormat_NV12"},
        {MFVideoFormat_RGB32, L"MFVideoFormat_RGB32"},
        {MFVideoFormat_I420, L"MFVideoFormat_I420"}
        // Add more known formats as needed
    };

    auto it = knownSubtypes.find(subType);
    if (it != knownSubtypes.end())
    {
        return it->second;
    }

    // Fallback: Return the GUID as a string
    static WCHAR guidString[40];
    StringFromGUID2(subType, guidString, sizeof(guidString) / sizeof(WCHAR));
    return guidString;
}

Camera::Camera() : reader(nullptr), frameWidth(640), frameHeight(480), initialized(false)
{
    InitializeMediaFoundation();
}

Camera::~Camera()
{
    Release();
    ShutdownMediaFoundation();
}

void Camera::InitializeMediaFoundation()
{
    if (!initialized)
    {
        HRESULT hr = MFStartup(MF_VERSION);
        if (FAILED(hr))
        {
            std::cerr << "Failed to initialize Media Foundation." << std::endl;
        }
        else
        {
            initialized = true;
        }
    }
}

void Camera::ShutdownMediaFoundation()
{
    if (initialized)
    {
        if (ms)
        {
            ms->Shutdown();
            ms.Reset();
        }

        MFShutdown();
        initialized = false;
    }
}

bool Camera::Open(int cameraIndex)
{
    Release();

    // Enumerate video devices
    ComPtr<IMFAttributes> attributes;
    HRESULT hr = MFCreateAttributes(&attributes, 1);
    if (FAILED(hr))
        return false;

    hr = attributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
    if (FAILED(hr))
        return false;

    IMFActivate **devices = nullptr;
    UINT32 count = 0;
    hr = MFEnumDeviceSources(attributes.Get(), &devices, &count);
    if (FAILED(hr) || count == 0)
    {
        std::cerr << "No video devices found." << std::endl;
        return false;
    }

    if (cameraIndex >= (int)count)
    {
        std::cerr << "Invalid camera index." << std::endl;
        for (UINT32 i = 0; i < count; i++)
            devices[i]->Release();
        CoTaskMemFree(devices);
        return false;
    }

    // Activate the specified camera
    ComPtr<IMFMediaSource> mediaSource;
    hr = devices[cameraIndex]->ActivateObject(IID_PPV_ARGS(&mediaSource));
    for (UINT32 i = 0; i < count; i++)
        devices[i]->Release();
    CoTaskMemFree(devices);

    if (FAILED(hr))
        return false;

    this->ms = mediaSource;

    ComPtr<IMFSourceReader> mfReader;
    hr = MFCreateSourceReaderFromMediaSource(mediaSource.Get(), nullptr, &mfReader);
    if (FAILED(hr))
        return false;

    // Set media type to YUY2 format
    ComPtr<IMFMediaType> mediaType;
    hr = MFCreateMediaType(&mediaType);
    if (FAILED(hr))
        return false;

    hr = mediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    hr = mediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_YUY2);
    hr = MFSetAttributeSize(mediaType.Get(), MF_MT_FRAME_SIZE, frameWidth, frameHeight);

    if (SUCCEEDED(hr))
    {
        hr = mfReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, mediaType.Get());

        reader = reinterpret_cast<void *>(mfReader.Detach());

        if (SUCCEEDED(hr))
        {
            return true;
        }
    }

    return false;
}

void Camera::Release()
{
    if (reader)
    {
        ComPtr<IMFSourceReader> mfReader(static_cast<IMFSourceReader *>(reader));
        reader = nullptr;

        ShutdownMediaFoundation();
    }
}

std::vector<MediaTypeInfo> Camera::ListSupportedMediaTypes()
{
    std::vector<MediaTypeInfo> mediaTypes;

    if (!reader)
        return mediaTypes;

    // Use a set to store unique media types
    std::set<std::tuple<uint32_t, uint32_t, std::wstring>> uniqueMediaTypes;
    IMFSourceReader *mfReader = reinterpret_cast<IMFSourceReader *>(reader);
    for (DWORD i = 0;; i++)
    {
        ComPtr<IMFMediaType> mediaType;
        HRESULT hr = mfReader->GetNativeMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, i, &mediaType);
        if (FAILED(hr))
            break;

        MediaTypeInfo info = {};
        MFGetAttributeSize(mediaType.Get(), MF_MT_FRAME_SIZE, &info.width, &info.height);

        WCHAR subtypeName[40] = {};
        GUID subType;
        mediaType->GetGUID(MF_MT_SUBTYPE, &subType);
        StringFromGUID2(subType, subtypeName, sizeof(subtypeName) / sizeof(WCHAR));
        const wchar_t *typeName = GetSubtypeName(subType);
        wcsncpy(info.subtypeName, typeName, sizeof(subtypeName) / sizeof(WCHAR));

        // Check for uniqueness
        auto key = std::make_tuple(info.width, info.height, info.subtypeName);
        if (uniqueMediaTypes.find(key) == uniqueMediaTypes.end())
        {
            uniqueMediaTypes.insert(key); // Add to the set
            mediaTypes.push_back(info);   // Add to the list
        }
    }

    return mediaTypes;
}

bool Camera::SetResolution(int width, int height)
{
    if (!reader)
        return false;

    ComPtr<IMFMediaType> mediaType;
    HRESULT hr = MFCreateMediaType(&mediaType);
    if (FAILED(hr))
        return false;

    hr = mediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    hr = mediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_YUY2);
    hr = MFSetAttributeSize(mediaType.Get(), MF_MT_FRAME_SIZE, width, height);

    if (SUCCEEDED(hr))
    {
        IMFSourceReader *mfReader = reinterpret_cast<IMFSourceReader *>(reader);

        hr = mfReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, mediaType.Get());
        if (SUCCEEDED(hr))
        {
            frameWidth = width;
            frameHeight = height;
            return true;
        }
    }

    return false;
}

FrameData Camera::CaptureFrame()
{

    HRESULT hr;
    DWORD streamIndex, flags;
    LONGLONG timestamp;
    ComPtr<IMFSample> sample;
    FrameData frame;

    frame.width = frameWidth;
    frame.height = frameHeight;
    frame.rgbData = nullptr;

    // Read a sample from the media source
    IMFSourceReader *mfReader = reinterpret_cast<IMFSourceReader *>(reader);
    hr = mfReader->ReadSample(
        MF_SOURCE_READER_FIRST_VIDEO_STREAM,
        0,
        &streamIndex,
        &flags,
        &timestamp,
        &sample);

    if (FAILED(hr))
    {
        std::cerr << "Failed to read sample." << std::endl;
        return frame; // Return an empty FrameData structure on failure
    }

    if (sample)
    {
        ComPtr<IMFMediaBuffer> buffer;
        hr = sample->ConvertToContiguousBuffer(&buffer);
        if (FAILED(hr))
        {
            std::cerr << "Failed to convert sample to contiguous buffer." << std::endl;
            return frame; // Return an empty FrameData structure on failure
        }

        BYTE *rawData = nullptr;
        DWORD maxLength = 0, currentLength = 0;
        hr = buffer->Lock(&rawData, &maxLength, &currentLength);
        if (SUCCEEDED(hr))
        {
            // Allocate memory for the RGB frame
            frame.size = frameWidth * frameHeight * 3;
            frame.rgbData = new unsigned char[frame.size];
            if (!frame.rgbData)
            {
                std::cerr << "Failed to allocate memory for RGB data." << std::endl;
                return frame; // Return empty frame
            }

            // Convert YUY2 to RGB
            ConvertYUY2ToRGB(rawData, frame.rgbData, frameWidth, frameHeight);

            buffer->Unlock();
        }
    }

    return frame;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

void DisplayFrameInWindow(const unsigned char *rgbData, int width, int height, HWND hwnd, HDC hdc, const std::vector<std::pair<int, int>> &points)
{
    BITMAPINFO bmpInfo = {};
    bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmpInfo.bmiHeader.biWidth = width;
    bmpInfo.bmiHeader.biHeight = -height; // Negative to flip the image vertically
    bmpInfo.bmiHeader.biPlanes = 1;
    bmpInfo.bmiHeader.biBitCount = 24; // 24 bits (3 bytes per pixel for RGB)
    bmpInfo.bmiHeader.biCompression = BI_RGB;

    StretchDIBits(
        hdc,                 // Handle to device context
        0, 0, width, height, // Destination rectangle
        0, 0, width, height, // Source rectangle
        rgbData,             // Pointer to image data
        &bmpInfo,            // Pointer to BITMAPINFO
        DIB_RGB_COLORS,      // Color format
        SRCCOPY              // Raster operation code
    );

    if (points.size() >= 4)
    {                                                       // Ensure there are at least 4 points
        HPEN hPen = CreatePen(PS_SOLID, 2, RGB(0, 255, 0)); // Green pen for the contour
        SelectObject(hdc, hPen);

        // Start at the first point
        MoveToEx(hdc, points[0].first, points[0].second, nullptr);

        // Draw lines to the next points
        for (size_t i = 1; i < points.size(); ++i)
        {
            LineTo(hdc, points[i].first, points[i].second);
        }

        // Close the contour (connect the last point to the first)
        LineTo(hdc, points[0].first, points[0].second);

        // Clean up GDI objects
        DeleteObject(hPen);
    }
}

std::vector<MediaTypeInfo> ListSupportedMediaTypes(ComPtr<IMFSourceReader> reader)
{
    HRESULT hr;
    ComPtr<IMFMediaType> mediaType;
    std::vector<MediaTypeInfo> mediaTypes;

    for (DWORD i = 0;; i++)
    {
        hr = reader->GetNativeMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, i, &mediaType);
        if (FAILED(hr))
        {
            break; // Exit the loop if no more media types are available
        }

        MediaTypeInfo info = {};
        GUID majorType, subType;

        // Retrieve media type attributes
        mediaType->GetGUID(MF_MT_MAJOR_TYPE, &majorType);
        mediaType->GetGUID(MF_MT_SUBTYPE, &subType);
        MFGetAttributeSize(mediaType.Get(), MF_MT_FRAME_SIZE, &info.width, &info.height);

        // Convert GUID subtype to string
        WCHAR subTypeName[40] = {};
        StringFromGUID2(subType, subTypeName, sizeof(subTypeName) / sizeof(WCHAR));
        wcsncpy(info.subtypeName, subTypeName, sizeof(subTypeName) / sizeof(WCHAR));

        // Store the media type information in the vector
        mediaTypes.push_back(info);

        mediaType.Reset();
    }

    return mediaTypes;
}

std::vector<CaptureDeviceInfo> ListCaptureDevices()
{
    HRESULT hr = S_OK;
    ComPtr<IMFAttributes> attributes;
    std::vector<CaptureDeviceInfo> devicesInfo;

    // Create attributes to specify video capture device
    hr = MFCreateAttributes(&attributes, 1);
    if (FAILED(hr))
    {
        std::cerr << "Failed to create attributes." << std::endl;
        return devicesInfo;
    }

    hr = attributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
    if (FAILED(hr))
    {
        std::cerr << "Failed to set video capture device attribute." << std::endl;
        return devicesInfo;
    }

    // Enumerate video capture devices
    UINT32 count = 0;
    IMFActivate **devices = nullptr;

    hr = MFEnumDeviceSources(attributes.Get(), &devices, &count);
    if (FAILED(hr) || count == 0)
    {
        std::cerr << "No video capture devices found." << std::endl;
        return devicesInfo;
    }

    for (UINT32 i = 0; i < count; ++i)
    {
        WCHAR *friendlyName = nullptr;
        UINT32 nameLength = 0;

        hr = devices[i]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &friendlyName, &nameLength);
        if (SUCCEEDED(hr))
        {
            CaptureDeviceInfo info = {};
            wcsncpy(info.friendlyName, friendlyName, nameLength);
            devicesInfo.push_back(info);
            CoTaskMemFree(friendlyName);
        }
        devices[i]->Release();
    }

    CoTaskMemFree(devices);
    return devicesInfo;
}

void ReleaseFrame(FrameData &frame)
{
    if (frame.rgbData)
    {
        delete[] frame.rgbData;
        frame.rgbData = nullptr;
        frame.size = 0;
    }
}

///////////////////////////////////////////////////////////////////////////////
// Save a frame as a JPEG image using the STB library
// https://github.com/nothings/stb/blob/master/stb_image_write.h
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
void saveFrameAsJPEG(const unsigned char *data, int width, int height, const std::string &filename)
{
    // Simple image saving using STB library or another JPEG encoding method
    if (stbi_write_jpg(filename.c_str(), width, height, 3, data, 90))
    {
        std::cout << "Saved frame to " << filename << std::endl;
    }
    else
    {
        std::cerr << "Error saving frame as JPEG." << std::endl;
    }
}
///////////////////////////////////////////////////////////////////////////////