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

system = platform.system()
if system == "Darwin":
    spacemouse_link_args = ['-framework', '3DconnexionClient']
    spacemouse_compiler_args.extend(['-DWITH_LIB3DX'])
elif system == "Linux":
    spacemouse_static_libs = ['/usr/lib/libspnav.a']
    spacemouse_compiler_args.extend(['-DWITH_LIBSPACENAV', '-DWITH_DAEMONSPACENAV'])
elif system == "Windows":
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
      ext_modules=[module])
