#include <view.h>


void multiplyMatrices(float result[16], float mat1[16], float mat2[16]) {
    float temp[16];

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            temp[i * 4 + j] = 0;
            for (int k = 0; k < 4; ++k) {
                temp[i * 4 + j] += mat1[i * 4 + k] * mat2[k * 4 + j];
            }
        }
    }

    memcpy(result, temp, sizeof(temp));
}

void createPerspectiveMatrix(float* m, float fov, float aspectRatio, float near_plane, float far_plane) {
    float tanHalfFov = tan(fov / 2.0f);
    
    m[0] = 1.0f / (tanHalfFov * aspectRatio); // Column 1
    m[1] = 0.0f;
    m[2] = 0.0f;
    m[3] = 0.0f;

    m[4] = 0.0f; // Column 2
    m[5] = 1.0f / tanHalfFov;
    m[6] = 0.0f;
    m[7] = 0.0f;

    m[8] = 0.0f; // Column 3
    m[9] = 0.0f;
    m[10] = -(far_plane + near_plane) / (far_plane - near_plane);
    m[11] = -1.0f;

    m[12] = 0.0f; // Column 4
    m[13] = 0.0f;
    m[14] = -(2.0f * far_plane * near_plane) / (far_plane - near_plane);
    m[15] = 0.0f;
}

void setArray(float* dest, float* source, int size){
    for(int i = 0; i < size;i++) dest[i] = source[i];
}
void setupProjection(ShaderProgram* program, float FOV){
    /*glEnable(GL_CULL_FACE);
    // Specify which faces to cull (back faces)
    glCullFace(GL_BACK);
        // Define the winding order of front faces (default is GL_CCW - counter-clockwise)
    glFrontFace(GL_CCW);*/

    setArray(program->projectionMatrix, (float[]){
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    }, 16);
    createPerspectiveMatrix(program->projectionMatrix, (M_PI / 180.0f) * 45.0f, 1920.0 / 1080.0, 0.1f, 1000.0f);

    setArray(program->viewMatrix, (float[]){
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    }, 16);

    setArray(program->modelMatrix, (float[]){
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    }, 16);

    program->projLoc = glGetUniformLocation(program->program, "projection");
    program->viewLoc = glGetUniformLocation(program->program, "view");
    program->modelLoc = glGetUniformLocation(program->program, "model");

    if(program->projLoc == -1  || program->viewLoc == -1){
        printf("Failed to get all uniforms for projection.\n");
        return;
    }
    // Pass the perspective projection matrix to the shader
    glUniformMatrix4fv(program->projLoc, 1, GL_FALSE, program->projectionMatrix);
    glUniformMatrix4fv(program->viewLoc, 1, GL_TRUE, program->viewMatrix);
    if(program->modelLoc != -1) glUniformMatrix4fv(program->modelLoc, 1, GL_TRUE, program->modelMatrix);
}

void recalculateProjectionMatrix(ShaderProgram* program, int width, int height, float FOV){
    createPerspectiveMatrix(program->projectionMatrix, (M_PI / 180.0f) * FOV, (double)width / (double)height, 0.1f, 1000.0f);
    glUniformMatrix4fv(program->projLoc, 1, GL_FALSE, program->projectionMatrix);
}

void setViewOffset(ShaderProgram* program, float x, float y, float z){
    useShaderProgram(program);
    if(program->modelLoc == -1){
        printf("Impossible to move view without model matrix.\n");
        return;
    }

    program->modelMatrix[3] = x;
    program->modelMatrix[7] = y;
    program->modelMatrix[11] = z;
    
    glUniformMatrix4fv(program->modelLoc, 1, GL_TRUE, program->modelMatrix);
}

void setViewRotation(ShaderProgram* program, int x, int y, int z){
    useShaderProgram(program);
    float radiansX = x * M_PI / 180.0;
    float radiansY = y * M_PI / 180.0;
    float radiansZ = z * M_PI / 180.0;

    float source[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };

    float rotationMatrixX[16] = {
        1, 0, 0, 0,
        0, cos(radiansX), -sin(radiansX), 0,
        0, sin(radiansX), cos(radiansX), 0,
        0, 0, 0, 1
    };
    float rotationMatrixY[16] = {
        cos(radiansY), 0, sin(radiansY), 0,
        0, 1, 0, 0,
        -sin(radiansY), 0, cos(radiansY), 0,
        0, 0, 0, 1
    };

    float rotationMatrixZ[16] = {
        cos(radiansZ), -sin(radiansZ), 0, 0,
        sin(radiansZ), cos(radiansZ), 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };

    multiplyMatrices(source, rotationMatrixZ, source);
    multiplyMatrices(source, rotationMatrixY, source);
    multiplyMatrices(program->viewMatrix, rotationMatrixX, source);

    //view[3] = getArgument(1)->float_value;
    //view[7] = getArgument(2)->float_value;
    //view[11] = getArgument(3)->float_value;

    //printf("%u %u\n", program->viewLoc, program->program);

    glUniformMatrix4fv(program->viewLoc, 1, GL_TRUE, program->viewMatrix);
}