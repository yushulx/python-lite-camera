cmake_minimum_required(VERSION 3.15)

# Project name and version
project(CameraProject VERSION 1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Define include directories
set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)

# Define source files for the Camera library
if (WIN32)
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebug")
    else()
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded") 
    endif()

    set(LIBRARY_SOURCES
        src/CameraWindows.cpp
        src/CameraPreviewWindows.cpp
    )
elseif (UNIX AND NOT APPLE)
    set(LIBRARY_SOURCES
        src/CameraLinux.cpp
        src/CameraPreviewLinux.cpp
    )
elseif (APPLE)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64")
    # Ensure that Objective-C++ source files are compiled as Objective-C++
    set(LIBRARY_SOURCES
        src/CameraMacOS.mm
        src/CameraPreviewMacOS.mm
    )
    set_source_files_properties(src/CameraMacOS.mm src/CameraPreviewMacOS.mm PROPERTIES COMPILE_FLAGS "-x objective-c++")

    # Set main.cpp to be treated as Objective-C++
    set_source_files_properties(src/main.cpp PROPERTIES COMPILE_FLAGS "-x objective-c++")
endif()

# Define source files for the executable
set(EXECUTABLE_SOURCES
    src/main.cpp
)

# Add the Camera shared library
add_library(litecam SHARED ${LIBRARY_SOURCES})

# Set include directories for the Camera library
target_include_directories(litecam PUBLIC ${INCLUDE_DIR})

# Define the CAMERA_EXPORTS macro for the shared library
target_compile_definitions(litecam PRIVATE CAMERA_EXPORTS)

# Platform-specific dependencies for the Camera library
if (UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    if (X11_FOUND)
        target_include_directories(litecam PUBLIC ${X11_INCLUDE_DIR})
        target_link_libraries(litecam PRIVATE ${X11_LIBRARIES} pthread)
    endif()
elseif (APPLE)
    find_library(COCOA_LIBRARY Cocoa REQUIRED)
    find_library(AVFOUNDATION_LIBRARY AVFoundation REQUIRED)
    find_library(COREMEDIA_LIBRARY CoreMedia REQUIRED)
    find_library(COREVIDEO_LIBRARY CoreVideo REQUIRED)
    find_library(OBJC_LIBRARY objc REQUIRED)  # Add the Objective-C runtime library

    target_link_libraries(litecam PRIVATE 
        ${COCOA_LIBRARY} 
        ${AVFOUNDATION_LIBRARY} 
        ${COREMEDIA_LIBRARY} 
        ${COREVIDEO_LIBRARY} 
        ${OBJC_LIBRARY}  # Link the Objective-C runtime
    )
elseif (WIN32)
    target_link_libraries(litecam PRIVATE ole32 uuid mfplat mf mfreadwrite mfuuid)
endif()

# Add the camera_capture executable
add_executable(camera_capture ${EXECUTABLE_SOURCES})

# Link the Camera library to the executable
target_link_libraries(camera_capture PRIVATE litecam)

# Include the shared library's headers in the executable
target_include_directories(camera_capture PRIVATE ${INCLUDE_DIR})

# For macOS, link against the Cocoa framework
if (APPLE)
    target_link_libraries(camera_capture PRIVATE 
        ${COCOA_LIBRARY} 
        ${AVFOUNDATION_LIBRARY} 
        ${COREMEDIA_LIBRARY} 
        ${COREVIDEO_LIBRARY} 
        ${OBJC_LIBRARY}  # Link the Objective-C runtime
    )
endif()
