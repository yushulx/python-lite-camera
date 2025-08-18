#include "CameraPreview.h"
#include <iostream>
#include <algorithm>
#include <cstring>

CameraWindow::CameraWindow(int w, int h, const char *title)
    : width(w), height(h), display(nullptr), window(0), gc(nullptr)
{
    this->title = new char[strlen(title) + 1];
    strcpy(this->title, title);
}

CameraWindow::~CameraWindow()
{
    delete[] title;

    if (gc)
    {
        XFreeGC(display, gc);
    }
    if (window)
    {
        XDestroyWindow(display, window);
    }
    if (display)
    {
        XCloseDisplay(display);
    }
}

bool CameraWindow::Create()
{
    display = XOpenDisplay(nullptr);
    if (!display)
    {
        std::cerr << "Failed to open X display." << std::endl;
        return false;
    }

    int screen = DefaultScreen(display);
    Window rootWindow = RootWindow(display, screen);

    window = XCreateSimpleWindow(
        display, rootWindow,
        0, 0, width, height, 1,
        BlackPixel(display, screen),
        WhitePixel(display, screen));

    if (!window)
    {
        std::cerr << "Failed to create window." << std::endl;
        return false;
    }

    XStoreName(display, window, title);

    gc = XCreateGC(display, window, 0, nullptr);
    if (!gc)
    {
        std::cerr << "Failed to create graphics context." << std::endl;
        return false;
    }

    wmDeleteMessage = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window, &wmDeleteMessage, 1);

    XSelectInput(display, window, ExposureMask | KeyPressMask | StructureNotifyMask);
    XMapWindow(display, window);

    return true;
}

void CameraWindow::Show()
{
    XFlush(display);
}

bool CameraWindow::WaitKey(char key)
{
    if (!display || !window)
        return false;

    while (XPending(display) > 0)
    {
        XEvent event;
        XNextEvent(display, &event);

        // Handle window close
        if (event.type == ClientMessage && (Atom)event.xclient.data.l[0] == wmDeleteMessage)
        {
            return false; // Exit the loop and close the window
        }

        // Handle keyboard input
        if (event.type == KeyPress)
        {
            char buffer[32] = {0};
            KeySym keysym;
            int len = XLookupString(&event.xkey, buffer, sizeof(buffer), &keysym, nullptr);

            if (len > 0 && buffer[0] != '\0')
            {
                if (key == '\0') // If no specific key is provided, return true for any key
                {
                    return false;
                }
                else if (buffer[0] == key || buffer[0] == std::toupper(key))
                {
                    return false; // Exit the loop for the specified key
                }
            }
        }
    }

    return true; // Continue waiting if no exit condition is met
}

void CameraWindow::ShowFrame(const unsigned char *rgbData, int frameWidth, int frameHeight)
{
    if (!display || !window || !gc || !rgbData)
    {
        std::cerr << "Invalid display, window, gc, or rgbData." << std::endl;
        return;
    }

    // Create a buffer with an alpha channel for each pixel
    int stride = frameWidth * 4; // 4 bytes per pixel (RGBA)
    std::vector<unsigned char> rgbaBuffer(frameWidth * frameHeight * 4);

    // Populate the buffer with RGB data and set alpha to 255
    for (int i = 0; i < frameWidth * frameHeight; ++i)
    {
        rgbaBuffer[i * 4 + 0] = rgbData[i * 3 + 2]; // Red
        rgbaBuffer[i * 4 + 1] = rgbData[i * 3 + 1]; // Green
        rgbaBuffer[i * 4 + 2] = rgbData[i * 3 + 0]; // Blue
        rgbaBuffer[i * 4 + 3] = 255;                // Alpha
    }

    // Try to create the XImage
    XImage *image = XCreateImage(
        display,
        DefaultVisual(display, DefaultScreen(display)),
        24,      // Depth: 24 bits for RGB
        ZPixmap, // Format
        0,       // Offset
        reinterpret_cast<char *>(rgbaBuffer.data()),
        frameWidth, frameHeight,
        32, // Bitmap pad: 32-bit alignment
        0   // Bytes per line (auto-calculated)
    );

    if (!image)
    {
        std::cerr << "Failed to create XImage." << std::endl;
        return;
    }

    XPutImage(display, window, gc, image, 0, 0, 0, 0, frameWidth, frameHeight);
    XFlush(display);
    image->data = nullptr; // Prevent XDestroyImage from freeing the buffer
    XDestroyImage(image);
}

void CameraWindow::DrawContour(const std::vector<std::pair<int, int>> &points)
{
    if (!display || !gc || !window || points.size() < 2)
        return;

    XSetForeground(display, gc, 0x00FF00); // Green color

    for (size_t i = 0; i < points.size(); ++i)
    {
        const auto &start = points[i];
        const auto &end = points[(i + 1) % points.size()];
        XDrawLine(display, window, gc, start.first, start.second, end.first, end.second);
    }

    XFlush(display);
}

void CameraWindow::DrawText(const std::string &text, int x, int y, int fontSize, const Color &color)
{
    if (!gc || !display || !window)
        return;

    XSetForeground(display, gc, (color.r << 16) | (color.g << 8) | color.b);

    XFontStruct *font = XLoadQueryFont(display, "-*-helvetica-*-r-*-*-12-*-*-*-*-*-*-*");
    if (!font)
    {
        font = XLoadQueryFont(display, "fixed");
    }
    if (!font)
    {
        std::cerr << "Failed to load font." << std::endl;
        return;
    }

    XSetFont(display, gc, font->fid);
    XDrawString(display, window, gc, x, y, text.c_str(), text.size());
    XFreeFont(display, font);
}
