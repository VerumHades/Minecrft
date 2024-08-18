#include <standard.hpp>

char* getRootPath(){
    char* path = calloc(1,PATH_MAX);
    
    #ifdef __linux__
    ssize_t len = readlink("/proc/self/exe", path, PATH_MAX - 1) - 13;

    if (len != -1) {
        path[len] = '\0';  // Null-terminate the path
        //printf("Executable path: %s\n", path);
    } else return NULL;

    #else
    DWORD len = GetModuleFileName(NULL, path, MAX_PATH);

    if (len == 0)  return NULL;

    path[len - 17] = '\0';
    #endif

    return path;
}

void printStringSegment(char* data, int start, int end){
    for(int i = start;i < end;i++) {
        if(i < 0 || i >= strlen(data)){
            printf("ILLEGAL ACCESS (%i-%i)[%i]",start,end,i);
            return;
        }
        //if(data[i] == '\n') printf("\\n");
        printf("%c",data[i]);
    }
}

char* allocateString(char* string){
    char *test = (char*) malloc(strlen(string)*sizeof(char)+1);
    strcpy(test, string);
    return test;
}

void dirToFilepath(const char *filename) {
    char *filepath;
    char *last_separator;

    // Duplicate the filename string to avoid modifying the original
    filepath = strdup(filename);
    if (filepath == NULL) {
        perror("strdup");
        exit(EXIT_FAILURE);
    }

    // Find the last occurrence of the path separator
    last_separator = strrchr(filepath, PATH_SEPARATOR_CHAR);
    if (last_separator != NULL) {
        // Terminate the string at the last separator
        *last_separator = '\0';

#ifdef _WIN32
        // Convert relative path to absolute path on Windows
        char fullpath[MAX_PATH];
        if (_fullpath(fullpath, filepath, MAX_PATH) == NULL) {
            perror("GetFullPathName");
            free(filepath);
            exit(EXIT_FAILURE);
        }

        // Change the working directory
        if (chdir(fullpath) != 0) {
            perror("chdir");
            free(filepath);
            exit(EXIT_FAILURE);
        }
#else
        // Change the working directory on Unix-like systems
        if (chdir(filepath) != 0) {
            perror("chdir");
            free(filepath);
            exit(EXIT_FAILURE);
        }
#endif
    } else {
        // No path separator found, meaning the file is in the current directory
        //printf("File is in the current directory\n");
    }

    free(filepath);
}

int check_and_create_directory(const char *path) {
    int result = 0;

#if defined(_WIN32) || defined(_WIN64)
    struct _stat st = {0};

    // Check if the directory exists
    if (_stat(path, &st) == -1) {
        // Directory does not exist, so create it
        if (_mkdir(path) == 0) {
            result = 0; // Success
        } else {
            perror("mkdir");
            result = -1; // Failure
        }
    } else if (st.st_mode & _S_IFDIR) {
        result = 0; // Directory already exists
    } else {
        result = -1; // Path exists but is not a directory
    }
#else
    struct stat st = {0};

    // Check if the directory exists
    if (stat(path, &st) == -1) {
        // Directory does not exist, so create it
        if (mkdir(path, 0700) == 0) {
            result = 0; // Success
        } else {
            result = -1; // Failure
        }
    } else if (S_ISDIR(st.st_mode)) {
        result = 0; // Directory already exists
    } else {
        result = -1; // Path exists but is not a directory
    }
#endif

    return result;
}

int directoryExists(const char *path) {
#ifdef _WIN32
    DWORD attrib = GetFileAttributes(path);
    return (attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat statbuf;
    if (stat(path, &statbuf) != 0) {
        return 0;
    }
    return S_ISDIR(statbuf.st_mode);
#endif
}


#ifdef _WIN32
void walkPath(const char *path, List* files) {
    WIN32_FIND_DATA findFileData;
    HANDLE hFind = INVALID_HANDLE_VALUE;

    // Construct the search path
    char searchPath[MAX_PATH];
    snprintf(searchPath, sizeof(searchPath), "%s\\*", path);

    hFind = FindFirstFile(searchPath, &findFileData);

    if (hFind == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "FindFirstFile failed (%lu)\n", GetLastError());
        return;
    } 

    do {
        char full_path[MAX_PATH];

        // Skip the "." and ".." entries
        if (strcmp(findFileData.cFileName, ".") == 0 || strcmp(findFileData.cFileName, "..") == 0)
            continue;

        snprintf(full_path, sizeof(full_path), "%s\\%s", path, findFileData.cFileName);

        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            // If the entry is a directory, recurse into it
            walkPath(full_path, files);
        } else {
            // If the entry is a file, add it to the list
            addToList(files, allocateString(full_path));
        }
    } while (FindNextFile(hFind, &findFileData) != 0);

    if (GetLastError() != ERROR_NO_MORE_FILES) {
        fprintf(stderr, "FindNextFile failed (%lu)\n", GetLastError());
    }

    FindClose(hFind);
}
#else
void walkPath(const char *path, List* files) {
    struct dirent *entry;
    DIR *dp = opendir(path);

    if (dp == NULL) {
        perror("opendir");
        return;
    }

    while ((entry = readdir(dp))) {
        char full_path[1024];
        struct stat statbuf;

        // Skip the "." and ".." entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        if (stat(full_path, &statbuf) == -1) {
            perror("stat");
            continue;
        }

        if (S_ISDIR(statbuf.st_mode)) {
            // If the entry is a directory, recurse into it
            walkPath(full_path, files);
        } else {
            // If the entry is a file, add it to the list
            addToList(files, allocateString(full_path));
        }
    }

    closedir(dp);
}
#endif

char* readFilename(char* filename){
    FILE* f;
    f = fopen(filename,"r");
    if(f == NULL){
        perror("Error\n");
        return NULL;
    }

    char* data = readFile(f);
    fclose(f);
    return data;
}

char* readFile(FILE* fp){
    char *fcontent = NULL;
    int fsize = 0;

    if(fp) {
        fseek(fp, 0, SEEK_END);
        fsize = ftell(fp);
        rewind(fp);

        fcontent = (char*) calloc(fsize + 1,sizeof(char));
        fread(fcontent, 1, fsize, fp);

        /*if((int)read != (int)fsize){
            printf("%i != %i\n" ,(int)read, fsize);
            return NULL;
        }*/
        //fclose(fp);
    }
    else{
        return NULL;
    }

    fcontent[fsize] = '\0';
    return fcontent;
}

int clampAngle(int angle){
    return (angle + 360) % 360;
}