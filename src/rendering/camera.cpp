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
}

void PerspectiveCamera::adjustFOV(float fov){
    this->projectionMatrix = glm::perspective<float>(glm::radians(fov), aspect, zNear, zFar);
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

void PerspectiveCamera::initialize(std::vector<std::reference_wrapper<ShaderProgram>> programs){
    for(int i = 0;i < programs.size();i++){
        projectionMatrix.attach(programs[i].get());
        viewMatrix.attach(programs[i].get());
        modelMatrix.attach(programs[i].get());
    }
}

void PerspectiveCamera::setModelPosition(const glm::vec3& position_){
    this->modelMatrix = glm::translate(glm::mat4(1.0f), position_);
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



void DepthCamera::updateProjection(){
    float size = 140;

    this->projectionMatrix = glm::ortho(-size, size, -size, size, this->zNear, this->zFar); 
    this->viewMatrix = glm::lookAt(this->position, this->target, glm::vec3(0,1,0));
    this->lightSpaceMatrix = this->projectionMatrix.getValue() * this->viewMatrix.getValue(); 
}

void DepthCamera::initialize(){
    program.initialize();
    program.addShader("shaders/depth.vs", GL_VERTEX_SHADER);
    program.addShader("shaders/depth.fs", GL_FRAGMENT_SHADER);
    program.compile();
    program.use();

    modelMatrix.attach(program);
    lightSpaceMatrix.attach(program);

    glGenFramebuffers(1, &depthMapFBO);  

    texture = std::make_unique<GLDepthTexture>(SHADOW_WIDTH, SHADOW_HEIGHT);

    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture->getID(), 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);  
}

void DepthCamera::prepareForRender(){
    program.updateUniforms();
    program.use();
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
}

void DepthCamera::setModelPosition(const glm::vec3& position_){
    this->modelMatrix = glm::translate(glm::mat4(1.0f), position_);
}
void DepthCamera::setPosition(float x, float y, float z){
    this->position = glm::vec3(x,y,z);
}
void DepthCamera::setTarget(float x, float y, float z){
    this->target = glm::vec3(x,y,z);
}