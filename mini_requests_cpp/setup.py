#!/usr/bin/env python3
from setuptools import setup, Extension
import subprocess
import sys

# 获取 Python 配置
def get_python_includes():
    try:
        result = subprocess.run(['python3-config', '--includes'], capture_output=True, text=True)
        includes = []
        for item in result.stdout.strip().split():
            if item.startswith('-I'):
                includes.append(item[2:])  # 移除 -I 前缀
        return includes
    except:
        return []

def get_python_ldflags():
    try:
        result = subprocess.run(['python3-config', '--ldflags'], capture_output=True, text=True)
        return result.stdout.strip().split()
    except:
        return []

# Boost.Python 扩展配置
ext_modules = [
    Extension(
        'mini_requests_cpp',
        sources=['lib.cpp'],
        include_dirs=[
            '/opt/homebrew/include',
        ] + get_python_includes(),
        library_dirs=[
            '/opt/homebrew/lib',
        ],
        libraries=['boost_python313', 'boost_url'],
        extra_compile_args=['-std=c++11', '-fPIC', '-Wall'],
        extra_link_args=[],
    ),
]

setup(
    name='mini_requests_cpp',
    version='0.1.0',
    description='A simple C++ extension using Boost.Python',
    author='Your Name',
    author_email='your.email@example.com',
    ext_modules=ext_modules,
    zip_safe=False,
    python_requires='>=3.8',
    classifiers=[
        'Development Status :: 3 - Alpha',
        'Intended Audience :: Developers',
        'License :: OSI Approved :: MIT License',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.8',
        'Programming Language :: Python :: 3.9',
        'Programming Language :: Python :: 3.10',
        'Programming Language :: Python :: 3.11',
        'Programming Language :: Python :: 3.12',
        'Programming Language :: C++',
    ],
)
