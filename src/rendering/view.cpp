#include <standard.hpp>
#include <rendering/shaders.hpp>

void ShaderProgram::setupProjection(int width, int height, float FOV){
    this->use();

    this->projectionMatrix = glm::perspective(glm::radians(45.0f), (float) width / (float) height, 0.1f, 100.0f);
    this->viewMatrix = glm::mat4(1.0f);
    this->modelMatrix = glm::mat4(1.0f);

    this->projLoc = glGetUniformLocation(this->program, "projection");
    this->viewLoc = glGetUniformLocation(this->program, "view");
    this->modelLoc = glGetUniformLocation(this->program, "model");

    if(this->projLoc == -1  || this->viewLoc == -1){
        printf("Failed to get all uniforms for projection.\n");
        return;
    }
    // Pass the perspective projection matrix to the shader
    glUniformMatrix4fv(this->projLoc, 1, GL_FALSE, glm::value_ptr(this->projectionMatrix));
    glUniformMatrix4fv(this->viewLoc, 1, GL_FALSE, glm::value_ptr(this->viewMatrix));
    if(this->modelLoc != -1) glUniformMatrix4fv(this->modelLoc, 1, GL_TRUE, glm::value_ptr(this->modelMatrix));
}

void ShaderProgram::recalculateProjection(int width, int height, float FOV){
    if(!this->projectionSetup){
        this->setupProjection(width,height,FOV);
        return;
    }
    this->projectionMatrix = glm::perspective(glm::radians(FOV), (float)width / (float)height, 0.1f, 1000.0f);
    this->use();
    glUniformMatrix4fv(this->projLoc, 1, GL_FALSE, glm::value_ptr(this->projectionMatrix));
}

void ShaderProgram::setCameraPosition(float x, float y, float z){
    if(this->modelLoc == -1){
        printf("Impossible to move view without model matrix.\n");
        return;
    }

    this->use();
    this->cameraPosition = glm::vec3(x,y,z);
    this->modelMatrix = glm::translate(this->modelMatrix, this->cameraPosition);
    
    glUniformMatrix4fv(this->modelLoc, 1, GL_FALSE, glm::value_ptr(this->modelMatrix));
}

void ShaderProgram::setCameraRotation(int pitch, int yaw){
    this->use();

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    
    this->cameraDirection = glm::normalize(front);
    this->viewMatrix = glm::lookAt(this->cameraPosition, this->cameraPosition + this->cameraDirection, cameraUp);

    glUniformMatrix4fv(this->viewLoc, 1, GL_FALSE, glm::value_ptr(this->viewMatrix));
}