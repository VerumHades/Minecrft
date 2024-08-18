#ifndef STANDARD
#define STANDARD

#include <cstdlib>
#include <string>
#include <iostream>

#include <string>

#include <iostream>
#include <cstdlib>
#include <string>


#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


#define PATH_JOIN2(a,b) a PATH_SEPARATOR b
#define PATH_JOIN3(a,b,c) a PATH_SEPARATOR b PATH_SEPARATOR c


#define arrayLen(array) sizeof(array)/sizeof(array[0])

char* readFilename(char* filename);
char* readFile(FILE* fp);
int clampAngle(int angle);

#endif