# Copyright (c) 2020 FlyingSamson.
# SpaceMouseTool is released under the terms of the AGPLv3 or higher.

from distutils.core import setup, Extension
import os
import platform

os.environ['CC'] = 'g++'

debug = True

undefList = []
if debug:
    undefList = ['NDEBUG']

spacemouse_link_args = []
spacemouse_static_libs = []
spacemouse_compiler_args = ['-std=c++11', '-DWITH_SPACEMOUSE']
spacemouse_include_args = ['.']
spacemouse_libraries = []
spacemouse_lib_dirs = []
extension_dir = ""

libdir = os.path.join("..", "lib")

system = platform.system()
if system == "Darwin":
    spacemouse_link_args = ['-F', '/Library/Frameworks/', '-framework', '3DconnexionClient']
    spacemouse_compiler_args.extend(['-F', '/Library/Frameworks/', '-DWITH_LIB3DX'])
    if platform.machine() == "arm64":
        libdir = os.path.join(libdir, "darwin_arm64")
    else:
        libdir = os.path.join(libdir, "darwin_x86_64")
elif system == "Linux":
    libdir = os.path.join(libdir, "linux")
    spacemouse_static_libs = ['/usr/lib/libspnav.a']
    spacemouse_compiler_args.extend(['-DWITH_LIBSPACENAV', '-DWITH_DAEMONSPACENAV'])
elif system == "Windows":
    libdir = os.path.join(libdir, "windows")
    spacemouse_compiler_args.extend(['-DWITH_LIB3DX_WIN'])
    spacemouse_libraries.extend(['siapp'])
    spacemouse_lib_dirs.extend(["C:\\Program Files (x86)\\3Dconnexion\\3DxWare SDK\\Lib\\x64"])
    spacemouse_include_args.extend(["C:\\Program Files (x86)\\3Dconnexion\\3DxWare SDK\\Inc"])


module = Extension('pyspacemouse',
                   language='c++',
                   extra_compile_args=spacemouse_compiler_args,
                   sources=['PySpaceMouse.cpp',
                            'SpaceMouse.cpp'],
                   include_dirs=spacemouse_include_args,
                   extra_link_args=spacemouse_link_args,
                   extra_objects=spacemouse_static_libs,
                   libraries=spacemouse_libraries,
                   library_dirs=spacemouse_lib_dirs,
                   undef_macros=undefList
                   )

setup(name='PackageName',
      version=1.0,
      description='First test',
      ext_modules=[module],
      options={
        'build': {'build_lib': libdir}
      })
