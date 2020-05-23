from distutils.core import setup, Extension
import os
import platform

os.environ['CC'] = 'g++'

spacemouse_link_args = []
spacemouse_static_libs = []
spacemouse_compiler_args = ['-std=c++11', '-DWITH_SPACEMOUSE']

system = platform.system()
if system == "Darwin":
    spacemouse_link_args = ['-framework', '3DconnexionClient']
    spacemouse_compiler_args.extend(['-DWITH_LIB3DX'])
elif system == "Linux":
    spacemouse_static_libs = ['/usr/lib/libspnav.a']
    spacemouse_compiler_args.extend(['-DWITH_LIBSPACENAV', '-DWITH_DAEMONSPACENAV'])


module = Extension('pyspacemouse',
                   language='c++',
                   extra_compile_args=spacemouse_compiler_args,
                   sources=['PySpaceMouse.cpp',
                            'SpaceMouse.cpp'],
                   include_dirs=['.'],
                   extra_link_args=spacemouse_link_args,
                   extra_objects=spacemouse_static_libs
                   )

setup(name='PackageName',
      version=1.0,
      description='First test',
      ext_modules=[module])
