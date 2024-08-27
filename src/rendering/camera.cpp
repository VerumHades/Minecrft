#include <rendering/camera.hpp>

void Camera::resizeScreen(int width, int height, float FOV){
    this->screenWidth = width;
    this->screenHeight = height;
    this->FOV = FOV;
    this->aspect = (float) this->screenWidth / (float) this->screenHeight;

    this->projectionMatrix = glm::perspective(glm::radians(FOV), aspect, zNear, zFar);
    //this->viewMatrix = glm::mat4(1.0f);
    //this->modelMatrix = glm::mat4(1.0f);

    updateUniforms();
}

void Camera::adjustFOV(float FOV){
    this->FOV = FOV;

    this->projectionMatrix = glm::perspective(glm::radians(FOV), aspect, zNear, zFar);
    updateUniforms();
}

void Camera::calculateFrustum(){
    const float halfVSide = zFar * tanf(FOV * .5f);
    const float halfHSide = halfVSide * aspect;
    const glm::vec3 frontMultFar = zFar * this->direction;

    glm::vec3 CamRight = glm::normalize(glm::cross(this->direction, glm::vec3(0.0f, 1.0f, 0.0f)));
    glm::vec3 CamUp = glm::normalize(glm::cross(CamRight, this->direction));

    frustum.nearFace = { this->position + zNear * this->direction, this->direction };
    frustum.farFace = { this->position + frontMultFar, -this->direction };
    frustum.rightFace = { this->position,
                            glm::cross(frontMultFar - CamRight * halfHSide, CamUp) };
    frustum.leftFace = { this->position,
                            glm::cross(CamUp,frontMultFar + CamRight  * halfHSide) };
    frustum.topFace = { this->position,
                            glm::cross(CamRight, frontMultFar - CamUp * halfVSide) };
    frustum.bottomFace = { this->position,
                            glm::cross(frontMultFar + CamUp * halfVSide, CamRight) };
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
    if(this->programs.find(this->currentProgram) != this->programs.end()) this->programs[this->currentProgram].use();
}

void Camera::setModelPosition(float x, float y, float z){
    this->modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(x,y,z));
}

void Camera::setPosition(float x, float y, float z) {
    this->position = glm::vec3(x, y, z);
    this->viewMatrix = glm::lookAt(this->position, this->position + this->direction, this->up);
    calculateFrustum();
}

void Camera::setRotation(float pitch, float yaw) {
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

    this->direction = glm::normalize(front);
    this->viewMatrix = glm::lookAt(this->position, this->position + this->direction, this->up);
    calculateFrustum();
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
    this->useProgram("skybox");
    
    this->skybox = std::make_unique<GLSkybox>(paths);
}

ShaderProgram& Camera::getProgram(std::string name){
    if(this->programs.find(name) == this->programs.end()){
        throw std::runtime_error("Failed to find program: " + name);
    }

    return this->programs[name];
}
void Camera::useProgram(std::string name){
    if(this->programs.find(name) == this->programs.end()){
        throw std::runtime_error("Failed to find program: " + name);
    }

    if(this->currentProgram == name) return;
    this->currentProgram = name;
    this->programs[name].use();
}

void Camera::drawSkybox(){
    if((this->programs.find("skybox") == programs.end()) && skybox){
        std::cout << "Skybox uninitialized" << std::endl;
        return;
    }
    glDisable(GL_CULL_FACE);
    this->useProgram("skybox");
    this->skybox->draw();
    glEnable(GL_CULL_FACE);
}
