
#include <iostream>
#include "Camera.h"
#include "CameraPreview.h"

#include <cstdlib> // For wcstombs

#if __APPLE__
#include <Cocoa/Cocoa.h>
#endif

void PrintMediaTypes(const std::vector<MediaTypeInfo> &mediaTypes)
{
    printf("Supported Media Types:\n");
    for (size_t i = 0; i < mediaTypes.size(); ++i)
    {
#ifdef _WIN32
        // For Windows: Use wide-character format for subtypeName
        wprintf(L"[%zu]: Width: %u, Height: %u, Format: %ls\n",
                i,
                mediaTypes[i].width,
                mediaTypes[i].height,
                mediaTypes[i].subtypeName);
#else
        // For Linux: Use narrow-character format for subtypeName
        printf("[%zu]: Width: %u, Height: %u, Format: %s\n",
               i,
               mediaTypes[i].width,
               mediaTypes[i].height,
               mediaTypes[i].subtypeName);
#endif
    }
}

void PrintDevices(const std::vector<CaptureDeviceInfo> &devices)
{
    printf("Video Capture Devices:\n");
    for (size_t i = 0; i < devices.size(); ++i)
    {

#ifdef _WIN32
        char buffer[512]; // Buffer for converted string
        // wcstombs(buffer, devices[i].friendlyName, sizeof(buffer));
        wcstombs_s(nullptr, buffer, devices[i].friendlyName, sizeof(buffer));
        printf("[%zu]: %s\n", i, buffer);
#else
        printf("[%zu]: %s\n", i, devices[i].friendlyName);
#endif
    }
}

int main()
{
    auto devices = ListCaptureDevices();

    if (devices.empty())
    {
        std::cout << "No video capture devices found.\n";
        return -1;
    }
    else
    {
        PrintDevices(devices);
    }

    Camera camera;

    std::cout << "Capturing frames...\n";
    if (camera.Open(0))
    {
        auto mediaTypes = camera.ListSupportedMediaTypes();
        if (mediaTypes.empty())
        {
            printf("No supported media types found.\n");
        }
        else
        {
            PrintMediaTypes(mediaTypes);
        }

        for (int i = 0; i < 2; ++i)
        {
            FrameData frame = camera.CaptureFrame();
            if (frame.rgbData)
            {
                std::string filename = "frame_" + std::to_string(i) + ".jpg";
                saveFrameAsJPEG(frame.rgbData, camera.frameWidth, camera.frameHeight, filename);
            }

            ReleaseFrame(frame);
#ifdef _WIN32
            Sleep(500);
#else
            sleep(1);
#endif
        }

        // Create a window
        CameraWindow window(camera.frameWidth, camera.frameHeight, "Camera Stream");
        if (!window.Create())
        {
            std::cerr << "Failed to create window." << std::endl;
            return -1;
        }

        window.Show();

        // Start streaming frames
        while (window.WaitKey('q'))
        {
            // Capture a frame
            FrameData frame = camera.CaptureFrame();

            if (frame.rgbData)
            {
                // Display the frame
                window.ShowFrame(frame.rgbData, frame.width, frame.height);

                // Define points for the contour (example)
                std::vector<std::pair<int, int>> points = {
                    {100, 100}, // Top-left
                    {200, 100}, // Top-right
                    {200, 200}, // Bottom-right
                    {100, 200}  // Bottom-left
                };

                window.DrawContour(points);

                CameraWindow::Color textColor = {255, 0, 0};
                window.DrawText("Hello, Camera!", 50, 50, 24, textColor);
            }

            ReleaseFrame(frame);
        }

        camera.Release();
    }
    return 0;
}
