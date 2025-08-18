#include "Camera.h"
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <cstring>

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

bool Camera::Open(int cameraIndex)
{
    std::string devicePath = "/dev/video" + std::to_string(cameraIndex);
    fd = open(devicePath.c_str(), O_RDWR);
    if (fd < 0)
    {
        perror("Error opening video device");
        return false;
    }

    struct v4l2_capability cap;
    if (ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0)
    {
        perror("Error querying device capabilities");
        close(fd);
        return false;
    }

    std::cout << "Driver: " << cap.driver << "\nCard: " << cap.card << std::endl;

    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = frameWidth;
    fmt.fmt.pix.height = frameHeight;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;

    if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0)
    {
        perror("Error setting resolution");
        return false;
    }

    if (!InitDevice() || !StartCapture())
    {
        Release();
        return false;
    }

    return true;
}

void Camera::Release()
{
    StopCapture();
    UninitDevice();
    if (fd >= 0)
    {
        close(fd);
        fd = -1;
    }
}

bool Camera::SetResolution(int width, int height)
{
    if (fd == -1)
    {
        std::cerr << "Device not opened." << std::endl;
        return false;
    }

    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = width;
    fmt.fmt.pix.height = height;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;

    if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0)
    {
        perror("Error setting resolution");
        return false;
    }

    // Save the actual resolution set by the device
    frameWidth = fmt.fmt.pix.width;
    frameHeight = fmt.fmt.pix.height;

    return true;
}

FrameData Camera::CaptureFrame()
{
    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    // Dequeue a buffer from the video capture queue
    if (ioctl(fd, VIDIOC_DQBUF, &buf) < 0)
    {
        perror("Failed to dequeue buffer");
        return {};
    }

    // FILE *file = fopen("raw_frame.yuyv", "wb");
    // fwrite(buffers[buf.index].start, 1, buffers[buf.index].length, file);
    // fclose(file);

    // Prepare FrameData structure
    FrameData frame;
    frame.width = frameWidth;
    frame.height = frameHeight;
    frame.size = frameWidth * frameHeight * 3; // 3 bytes per pixel (RGB)
    frame.rgbData = new unsigned char[frame.size];

    // Convert YUYV to RGB
    ConvertYUY2ToRGB(reinterpret_cast<unsigned char *>(buffers[buf.index].start), frame.rgbData, frameWidth, frameHeight);

    // Requeue the buffer
    if (ioctl(fd, VIDIOC_QBUF, &buf) < 0)
    {
        perror("Failed to queue buffer");
    }

    return frame;
}

bool Camera::InitDevice()
{
    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));
    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0)
    {
        perror("Failed to request buffers");
        return false;
    }

    buffers = new Buffer[req.count];
    bufferCount = req.count;

    for (unsigned int i = 0; i < bufferCount; ++i)
    {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0)
        {
            perror("Failed to query buffer");
            return false;
        }

        buffers[i].length = buf.length;
        buffers[i].start = mmap(nullptr, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);

        if (buffers[i].start == MAP_FAILED)
        {
            perror("Failed to map buffer");
            return false;
        }

        if (ioctl(fd, VIDIOC_QBUF, &buf) < 0)
        {
            perror("Failed to queue buffer");
            return false;
        }
    }
    return true;
}

void Camera::UninitDevice()
{
    if (buffers)
    {
        for (unsigned int i = 0; i < bufferCount; ++i)
        {
            munmap(buffers[i].start, buffers[i].length);
        }
        delete[] buffers;
        buffers = nullptr;
    }
}

bool Camera::StartCapture()
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    return ioctl(fd, VIDIOC_STREAMON, &type) >= 0;
}

void Camera::StopCapture()
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(fd, VIDIOC_STREAMOFF, &type);
}

std::vector<MediaTypeInfo> Camera::ListSupportedMediaTypes()
{
    std::vector<MediaTypeInfo> mediaTypes;

    if (fd == -1)
    {
        std::cerr << "Device not opened.\n";
        return mediaTypes;
    }

    struct v4l2_fmtdesc fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    while (ioctl(fd, VIDIOC_ENUM_FMT, &fmt) == 0)
    {
        struct v4l2_frmsizeenum frmsize;
        memset(&frmsize, 0, sizeof(frmsize));
        frmsize.pixel_format = fmt.pixelformat;

        // Iterate over supported frame sizes for this format
        while (ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) == 0)
        {
            MediaTypeInfo info = {};
            info.width = 0;
            info.height = 0;

            // Handle discrete frame sizes
            if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE)
            {
                info.width = frmsize.discrete.width;
                info.height = frmsize.discrete.height;
            }
            // Handle stepwise frame sizes
            else if (frmsize.type == V4L2_FRMSIZE_TYPE_STEPWISE)
            {
                // Use the minimum resolution as an example
                info.width = frmsize.stepwise.min_width;
                info.height = frmsize.stepwise.min_height;
            }

            // Copy the format description
            strncpy(info.subtypeName, reinterpret_cast<const char *>(fmt.description), sizeof(info.subtypeName) - 1);
            info.subtypeName[sizeof(info.subtypeName) - 1] = '\0'; // Ensure null-termination

            mediaTypes.push_back(info);
            frmsize.index++;
        }

        fmt.index++;
    }

    if (mediaTypes.empty())
    {
        std::cerr << "No supported media types found.\n";
    }

    return mediaTypes;
}

std::vector<CaptureDeviceInfo> ListCaptureDevices()
{
    std::vector<CaptureDeviceInfo> devices;

    // Scan for /dev/videoX devices
    for (int i = 0; i < 10; ++i) // Check first 10 devices (can be increased if needed)
    {
        std::string devicePath = "/dev/video" + std::to_string(i);

        // Open the device
        int fd = open(devicePath.c_str(), O_RDWR | O_NONBLOCK, 0);
        if (fd == -1)
        {
            continue; // Skip if the device cannot be opened
        }

        // Query device capabilities
        struct v4l2_capability cap;
        if (ioctl(fd, VIDIOC_QUERYCAP, &cap) == 0)
        {
            // Check if it's a video capture device
            if (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)
            {
                CaptureDeviceInfo deviceInfo = {};

                // Copy the friendly name directly into the `friendlyName` buffer
                strncpy(deviceInfo.friendlyName, reinterpret_cast<const char *>(cap.card), sizeof(deviceInfo.friendlyName) - 1);

                // Null-terminate the friendlyName string
                deviceInfo.friendlyName[sizeof(deviceInfo.friendlyName) - 1] = '\0';

                devices.push_back(deviceInfo);
            }
        }

        // Close the device
        close(fd);
    }

    return devices;
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