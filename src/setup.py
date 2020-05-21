from distutils.core import setup, Extension
import os

os.environ['CC'] = 'g++'

module = Extension( 'pyspacemouse',
                    language = 'c++',
                    extra_compile_args = ["-std=c++11", "-DWITH_SPACEMOUSE", "-DWITH_LIB3DX"],
                    sources = ['PySpaceMouse.cpp',
                               'SpaceMouse.cpp'],
                    include_dirs = ['.'],
                    extra_link_args = ['-framework', '3DconnexionClient'])

setup( name = 'PackageName',
       version = 1.0,
       description = 'First test',
       ext_modules = [module] )
