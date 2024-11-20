# CLEAN & REBUILD EVERYTHING

Delete object files and rebuild everything, sometimes it just fixes things.

# THREADS BREAK THINGS

make sure things that should be synchronized are actually synchronized, it saves hours!

# SOME GL VERSIONS MISS SOME FUNCTIONS

make sure the glad version matches the opengl version, if not you are in trouble!

# Make sure texture samplers are set to the corresponding texture units

if they arent it breaks stuff badly and is hard to find!

# Make sure you funtions return what they should

SIGKILL happenes sometimes when they dont