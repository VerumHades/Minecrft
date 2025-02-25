import os,shutil
from distutils.dir_util import copy_tree

if not os.path.exists("releases"):
    os.mkdir("releases")

os.chdir("build/Release")
os.system("cmake --install . --config Release --prefix ../../releases")
os.chdir("../..")

copy_tree("resources", "releases/resources")
shutil.copyfile("build/Release/main.exe", "releases/main.exe")
shutil.make_archive("release", 'zip', "releases")