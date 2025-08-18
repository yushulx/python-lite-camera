#include "CameraPreview.h"
#include <iostream>

CameraWindow::CameraWindow(int w, int h, const char *title)
    : width(w), height(h), hwnd(nullptr), hdc(nullptr)
{

    this->title = new char[strlen(title) + 1];
    strcpy_s(this->title, strlen(title) + 1, title);

    hInstance = GetModuleHandle(nullptr);

    // Initialize window class
    wc = {};
    wc.lpfnWndProc = WindowProc; // Static window procedure
    wc.hInstance = hInstance;
    wc.lpszClassName = "CameraWindowClass";
}

CameraWindow::~CameraWindow()
{
    delete[] title;

    if (hdc)
    {
        ReleaseDC(hwnd, hdc);
    }
    if (hwnd)
    {
        DestroyWindow(hwnd);
    }
    UnregisterClass("CameraWindowClass", hInstance);
}

void CameraWindow::DrawText(const std::string &text, int x, int y, int fontSize, const Color &color)
{
    if (!hdc)
        return;

    // Set the text color and background mode
    SetTextColor(hdc, RGB(color.r, color.g, color.b));
    SetBkMode(hdc, TRANSPARENT);

    // Create a font for the text
    HFONT hFont = CreateFont(
        fontSize,                    // Height of the font
        0,                           // Width of the font (0 = auto)
        0,                           // Escapement angle (in tenths of degrees)
        0,                           // Orientation angle
        FW_NORMAL,                   // Font weight (FW_NORMAL, FW_BOLD, etc.)
        FALSE,                       // Italic
        FALSE,                       // Underline
        FALSE,                       // Strikeout
        DEFAULT_CHARSET,             // Character set
        OUT_DEFAULT_PRECIS,          // Output precision
        CLIP_DEFAULT_PRECIS,         // Clipping precision
        DEFAULT_QUALITY,             // Output quality
        DEFAULT_PITCH | FF_DONTCARE, // Family and pitch
        "Arial");                    // Font face name (e.g., "Arial", "Courier New")

    if (!hFont)
        return;

    // Select the font into the device context
    HGDIOBJ oldFont = SelectObject(hdc, hFont);

    // Draw the text
    TextOut(hdc, x, y, text.c_str(), static_cast<int>(text.length()));

    // Restore the previous font and clean up
    SelectObject(hdc, oldFont);
    DeleteObject(hFont);
}

LRESULT CALLBACK CameraWindow::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

bool CameraWindow::Create()
{
    // Register the window class
    if (!RegisterClass(&wc))
    {
        std::cerr << "Failed to register window class." << std::endl;
        return false;
    }

    // Create the window
    hwnd = CreateWindowEx(
        0, "CameraWindowClass", title, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        nullptr, nullptr, hInstance, nullptr);

    if (!hwnd)
    {
        std::cerr << "Failed to create window." << std::endl;
        return false;
    }

    hdc = GetDC(hwnd);
    return true;
}

void CameraWindow::Show()
{
    ShowWindow(hwnd, SW_SHOW);
}

bool CameraWindow::WaitKey(char key)
{
    MSG msg = {};
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        // Handle WM_QUIT (default quit behavior)
        if (msg.message == WM_QUIT)
        {
            return false; // Exit the message loop
        }

        // Handle keyboard input
        if (msg.message == WM_KEYDOWN)
        {
            char keyPressed = static_cast<char>(msg.wParam);

            // If a specific key is passed, return false if it matches
            if (key != '\0' && (keyPressed == key || keyPressed == std::toupper(key)))
            {
                return false; // Quit if the specific key is pressed
            }
        }
    }
    return true;
}

void CameraWindow::ShowFrame(const unsigned char *rgbData, int frameWidth, int frameHeight)
{
    if (!hdc || !rgbData)
        return;

    // Prepare the bitmap information
    BITMAPINFO bmpInfo = {};
    bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmpInfo.bmiHeader.biWidth = frameWidth;
    bmpInfo.bmiHeader.biHeight = -frameHeight; // Negative to flip the image vertically
    bmpInfo.bmiHeader.biPlanes = 1;
    bmpInfo.bmiHeader.biBitCount = 24; // 24 bits (3 bytes per pixel for RGB)
    bmpInfo.bmiHeader.biCompression = BI_RGB;

    // Draw the frame
    StretchDIBits(
        hdc,
        0, 0, frameWidth, frameHeight, // Destination rectangle
        0, 0, frameWidth, frameHeight, // Source rectangle
        rgbData,                       // RGB pixel data
        &bmpInfo,                      // Bitmap information
        DIB_RGB_COLORS,                // Color format
        SRCCOPY                        // Raster operation
    );
}

void CameraWindow::DrawContour(const std::vector<std::pair<int, int>> &points)
{
    if (!hdc || points.size() < 4)
        return;

    HPEN hPen = CreatePen(PS_SOLID, 2, RGB(0, 255, 0)); // Green pen
    HGDIOBJ oldPen = SelectObject(hdc, hPen);

    // Draw contour lines based on the given points
    MoveToEx(hdc, points[0].first, points[0].second, nullptr);
    for (size_t i = 1; i < points.size(); ++i)
    {
        LineTo(hdc, points[i].first, points[i].second);
    }
    LineTo(hdc, points[0].first, points[0].second); // Close the contour shape

    // Restore old pen and clean up
    SelectObject(hdc, oldPen);
    DeleteObject(hPen);
}
