#! /usr/bin/python3

import os, platform, argparse, shutil

parser = argparse.ArgumentParser()
parser.add_argument('-r', '--remake', action='store_true')
parser.add_argument('-d', '--debug', action='store_true')
parser.add_argument('-c', '--clear-build', action='store_true')
parser.add_argument('-w', '--with-debuger', action='store_true')
parser.add_argument('-b', '--build-type', default=None)

args = parser.parse_args()
root_director = os.getcwd()

build_types_valid = [
    "Release",
    "Debug",
    "RelWithDebInfo",
    "MinSizeRel"
]

build_type = "Debug" if args.debug else "Release"

if not args.build_type is None:
    if args.build_type in build_types_valid:
        build_type = args.build_type
    else:
        print(f"Invalid build type: {args.build_type}")
        exit()

build_directory = os.path.join("build",build_type)

if not os.path.isdir("build"):
    os.mkdir("build")
    
if not os.path.isdir(build_directory):
    os.mkdir(build_directory)
elif args.clear_build:
    shutil.rmtree(build_directory)
    os.mkdir(build_directory)  
    
os.chdir(build_directory)
platform_is_windows = platform.system().lower() == "windows"

def os_command(linux,windows, prefix = ""):
    if platform_is_windows:
        os.system(prefix + windows)
    else:
        os.system(prefix + linux)


if args.remake:
    os_command(
        f"cmake ../.. -DCMAKE_BUILD_TYPE={build_type}",
        f"cmake ../.. -DCMAKE_COLOR_MAKEFILE=ON -G 'Unix Makefiles' -DCMAKE_BUILD_TYPE={build_type}"
    )

os.chdir(root_director)

prefix = "gdb -ex run " if args.debug or args.with_debuger else "" 

build_exit_code = os.system(f"cmake --build build/{build_type} -j 8")
if build_exit_code != 0:
    print("Build failed.")
    exit()
    
os_command(
    f"./build/{build_type}/main",
    f"run build/{build_type}/main.exe",
    prefix = prefix
)
    
    

#platform_command("clear","cls")

