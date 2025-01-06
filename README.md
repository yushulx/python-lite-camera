# LiteCam for Python
[LiteCam](https://github.com/yushulx/cmake-cpp-barcode-qrcode-mrz/tree/main/litecam) is a lightweight, cross-platform library for capturing RGB frames from cameras and displaying them. Designed with simplicity and ease of integration in mind, LiteCam supports **Windows**, **Linux** and **macOS** platforms. It integrates easily with image processing frameworks like **OpenCV**, making it a versatile choice for developers.

![lite camera for Python](https://www.dynamsoft.com/codepool/img/2025/01/lite-camera-python.png)

## Features

- Capture RGB frames from video devices.
- Display live frames in a window.
- Draw contours and text directly on the displayed frames.
- Simple API for quick integration.

## Supported Platforms

- **Windows**
- **Linux**
- **macOS**

## How to Build the CPython Extension    
- Development Mode
    
    ```bash
    python setup.py develop
    ```

- Wheel Package

    ```bash
    python setup.py bdist_wheel
    ```

- Source Distribution

    ```bash
    python setup.py sdist
    ```

## Usage

### Basic Example

```python
import litecam

camera = litecam.PyCamera()

if camera.open(0):

    window = litecam.PyWindow(
        camera.getWidth(), camera.getHeight(), "Camera Stream")

    while window.waitKey('q'):
        frame = camera.captureFrame()
        if frame is not None:
            width = frame[0]
            height = frame[1]
            size = frame[2]
            data = frame[3]
            window.showFrame(width, height, data)

    camera.release()
```

### API Overview

- **getDeviceList()**: Lists available video capture devices.
- **saveJpeg(filename, width, height, rgbdata)**: Saves the frame as a JPEG image.

#### PyCamera
- **open(index)**: Opens the camera with the specified index.
- **listMediaTypes()**: Lists supported media types.
- **setResolution(int width, int height)**: Sets the resolution for the camera.
- **captureFrame()`**: Captures a single RGB frame.
- **release()**: Closes the camera and releases resources.
- **getWidth()**: Returns the width of the frame.
- **getHeight()**: Returns the height of the frame.

#### PyWindow
- **waitKey(key)**: Waits for user input; returns `false` if the specified key is pressed or the window is closed.
- **showFrame(width, height, rgbdata)**: Displays an RGB frame.
- **drawContour(points)**: Draws contours on the frame.
- **drawText(text, x, y, fontSize, color)**: Draws text on the frame.