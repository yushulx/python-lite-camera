from setuptools.command import build_ext
from setuptools import setup, Extension
import sys
import os
import io
from setuptools.command.install import install
import shutil
from pathlib import Path

# Define platform-specific source files
def get_platform_sources():
    base_sources = [
        "src/litecam.cpp",  # Python binding
    ]
    
    # Add platform-specific camera implementation sources
    if sys.platform == "linux" or sys.platform == "linux2":
        # Linux
        platform_sources = [
            "lib/litecam/src/CameraLinux.cpp",
            "lib/litecam/src/CameraPreviewLinux.cpp",
        ]
    elif sys.platform == "darwin":
        # MacOS - use .cpp files (copied from .mm files)
        # Note: We copy .mm to .cpp because setuptools doesn't handle .mm files
        platform_sources = [
            "lib/litecam/src/CameraMacOS.cpp",
            "lib/litecam/src/CameraPreviewMacOS.cpp",
        ]
    elif sys.platform == "win32":
        # Windows
        platform_sources = [
            "lib/litecam/src/CameraWindows.cpp",
            "lib/litecam/src/CameraPreviewWindows.cpp",
        ]
    else:
        raise RuntimeError("Unsupported platform")
    
    return base_sources + platform_sources

# Define platform-specific compile and link arguments
def get_platform_config():
    include_dirs = [
        os.path.join(os.path.dirname(__file__), "include"),
        os.path.join(os.path.dirname(__file__), "lib/litecam/include"),
    ]
    
    libraries = []
    library_dirs = []
    extra_compile_args = []
    extra_link_args = []
    define_macros = [('CAMERA_EXPORTS', None)]  # Define for building the library
    
    if sys.platform == "linux" or sys.platform == "linux2":
        # Linux
        extra_compile_args = ['-std=c++17', '-fPIC']
        libraries = ['X11', 'pthread']
        extra_link_args = ["-Wl,-rpath=$ORIGIN"]
        
    elif sys.platform == "darwin":
        # MacOS
        extra_compile_args = ['-std=c++17', '-ObjC++']
        libraries = []  # Will be handled by extra_link_args
        extra_link_args = [
            "-framework", "Cocoa",
            "-framework", "AVFoundation", 
            "-framework", "CoreMedia",
            "-framework", "CoreVideo",
            "-lobjc",
            "-Wl,-rpath,@loader_path"
        ]
        
    elif sys.platform == "win32":
        # Windows
        extra_compile_args = ['/std:c++17']
        libraries = ['ole32', 'uuid', 'mfplat', 'mf', 'mfreadwrite', 'mfuuid', 'user32', 'gdi32']
        extra_link_args = []
        
    else:
        raise RuntimeError("Unsupported platform")
    
    return {
        'include_dirs': include_dirs,
        'libraries': libraries,
        'library_dirs': library_dirs,
        'extra_compile_args': extra_compile_args,
        'extra_link_args': extra_link_args,
        'define_macros': define_macros,
    }

# Ensure .mm files are copied to .cpp files before building (macOS only)
if sys.platform == "darwin":
    mm_files = [
        "lib/litecam/src/CameraMacOS.mm",
        "lib/litecam/src/CameraPreviewMacOS.mm",
    ]
    
    for mm_file in mm_files:
        cpp_file = mm_file.replace('.mm', '.cpp')
        
        # Always copy .mm to .cpp to ensure they're up to date
        if os.path.exists(mm_file):
            print(f"Copying {mm_file} to {cpp_file}")
            shutil.copy2(mm_file, cpp_file)
        else:
            raise RuntimeError(f"Required source file {mm_file} not found")

# Get platform-specific configuration
sources = get_platform_sources()
config = get_platform_config()

long_description = io.open("README.md", encoding="utf-8").read()

module_litecam = Extension(
    "litecam",
    sources=sources,
    include_dirs=config['include_dirs'],
    library_dirs=config['library_dirs'],
    libraries=config['libraries'],
    extra_compile_args=config['extra_compile_args'],
    extra_link_args=config['extra_link_args'],
    define_macros=config['define_macros'],
    language="c++",
)


def copyfiles(src, dst):
    if os.path.isdir(src):
        filelist = os.listdir(src)
        for file in filelist:
            libpath = os.path.join(src, file)
            shutil.copy2(libpath, dst)
    else:
        shutil.copy2(src, dst)

class CustomBuildExt(build_ext.build_ext):
    def run(self):
        build_ext.build_ext.run(self)
        # No need to copy external libraries since everything is built together
        dst = os.path.join(self.build_lib, "litecam")
        
        # Create destination directory if it doesn't exist
        os.makedirs(dst, exist_ok=True)
        
        # Copy any additional files if needed
        if os.path.exists(self.build_lib):
            filelist = os.listdir(self.build_lib)
            for file in filelist:
                filePath = os.path.join(self.build_lib, file)
                if not os.path.isdir(filePath) and file.endswith(('.pyd', '.so', '.dylib')):
                    copyfiles(filePath, dst)
                    # delete file for wheel package
                    os.remove(filePath)

class CustomBuildExtDev(build_ext.build_ext):
    def run(self):
        build_ext.build_ext.run(self)
        dev_folder = os.path.join(Path(__file__).parent, 'litecam')
        
        # Create destination directory if it doesn't exist
        os.makedirs(dev_folder, exist_ok=True)
        
        filelist = os.listdir(self.build_lib)
        for file in filelist:
            filePath = os.path.join(self.build_lib, file)
            if not os.path.isdir(filePath) and file.endswith(('.pyd', '.so', '.dylib')):
                copyfiles(filePath, dev_folder)


class CustomInstall(install):
    def run(self):
        install.run(self)


setup(name='lite-camera',
      version='2.0.7',
      description='LiteCam is a lightweight, cross-platform library for capturing RGB frames from cameras and displaying them. Designed with simplicity and ease of integration in mind, LiteCam supports Windows, Linux and macOS platforms.',
      long_description=long_description,
      long_description_content_type="text/markdown",
      author='yushulx',
      url='https://github.com/yushulx/python-lite-camera',
      license='MIT',
      packages=['litecam'],
      ext_modules=[module_litecam],
      classifiers=[
           "Development Status :: 5 - Production/Stable",
           "Environment :: Console",
           "Intended Audience :: Developers",
          "Intended Audience :: Education",
          "Intended Audience :: Information Technology",
          "Intended Audience :: Science/Research",
          "License :: OSI Approved :: MIT License",
          "Operating System :: Microsoft :: Windows",
          "Operating System :: MacOS",
          "Operating System :: POSIX :: Linux",
          "Programming Language :: Python",
          "Programming Language :: Python :: 3",
          "Programming Language :: Python :: 3 :: Only",
          "Programming Language :: Python :: 3.6",
          "Programming Language :: Python :: 3.7",
          "Programming Language :: Python :: 3.8",
          "Programming Language :: Python :: 3.9",
          "Programming Language :: Python :: 3.10",
          "Programming Language :: Python :: 3.11",
          "Programming Language :: Python :: 3.12",
          "Programming Language :: C++",
          "Programming Language :: Python :: Implementation :: CPython",
          "Topic :: Scientific/Engineering",
          "Topic :: Software Development",
      ],
      cmdclass={
          'install': CustomInstall,
          'build_ext': CustomBuildExt,
          'develop': CustomBuildExtDev},
      )
