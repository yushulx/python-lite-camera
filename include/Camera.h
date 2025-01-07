#ifndef CAMERA_H
#define CAMERA_H

#include <vector>
#include <string>
#include <iostream>
#include <cstdint>

// Export macro for shared library
#ifdef _WIN32
#ifdef CAMERA_EXPORTS
#define CAMERA_API __declspec(dllexport)
#else
#define CAMERA_API __declspec(dllimport)
#endif
#elif defined(__linux__) || defined(__APPLE__)
#define CAMERA_API __attribute__((visibility("default")))
#else
#define CAMERA_API
#endif

// Platform-specific includes
#ifdef _WIN32
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfobjects.h>
#include <mfreadwrite.h>
#include <wrl/client.h>
#include <dshow.h>

#elif __linux__
#include <linux/videodev2.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

struct Buffer
{
    void *start;
    size_t length;
};
#endif

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

// Struct definitions
struct CAMERA_API FrameData
{
    unsigned char *rgbData; // RGB pixel data
    int width;              // Frame width
    int height;             // Frame height
    size_t size;
};

struct CAMERA_API MediaTypeInfo
{
    uint32_t width;
    uint32_t height;
#ifdef _WIN32
    wchar_t subtypeName[512]; // Wide characters for Windows
#else
    char subtypeName[512]; // Narrow characters for Linux/macOS
#endif
};

struct CAMERA_API CaptureDeviceInfo
{

#ifdef _WIN32
    wchar_t friendlyName[512];
#else
    char friendlyName[512]; // Narrow characters for Linux/macOS
#endif
};

// Exported functions
CAMERA_API std::vector<CaptureDeviceInfo> ListCaptureDevices();
CAMERA_API void ReleaseFrame(FrameData &frame);
CAMERA_API void saveFrameAsJPEG(const unsigned char *data, int width, int height, const std::string &filename);

// Camera class
class CAMERA_API Camera
{
public:
#ifdef _WIN32
    Camera();
    ~Camera();
#elif __linux__
    Camera() : fd(-1), frameWidth(640), frameHeight(480), buffers(nullptr), bufferCount(0) {}
    ~Camera() { Release(); }
#elif __APPLE__
    Camera() noexcept; // Add noexcept to match the implementation
    ~Camera();
#endif

    bool Open(int cameraIndex);
    void Release();

    std::vector<MediaTypeInfo> ListSupportedMediaTypes();
    FrameData CaptureFrame();
    bool SetResolution(int width, int height);

    uint32_t frameWidth;
    uint32_t frameHeight;

private:
#ifdef _WIN32
    void *reader;

    bool initialized;
    void InitializeMediaFoundation();
    void ShutdownMediaFoundation();
#endif

#ifdef __linux__
    int fd;
    Buffer *buffers;
    unsigned int bufferCount;

    bool InitDevice();
    void UninitDevice();
    bool StartCapture();
    void StopCapture();
#endif

#ifdef __APPLE__
    void *captureSession; // AVFoundation session object
    void *videoOutput;
#endif
};

#endif // CAMERA_H
