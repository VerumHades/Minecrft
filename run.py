#! /usr/bin/python3

import os, platform, argparse, shutil, time, subprocess

parser = argparse.ArgumentParser()
parser.add_argument('-r', '--remake', action='store_true')

parser.add_argument('-d', '--debug', action='store_true', help="Build is Debug and runs gdb")
parser.add_argument('-w', '--with-debuger', action='store_true', help="Runs gdb")

parser.add_argument('-c', '--clear-build', action='store_true', help="Clears the build directory")
parser.add_argument(
    '-b', '--build-type', 
    default="Release", 
    help="Allows you to specify the build type. Ignored if -d is specified",
    choices = ["Release","Debug","RelWithDebInfo","MinSizeRel"]
)

parser.add_argument('-n', '--no-run', action='store_true', help="Builds but doesnt run")
parser.add_argument('-p', '--package', action='store_true', help="Builds and creates and installer in build directory")

args = parser.parse_args()
root_director = os.getcwd()

build_type = "Debug" if args.debug else args.build_type
build_directory = os.path.join("build", build_type)

print(f"Selected build type: {build_type}")
print(f"Selected build directory: {build_directory}")

os.makedirs(build_directory, exist_ok=True)
    
if args.clear_build:
    shutil.rmtree(build_directory)
    os.makedirs(build_directory, exist_ok=True) 
    
os.chdir(build_directory)
print("Verified build directory")

platform_is_windows = platform.system().lower() == "windows"

print(f"Platform is: {platform.system().lower()}")

def command(command):
    print(f"Running command: {command}")
    return os.system(command)

def os_command(linux,windows, prefix = ""):
    if platform_is_windows:
        return command(prefix + windows)
    else:
        return command(prefix + linux)


suffixus = "-DCMAKE_CXX_FLAGS=\"-DWINDOWS_PACKAGED_BUILD\"" if args.package else ""

if args.remake:
    print("Remaking...")
    os_command(
        f"cmake {root_director} -DCMAKE_BUILD_TYPE={build_type} {suffixus}",
        f"cmake {root_director} -DCMAKE_COLOR_MAKEFILE=ON -G \"Unix Makefiles\" -DCMAKE_BUILD_TYPE={build_type} {suffixus}"
    )

os.chdir(root_director)

prefix = "gdb -ex run " if args.debug or args.with_debuger else "" 
#prefix =  ""

start_time = time.time()

build_exit_code = command(f"cmake --build {build_directory} -j 8")
if build_exit_code != 0:
    print("Build failed.")
    exit()
    
end_time = time.time()
elapsed_time = end_time - start_time
print(f"Time to build: {elapsed_time} seconds")

if args.package:
    start_time = time.time()
    
    os.chdir(build_directory)
    
    file_path = 'CPackConfig.cmake'

    with open(file_path, 'r') as file:
            lines = file.readlines()

    with open(file_path, 'w') as file:
        for line in lines:
            if line.startswith("set(CPACK_COMPONENTS_ALL"):
                line = line.replace("-","_")
            file.write(line)
        
    os.system(f"cpack --config {file_path}")    
    
    end_time = time.time()
    elapsed_time = end_time - start_time
    print(f"Time to package: {elapsed_time} seconds")

    
elif not args.no_run:
    os_command(
        f"./{build_directory}/majnkraft",
        f"{build_directory}\\majnkraft.exe",
        prefix = prefix
    )


#platform_command("clear","cls")

