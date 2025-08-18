# LiteCam

LiteCam is a lightweight, cross-platform library for capturing RGB frames from cameras and displaying them. Designed with simplicity and ease of integration in mind, LiteCam supports **Windows**, **Linux** and **macOS** platforms.

## Demo Video
- Linux Virtual Machine

  https://github.com/user-attachments/assets/908b0a6e-4e3b-4064-8921-22229f41a9b7

- Windows

  https://github.com/user-attachments/assets/98af2001-6935-4151-a1ed-c5afe224491c

- macOS

  https://github.com/user-attachments/assets/ea2a2350-cd6a-4835-acae-2b78308b3b8a


  
## Features

- Capture RGB frames from video devices.
- Display live frames in a window.
- Draw contours and text directly on the displayed frames.
- Simple API for quick integration.

## Supported Platforms

- **Windows**: Uses Media Foundation for video capture.
- **Linux**: Uses Video4Linux (V4L2) and X11 for video capture and rendering.
- **macOS**: Uses AVFoundation for video capture.

## Requirements

- CMake

### Windows
- Visual Studio 2019 or later
- Media Foundation (included with Windows)

### Linux
- GCC or Clang
- Development libraries for:
  - Video4Linux2 (libv4l2)
  - X11 (libX11)
    
    ```bash
    sudo apt update
    sudo apt install libx11-dev
    ```

### macOS
- Xcode
- Development libraries for AVFoundation (included with macOS)    

## Installation

1. Clone the repository:
    ```bash
    git clone https://github.com/yushulx/cmake-cpp-barcode-qrcode-mrz.git
    cd litecam
    ```

2. Build the library:

    ```bash
    mkdir build && cd build
    cmake ..
    cmake --build .

    ## macOS: Use the lipo tool to inspect the architectures included in the .dylib
    lipo -info liblitecam.dylib
    ```

3. Link the library to your project.

## Usage

### Basic Example

```cpp
#include "Camera.h"
#include "CameraPreview.h"

int main()
{
    Camera camera;
    if (!camera.Open(0)) {
        std::cerr << "Failed to open camera." << std::endl;
        return -1;
    }

    CameraWindow window(640, 480, "LiteCam Preview");
    if (!window.Create()) {
        std::cerr << "Failed to create window." << std::endl;
        return -1;
    }

    window.Show();

    while (window.WaitKey('q')) {
        FrameData frame = camera.CaptureFrame();
        if (frame.rgbData) {
            window.ShowFrame(frame.rgbData, frame.width, frame.height);
            ReleaseFrame(frame);
        }
    }

    camera.Release();
    return 0;
}
```

### API Overview

#### Camera
- **`std::vector<CaptureDeviceInfo> ListCaptureDevices()`**: Lists available video capture devices.
- **`bool Open(int cameraIndex)`**: Opens the camera with the specified index.
- **`bool SetResolution(int width, int height)`**: Sets the resolution for the camera.
- **`std::vector<MediaTypeInfo> ListSupportedMediaTypes()`**: Lists supported media types.
- **`FrameData CaptureFrame()`**: Captures a single RGB frame.
- **`void ReleaseFrame(FrameData &frame)`**: Releases the memory allocated for a frame.
- **`void Release()`**: Closes the camera and releases resources.
- **`void saveFrameAsJPEG(const char *filename, const unsigned char *rgbData, int width, int height)`**: Saves the frame as a JPEG image.

#### CameraWindow
- **`bool Create()`**: Initializes and creates a window.
- **`void Show()`**: Displays the window.
- **`bool WaitKey(char key)`**: Waits for user input; returns `false` if the specified key is pressed or the window is closed.
- **`void ShowFrame(const unsigned char *rgbData, int width, int height)`**: Displays an RGB frame.
- **`void DrawContour(const std::vector<std::pair<int, int>> &points)`**: Draws contours on the frame.
- **`void DrawText(const std::string &text, int x, int y, int fontSize, const Color &color)`**: Draws text on the frame.

## Blog
- [Building a Lightweight C++ Camera Library for Barcode Scanning on Linux](https://www.dynamsoft.com/codepool/linux-cpp-camera-barcode-scanner.html)
- [How to Implement Camera Preview with Windows Media Foundation API in C++](https://www.dynamsoft.com/codepool/windows-cpp-camera-barcode-scanner.html)
- [Mixing Objective-C and C++ in macOS to Build a Camera-Based Barcode Scanner](https://www.dynamsoft.com/codepool/macos-cpp-camera-barcode-scanner.html)
