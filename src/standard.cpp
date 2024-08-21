#include <standard.hpp>

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