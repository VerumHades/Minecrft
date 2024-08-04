#ifndef STANDARD
#define STANDARD

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <string.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include <list.h>

#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifdef __linux__
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#else
#define WIN32_LEAN_AND_MEAN 
#include <windows.h>
#include <direct.h> // for _chdir
#include <tchar.h>
#endif

#ifdef _WIN32
#define PATH_SEPARATOR "\\"
#define PATH_SEPARATOR_CHAR '\\'
#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#define OS_NAME "windows"
#else
#define PATH_SEPARATOR "/"
#define PATH_SEPARATOR_CHAR '/'
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define OS_NAME "linux"
#endif

#define PATH_JOIN2(a,b) a PATH_SEPARATOR b
#define PATH_JOIN3(a,b,c) a PATH_SEPARATOR b PATH_SEPARATOR c


#define arrayLen(array) sizeof(array)/sizeof(array[0])
#define PRINT_DEPTH(depth) for(int i = 0;i < depth;i++) printf("    ");

void printStringSegment(char* data, int start, int end);
char* allocateString(char* string);

void dirToFilepath(const char *filename);
int check_and_create_directory(const char* dirname);
void walkPath(const char* dirname, List* files);
int directoryExists(const char *path);

char* getRootPath();
char* readFilename(char* filename);
char* readFile(FILE* fp);
int clampAngle(int angle);

#endif