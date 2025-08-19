/*
 CameraPreviewMacOS.mm
*/

#include "CameraPreview.h"
#import <Cocoa/Cocoa.h>
#include <vector>
#include <string>
#include <utility>
#include <cstring>

// Define a C++ implementation struct to hold drawing data
struct CameraContentViewImpl {
    std::vector<unsigned char> rgbData;
    int frameWidth = 0;
    int frameHeight = 0;
    int x = 0;
    int y = 0;
    int fontSize = 0; 
    std::vector<std::pair<int, int>> contourPoints;
    std::string displayText;
    CameraWindow::Color textColor;
};

// Objective-C subclass of NSView to handle custom drawing
@interface CameraContentView : NSView
{
    CameraContentViewImpl* impl; // Pointer to C++ implementation
}
- (void)updateFrame:(const unsigned char*)data width:(int)width height:(int)height;
- (void)updateContour:(const std::vector<std::pair<int, int>>&)points;
- (void)updateText:(const std::string&)text
                x:(int)x
                y:(int)y
         fontSize:(int)fontSize
            color:(const CameraWindow::Color&)color;
@end

@implementation CameraContentView

- (instancetype)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    if (self) {
        impl = new CameraContentViewImpl();
    }
    return self;
}

- (void)dealloc {
    delete impl;
    [super dealloc];
}

- (void)updateFrame:(const unsigned char*)data width:(int)width height:(int)height {
    impl->rgbData.assign(data, data + (width * height * 3));
    impl->frameWidth = width;
    impl->frameHeight = height;
    [self setNeedsDisplay:YES];
}

- (void)updateContour:(const std::vector<std::pair<int, int>>&)points {
    impl->contourPoints = points;
    [self setNeedsDisplay:YES];
}

- (void)updateText:(const std::string&)text
                x:(int)x
                y:(int)y
         fontSize:(int)fontSize
            color:(const CameraWindow::Color&)color {
    impl->displayText = text;
    impl->textColor = color;
    impl->x = x;
    impl->y = y;
    impl->fontSize = fontSize;
    impl->textColor = color;
    [self setNeedsDisplay:YES];
}

- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];

    NSRect bounds = [self bounds];
    CGContextRef context = [[NSGraphicsContext currentContext] CGContext];
    if (impl->rgbData.empty() || impl->frameWidth == 0 || impl->frameHeight == 0) {
        return;
    }

    // Calculate scaling factors
    CGFloat scaleX = bounds.size.width / impl->frameWidth;
    CGFloat scaleY = bounds.size.height / impl->frameHeight;

    // Optionally, maintain aspect ratio by using the minimum scale factor
    CGFloat scale = MIN(scaleX, scaleY);

    // Calculate offsets to center the content if aspect ratios differ
    CGFloat offsetX = (bounds.size.width - (impl->frameWidth * scale)) / 2.0;
    CGFloat offsetY = (bounds.size.height - (impl->frameHeight * scale)) / 2.0;

    CGContextSaveGState(context); // Save state before transformations

    // Apply scaling and translation
    CGContextTranslateCTM(context, offsetX, offsetY);
    CGContextScaleCTM(context, scale, scale);

    // Draw the RGB frame
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGDataProviderRef provider = CGDataProviderCreateWithData(NULL, impl->rgbData.data(), impl->rgbData.size(), NULL);
    CGImageRef image = CGImageCreate(impl->frameWidth, impl->frameHeight, 8, 24, impl->frameWidth * 3, colorSpace, kCGBitmapByteOrderDefault | kCGImageAlphaNone, provider, NULL, false, kCGRenderingIntentDefault);

    CGRect rect = CGRectMake(0, 0, impl->frameWidth, impl->frameHeight);
    CGContextDrawImage(context, rect, image);

    CGImageRelease(image);
    CGDataProviderRelease(provider);
    CGColorSpaceRelease(colorSpace);

    if (!impl->contourPoints.empty()) {
        CGContextSaveGState(context); // Save state before drawing contour

        CGContextSetLineWidth(context, 3.0 / scale); // Adjust line width based on scale
        CGContextSetStrokeColorWithColor(context, [[NSColor yellowColor] CGColor]); // Changed color to yellow

        // Move to the first point
        auto firstPoint = impl->contourPoints[0];
        CGContextMoveToPoint(context, firstPoint.first, impl->frameHeight - firstPoint.second);

        // Draw lines to subsequent points
        for (size_t i = 1; i < impl->contourPoints.size(); ++i) {
            auto point = impl->contourPoints[i];
            CGContextAddLineToPoint(context, point.first, impl->frameHeight - point.second);
        }

        CGContextClosePath(context);
        CGContextStrokePath(context);

        CGContextRestoreGState(context); // Restore state after drawing contour

        impl->contourPoints.clear(); // Clear the points after drawing
    }

    CGContextRestoreGState(context); // Restore state after transformations

    // Draw Text
    if (!impl->displayText.empty()) {
        CGContextSaveGState(context); // Save state before drawing text
    
        // Calculate scaled position
        CGFloat scaledX = impl->x * scale + offsetX;
        CGFloat scaledY = impl->y * scale + offsetY;

        NSColor *color = [NSColor colorWithRed:impl->textColor.r / 255.0 green:impl->textColor.g / 255.0 blue:impl->textColor.b / 255.0 alpha:1.0];
        // NSLog(@"Color: R:%f G:%f B:%f", impl->textColor.r, impl->textColor.g, impl->textColor.b);
        NSDictionary *attributes = @{
            NSFontAttributeName : [NSFont systemFontOfSize:impl->fontSize * scale],
            NSForegroundColorAttributeName : color
        };

        NSPoint point = NSMakePoint(scaledX, bounds.size.height - scaledY - (impl->fontSize * scale));// Adjust y for coordinate system
        NSString *nsText = [NSString stringWithUTF8String:impl->displayText.c_str()];
        [nsText drawAtPoint:point withAttributes:attributes];

        CGContextRestoreGState(context); // Restore state after drawing text

        impl->displayText.clear(); // Clear the text after drawing
    }
}

@end

// Delegate to handle window events
@interface CameraWindowDelegate : NSObject <NSWindowDelegate>
@end

@implementation CameraWindowDelegate
- (BOOL)windowShouldClose:(id)sender {
    [NSApp terminate:nil];
    return YES;
}
@end

// Implementation of CameraWindow class
CameraWindow::CameraWindow(int width, int height, const char *title)
    : width(width), height(height), nsWindow(nullptr), contentView(nullptr) {

        this->title = new char[strlen(title) + 1];
        strcpy(this->title, title);
    }

CameraWindow::~CameraWindow() {
    delete[] title;
    
    if (contentView) {
        CameraContentView *cv = (__bridge CameraContentView *)contentView;
        [cv removeFromSuperview];
    }

    if (nsWindow) {
        NSWindow *window = (__bridge NSWindow *)nsWindow;
        [window close];
    }
}

bool CameraWindow::Create() {
    @autoreleasepool {
        // Initialize the application if not already running
        if (NSApp == nil) {
            [NSApplication sharedApplication];
            [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
            [NSApp finishLaunching];
        }

        // Create the window
        NSRect contentRect = NSMakeRect(100, 100, width, height);
        NSUInteger styleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable;
        NSWindow *window = [[NSWindow alloc] initWithContentRect:contentRect
                                                       styleMask:styleMask
                                                         backing:NSBackingStoreBuffered
                                                           defer:NO];
        if (!window) {
            return false;
        }

        [window setTitle:[NSString stringWithUTF8String:title]];
        [window makeKeyAndOrderFront:nil];

        // Initialize the custom content view
        CameraContentView *cv = [[CameraContentView alloc] initWithFrame:contentRect];
        [window setContentView:cv];
        contentView = cv;

        // Set the delegate to handle window events
        CameraWindowDelegate *delegate = [[CameraWindowDelegate alloc] init];
        [window setDelegate:delegate];

        nsWindow = (void *)window;
        return true;
    }
}

void CameraWindow::Show() {
    @autoreleasepool {
        [NSApp activateIgnoringOtherApps:YES];
    }
}

bool CameraWindow::WaitKey(char key)
{
    @autoreleasepool {
        // Poll for *any* event quickly, instead of blocking forever
        NSEvent *event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                           untilDate:[NSDate distantPast]
                                              inMode:NSDefaultRunLoopMode
                                             dequeue:YES];
        if (event) {
            // Dispatch the event so the window can update, move, resize, etc.
            [NSApp sendEvent:event];

            // Check if it’s a keydown event
            if (event.type == NSEventTypeKeyDown) {
                NSString *characters = [event charactersIgnoringModifiers];
                if ([characters length] > 0) {
                    char pressedKey = [characters characterAtIndex:0];
                    if (key == '\0' || pressedKey == key || pressedKey == std::toupper(key)) {
                        return false;  // Return false to break your while-loop
                    }
                }
            }
        }
        // If no event was found, just continue
        return true;
    }
}

void CameraWindow::ShowFrame(const unsigned char *rgbData, int frameWidth, int frameHeight) {
    if (contentView) {
        [contentView updateFrame:rgbData width:frameWidth height:frameHeight];
    }
}

void CameraWindow::DrawContour(const std::vector<std::pair<int, int>> &points) {
    if (contentView) {
        [contentView updateContour:points];
    }
}

void CameraWindow::DrawText(const std::string &text, int x, int y, int fontSize, const CameraWindow::Color &color) {
    if (contentView) {
        [contentView updateText:text x:x y:y fontSize:fontSize color:color];
    }
}
