"""
Sentinel Core - Setup Script

This setup.py uses CMake to build the C++ extension module.
For most use cases, use: pip install .
"""

import os
import sys
import subprocess
from pathlib import Path

from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext


class CMakeExtension(Extension):
    """CMake extension that doesn't require sources."""
    
    def __init__(self, name: str, sourcedir: str = "") -> None:
        super().__init__(name, sources=[])
        self.sourcedir = os.fspath(Path(sourcedir).resolve())


class CMakeBuild(build_ext):
    """Custom build_ext that runs CMake."""
    
    def build_extension(self, ext: CMakeExtension) -> None:
        # Determine output directory
        ext_fullpath = Path.cwd() / self.get_ext_fullpath(ext.name)
        extdir = ext_fullpath.parent.resolve()
        
        # CMake configuration arguments
        cmake_args = [
            f"-DCMAKE_LIBRARY_OUTPUT_DIRECTORY={extdir}{os.sep}",
            f"-DPYTHON_EXECUTABLE={sys.executable}",
            f"-DCMAKE_BUILD_TYPE=Release",
            f"-DSENTINEL_USE_AVX2=ON",
        ]
        
        # Build arguments
        build_args = ["--config", "Release"]
        
        # Parallel build
        if "CMAKE_BUILD_PARALLEL_LEVEL" not in os.environ:
            build_args += [f"-j{os.cpu_count() or 1}"]
        
        # Create build directory
        build_temp = Path(self.build_temp) / ext.name
        build_temp.mkdir(parents=True, exist_ok=True)
        
        # Run CMake configure
        subprocess.run(
            ["cmake", ext.sourcedir] + cmake_args,
            cwd=build_temp,
            check=True,
        )
        
        # Run CMake build
        subprocess.run(
            ["cmake", "--build", "."] + build_args,
            cwd=build_temp,
            check=True,
        )


setup(
    ext_modules=[CMakeExtension("sentinel_core")],
    cmdclass={"build_ext": CMakeBuild},
    zip_safe=False,
)
