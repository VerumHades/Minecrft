#include <rendering/camera.hpp>

void Camera::resizeScreen(int width, int height, float FOV){
    this->projectionMatrix = glm::perspective(glm::radians(FOV), (float) width / (float) height, 0.1f, 1000.0f);
    //this->viewMatrix = glm::mat4(1.0f);
    //this->modelMatrix = glm::mat4(1.0f);

    updateUniforms();
}

Camera::Camera(){
    this->resizeScreen(1920,1080,90);
}

void Camera::updateUniforms(){
    for(auto& [key, program]: this->programs){
        program.use();
        glUniformMatrix4fv(program.getProjLoc(), 1, GL_FALSE, glm::value_ptr(this->projectionMatrix));
        
        if (program.isSkybox()) {
            // Strip the translation component for the skybox
            glm::mat4 view = glm::mat4(glm::mat3(this->viewMatrix));
            glUniformMatrix4fv(program.getViewLoc(), 1, GL_FALSE, glm::value_ptr(view));
        } else{
            glUniformMatrix4fv(program.getViewLoc(), 1, GL_FALSE, glm::value_ptr(this->viewMatrix));
            glUniformMatrix4fv(program.getModelLoc(), 1, GL_FALSE, glm::value_ptr(this->modelMatrix));
        }
    }
}

void Camera::setModelPosition(float x, float y, float z){
    this->modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(x,y,z));
}

void Camera::setPosition(float x, float y, float z) {
    this->position = glm::vec3(x, y, z);
    this->viewMatrix = glm::lookAt(this->position, this->position + this->direction, this->up);
}

void Camera::setRotation(float pitch, float yaw) {
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

    this->direction = glm::normalize(front);
    this->viewMatrix = glm::lookAt(this->position, this->position + this->direction, this->up);
}

void Camera::addShader(std::string name, std::string vertex, std::string fragment){
    ShaderProgram& program = this->programs[name];

    program.initialize();
    program.addShader(vertex.c_str(), GL_VERTEX_SHADER);
    program.addShader(fragment.c_str(), GL_FRAGMENT_SHADER);
    program.compile();
}
void Camera::addSkybox(std::string vertex, std::string fragment, std::array<std::string,6> paths){
    this->addShader("skybox", vertex, fragment);
    this->getProgram("skybox").makeSkybox();
    
    this->skybox = std::make_unique<GLSkybox>(paths, this->getProgram("skybox"));
}

ShaderProgram& Camera::getProgram(std::string name){
    if(this->programs.find(name) == this->programs.end()){
        throw std::runtime_error("Failed to find program: " + name);
    }

    return this->programs[name];
}

void Camera::drawSkybox(){
    if((this->programs.find("skybox") == programs.end()) && skybox){
        std::cout << "Skybox uninitialized" << std::endl;
        return;
    }
    glDisable(GL_CULL_FACE);
    this->getProgram("skybox").use();
    this->skybox->draw();
    glEnable(GL_CULL_FACE);
}