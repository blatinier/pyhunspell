#!/usr/bin/env python
#-*- coding:utf-8 -*-

"""
This file is part of PyHunspell.

PyHunspell is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

PyHunspell is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with PyHunspell. If not, see <http://www.gnu.org/licenses/>.
"""

from setuptools import setup, Extension
import platform
import os
import subprocess
import re

def get_mac_include_dirs():
    cmd = "brew list --versions hunspell | tr ' ' '\n' | tail -1"
    ps = subprocess.Popen(cmd,shell=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    output = ps.communicate()[0]
    hun_ver = output.decode('utf-8')
    hun_ver = hun_ver.strip()
    hmv = re.split(r'_',hun_ver)
    hun_maj_ver = hmv[0] 
    hun_path = '/usr/local/Cellar/hunspell/' + hun_ver + '/include/hunspell'
    symlink = 'ln -s /usr/local/Cellar/hunspell/' + hun_ver + '/lib/libhunspell-' + hun_maj_ver + '.dylib /usr/local/Cellar/hunspell/' + hun_ver + '/lib/libhunspell.dylib'
    os.system(symlink)
    return hun_path

def get_linux_include_dirs():
    return ['{}/hunspell'.format(d) for d in os.getenv('INCLUDE_PATH', '').split(':') if d]

main_module_kwargs = {"sources": ['hunspell.cpp'],
                      "language": "c++"}
if platform.system() == "Windows":
    main_module_kwargs['define_macros'] = [('HUNSPELL_STATIC', None)]
    main_module_kwargs['libraries'] = ['libhunspell']
    main_module_kwargs['include_dirs'] = ['V:/hunspell-1.3.3/src/hunspell']
    main_module_kwargs['library_dirs'] = ['V:/hunspell-1.3.3/src/win_api/x64/Release/libhunspell']
    main_module_kwargs['extra_compile_args'] = ['/MT']
elif platform.system() == "Darwin":
    main_module_kwargs['define_macros'] = [('_LINUX', None)]
    main_module_kwargs['libraries'] = ['hunspell']
    main_module_kwargs['include_dirs'] = get_mac_include_dirs(),
    main_module_kwargs['extra_compile_args'] = ['-Wall']
else:
    main_module_kwargs['define_macros'] = [('_LINUX', None)]
    main_module_kwargs['libraries'] = ['hunspell']
    main_module_kwargs['include_dirs'] = get_linux_include_dirs() + ['/usr/include/hunspell']
    main_module_kwargs['extra_compile_args'] = ['-Wall']

main = Extension('hunspell', **main_module_kwargs)

setup(name="hunspell",
      version="0.5.5",
      description="Module for the Hunspell spellchecker engine",
      long_description=open('README.md').read(),
      long_description_content_type="text/markdown",  # so PyPI renders it properly
      author="Benoît Latinier",
      author_email="benoit@latinier.fr",
      url="http://github.com/blatinier/pyhunspell",
      ext_modules=[main],
      license="LGPLv3")
