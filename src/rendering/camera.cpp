#include <rendering/camera.hpp>

void PerspectiveCamera::resizeScreen(int width, int height, float fov){
    this->screenWidth = width;
    this->screenHeight = height;
    this->FOV = fov;
    this->aspect = (float) this->screenWidth / (float) this->screenHeight;

    this->projectionMatrix = glm::perspective<float>(glm::radians(fov), aspect, zNear, zFar);
    //this->viewMatrix = glm::mat4(1.0f);
    //this->modelMatrix = glm::mat4(1.0f);
    calculateFrustum();
    updateUniforms();
}

void PerspectiveCamera::adjustFOV(float fov){
    this->FOV = fov;

    this->projectionMatrix = glm::perspective<float>(glm::radians(fov), aspect, zNear, zFar);
    calculateFrustum();
    updateUniforms();
}

void PerspectiveCamera::calculateFrustum(){
    const float halfVSide = zFar * tanf(FOV * .5f);
    const float halfHSide = halfVSide * aspect;
    const glm::vec3 frontMultFar = zFar * this->direction;

    glm::vec3 CamRight = glm::normalize(glm::cross(this->direction, this->up));
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

PerspectiveCamera::PerspectiveCamera(){
    this->resizeScreen(1920,1080,90);
}

void PerspectiveCamera::updateUniforms(){
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

void PerspectiveCamera::setModelPosition(float x, float y, float z){
    this->modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(x,y,z));
}

void PerspectiveCamera::setPosition(glm::vec3 pos) {
    this->position = pos;
    this->viewMatrix = glm::lookAt(this->position, this->position + this->direction, this->up);
    calculateFrustum();
}

void PerspectiveCamera::setRotation(float pitch_, float yaw_){
    this->pitch = pitch_;
    this->yaw = yaw_;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

    this->direction = glm::normalize(front);
    this->viewMatrix = glm::lookAt(this->position, this->position + this->direction, this->up);
    calculateFrustum();
}

void PerspectiveCamera::addShader(std::string name, std::string vertex, std::string fragment){
    ShaderProgram& program = this->programs[name];

    program.initialize();
    program.addShader(vertex.c_str(), GL_VERTEX_SHADER);
    program.addShader(fragment.c_str(), GL_FRAGMENT_SHADER);
    program.compile();
}
void PerspectiveCamera::addSkybox(std::string vertex, std::string fragment, std::array<std::string,6> paths){
    this->addShader("skybox", vertex, fragment);
    this->getProgram("skybox").makeSkybox();
    this->useProgram("skybox");
    
    this->skybox = std::make_unique<GLSkybox>(paths);
}

ShaderProgram& PerspectiveCamera::getProgram(std::string name){
    if(this->programs.find(name) == this->programs.end()){
        throw std::runtime_error("Failed to find program: " + name);
    }

    return this->programs[name];
}
void PerspectiveCamera::useProgram(std::string name){
    if(this->programs.find(name) == this->programs.end()){
        throw std::runtime_error("Failed to find program: " + name);
    }

    if(this->currentProgram == name) return;
    this->currentProgram = name;
    this->programs[name].use();
}

void PerspectiveCamera::drawSkybox(){
    if((this->programs.find("skybox") == programs.end()) && skybox){
        std::cout << "Skybox uninitialized" << std::endl;
        return;
    }
    glDisable(GL_CULL_FACE);
    this->useProgram("skybox");
    this->skybox->draw();
    glEnable(GL_CULL_FACE);
}



void DepthCamera::updateProjection(){
    float size = 256;

    this->projectionMatrix = glm::ortho(-size, size, -size, size, this->zNear, this->zFar); 
    this->viewMatrix = glm::lookAt(this->position, this->target, glm::vec3(0,1,0));
    this->lightSpaceMatrix = this->projectionMatrix * this->viewMatrix; 

    updateUniforms();
}

void DepthCamera::updateUniforms(){
    unsigned int loc = glGetUniformLocation(program.getID(), "lightSpaceMatrix");
    
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(this->lightSpaceMatrix));
    glUniformMatrix4fv(program.getModelLoc(), 1, GL_FALSE, glm::value_ptr(this->modelMatrix));
}
void DepthCamera::initialize(){
    program.initialize();
    program.addShader("shaders/depth.vs", GL_VERTEX_SHADER);
    program.addShader("shaders/depth.fs", GL_FRAGMENT_SHADER);
    program.compile();

    glGenFramebuffers(1, &depthMapFBO);  

    texture = std::make_unique<GLDepthTexture>(SHADOW_WIDTH, SHADOW_HEIGHT);

    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture->getID(), 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);  
}

void DepthCamera::prepareForRender(){
    program.use();
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
}

void DepthCamera::setModelPosition(float x, float y, float z){
    this->modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(x,y,z));
}
void DepthCamera::setPosition(float x, float y, float z){
    this->position = glm::vec3(x,y,z);
}
void DepthCamera::setTarget(float x, float y, float z){
    this->target = glm::vec3(x,y,z);
}