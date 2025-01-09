from setuptools.command import build_ext
from setuptools import setup, Extension
import sys
import os
import io
from setuptools.command.install import install
import shutil
from pathlib import Path

lib_dir = ''

sources = [
    "src/litecam.cpp",
]

include_dirs = [os.path.join(os.path.dirname(__file__), "include")]

libraries = ['litecam']

extra_compile_args = []

if sys.platform == "linux" or sys.platform == "linux2":
    # Linux
    lib_dir = 'lib/linux'
    extra_compile_args = ['-std=c++11']
    extra_link_args = ["-Wl,-rpath=$ORIGIN"]
elif sys.platform == "darwin":
    # MacOS
    lib_dir = 'lib/macos'
    # extra_compile_args = ["-x objective-c++"]
    # libraries = ["Cocoa", "AVFoundation", "CoreMedia", "CoreVideo", "objc"]
    extra_compile_args = ['-std=c++11']
    extra_link_args = ["-Wl,-rpath,@loader_path"]
elif sys.platform == "win32":
    # Windows
    lib_dir = 'lib/windows'
    extra_link_args = []

else:
    raise RuntimeError("Unsupported platform")


long_description = io.open("README.md", encoding="utf-8").read()

module_litecam = Extension(
    "litecam",
    sources=sources,
    include_dirs=include_dirs,
    library_dirs=[lib_dir],
    libraries=libraries,
    extra_compile_args=extra_compile_args,
    extra_link_args=extra_link_args,
    # language="c++",
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
        dst = os.path.join(self.build_lib, "litecam")
        copyfiles(lib_dir, dst)
        filelist = os.listdir(self.build_lib)
        for file in filelist:
            filePath = os.path.join(self.build_lib, file)
            if not os.path.isdir(file):
                copyfiles(filePath, dst)
                # delete file for wheel package
                os.remove(filePath)


class CustomBuildExtDev(build_ext.build_ext):
    def run(self):
        build_ext.build_ext.run(self)
        dev_folder = os.path.join(Path(__file__).parent, 'litecam')
        copyfiles(lib_dir, dev_folder)
        filelist = os.listdir(self.build_lib)
        for file in filelist:
            filePath = os.path.join(self.build_lib, file)
            if not os.path.isdir(file):
                copyfiles(filePath, dev_folder)


class CustomInstall(install):
    def run(self):
        install.run(self)


setup(name='lite-camera',
      version='2.0.4',
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
