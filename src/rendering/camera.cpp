#include <rendering/camera.hpp>

glm::mat4 calculateModelMatrix(glm::vec3 position, glm::vec3 rotation){
    glm::mat4 rotationX = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.x), glm::vec3(1, 0, 0));  // Rotation around X axis
    glm::mat4 rotationY = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.y), glm::vec3(0, 1, 0));  // Rotation around Y axis
    glm::mat4 rotationZ = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.z), glm::vec3(0, 0, 1));  // Rotation around Z axis

    // Combine the rotations in the correct order (commonly Z * Y * X)
    glm::mat4 modelMatrix = rotationZ * rotationY * rotationX;
    modelMatrix = glm::translate(modelMatrix, position);  // Apply the previous translation

    return modelMatrix;
}

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
    glm::vec3 CamUp    = glm::normalize(glm::cross(CamRight, this->direction));

    glm::vec3 origin = {0,0,0};

    localFrustum.nearFace   = {origin + zNear * this->direction,  this->direction                                      };
    localFrustum.farFace    = {origin + frontMultFar           , -this->direction                                      };
    localFrustum.rightFace  = {origin                          ,glm::cross(frontMultFar - CamRight * halfHSide, CamUp) };
    localFrustum.leftFace   = {origin                          ,glm::cross(CamUp,frontMultFar + CamRight  * halfHSide) };
    localFrustum.topFace    = {origin                          ,glm::cross(CamRight, frontMultFar - CamUp * halfVSide) };
    localFrustum.bottomFace = {origin                          ,glm::cross(frontMultFar + CamUp * halfVSide, CamRight) };

    glm::vec3 pos = this->position.getValue();

    frustum.nearFace   = {localFrustum.nearFace  .distance + pos, localFrustum.nearFace  .normal};
    frustum.farFace    = {localFrustum.farFace   .distance + pos, localFrustum.farFace   .normal};
    frustum.rightFace  = {localFrustum.rightFace .distance + pos, localFrustum.rightFace .normal};
    frustum.leftFace   = {localFrustum.leftFace  .distance + pos, localFrustum.leftFace  .normal};
    frustum.topFace    = {localFrustum.topFace   .distance + pos, localFrustum.topFace   .normal};
    frustum.bottomFace = {localFrustum.bottomFace.distance + pos, localFrustum.bottomFace.normal};
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
    modelPosition = position_;
    this->modelMatrix = calculateModelMatrix(position_, modelRotation);
}
void PerspectiveCamera::setModelRotation(const glm::vec3& rotation){
    modelRotation = rotation;
    this->modelMatrix = calculateModelMatrix(modelPosition, rotation);
}


void PerspectiveCamera::setPosition(glm::vec3 pos) {
    this->position = pos;
    this->viewMatrix = glm::lookAt(this->position.getValue(), this->position.getValue() + this->direction, this->up);
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
    this->viewMatrix = glm::lookAt(this->position.getValue(), this->position.getValue() + this->direction, this->up);
    calculateFrustum();
}


void DepthCamera::updateProjection(){
    this->projectionMatrix = glm::ortho(-size, size, -size, size, -size, size); 
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
    modelPosition = position_;
    this->modelMatrix = calculateModelMatrix(position_, modelRotation);
}
void DepthCamera::setModelRotation(const glm::vec3& rotation){
    modelRotation = rotation;
    this->modelMatrix = calculateModelMatrix(modelPosition, rotation);
}
void DepthCamera::setPosition(float x, float y, float z){
    this->position = glm::vec3(x,y,z);
}
void DepthCamera::setTarget(float x, float y, float z){
    this->target = glm::vec3(x,y,z);
}