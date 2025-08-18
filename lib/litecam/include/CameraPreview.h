#ifndef CAMERA_WINDOW_H
#define CAMERA_WINDOW_H

#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#elif __linux__
#include <X11/Xlib.h>
#include <X11/Xutil.h>
// #elif __APPLE__
// #include <Cocoa/Cocoa.h>
#endif

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

class CAMERA_API CameraWindow
{
public:
    CameraWindow(int width, int height, const char *title);
    ~CameraWindow();

    bool Create();
    void Show();
    bool WaitKey(char key);
    void ShowFrame(const unsigned char *rgbData, int width, int height);
    void DrawContour(const std::vector<std::pair<int, int>> &points);

    struct Color
    {
        unsigned char r, g, b;
    };

    void DrawText(const std::string &text, int x, int y, int fontSize, const Color &color);

private:
    int width;
    int height;
    char *title;

#ifdef _WIN32
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    HWND hwnd;
    HDC hdc;
    WNDCLASS wc;
    HINSTANCE hInstance;
#elif __linux__
    // Add Linux-specific members (e.g., X11 display and drawable)
    Display *display;
    Window window;
    GC gc; // Graphics context
    Atom wmDeleteMessage;
#elif __APPLE__
    // Add macOS-specific members (e.g., NSWindow or CGContext)
    void *nsWindow; // Use proper macOS data structures here
    void *contentView;
#endif
};

#endif // CAMERA_WINDOW_H
