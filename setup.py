import platform
import sys
from distutils.core import Extension, setup


def main():

    extra_objects = []
    include_dirs = []

    if sys.platform == "darwin":
        define_macros=[('FITPIX_LIBFTDI', '1')]
        if platform.machine() == "arm64":
            include_dirs=["ftdi/mac/arm64"]
            extra_objects=["ftdi/mac/arm64/libftdi1.a", "ftdi/mac/arm64/libusb-1.0.a"]
        else:
            include_dirs=["ftdi/mac/x64"]
            extra_objects=["ftdi/mac/x64/libftdi1.a", "ftdi/mac/x64/libusb-1.0.a"]


    if sys.platform == "linux":
        define_macros=[('FITPIX_LIBFTDI', '1')]
        include_dirs=["ftdi/linux"]
        extra_objects=["ftdi/linux/libftdi1.a", "ftdi/linux/libusb-1.0.a"]

    if sys.platform == "win32":
        define_macros=[('WIN32', '1')]
        include_dirs=["ftdi/win"]
        extra_objects=["ftdi/win/ftd2xx_vc_x64.lib"]


    setup(name="py_ftdi",
            version="1.1.3",
            description="FTDI library",
            author="Daniel Turecek",
            author_email="daniel@turecek.de",
            packages=["py_ftdi"],
            package_dir={"py_ftdi": "py_ftdi"},
            package_data={"py_ftdi": [
                "__init__.pyi",
                "py.typed",
            ]},
            include_package_data=True,
            ext_modules=[
                 Extension(
                    "py_ftdi",
                    sources=["py_ftdi/py_ftdi.cpp",
                             "py_ftdi/ftdidev.cpp" ],
                    define_macros=define_macros,
                    include_dirs=include_dirs,
                    extra_objects=extra_objects
                )
            ])

if __name__ == "__main__":
    main()
