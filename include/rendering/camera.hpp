#ifndef CAMERA_H
#define CAMERA_H

#include <optional>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>
#include <memory>

#include <rendering/opengl/texture.hpp>
#include <rendering/culling.hpp>

class PerspectiveCamera;

struct Volume
{
    virtual bool isOnFrustum(PerspectiveCamera& camera) const = 0;
};

#include <rendering/opengl/shaders.hpp>

class Camera{
    public:
        //~Camera() {std::cout << "Camera destroyed:" << this << std::endl;}
        virtual void setModelPosition(const glm::vec3& position) = 0;
        virtual void setModelRotation(const glm::vec3& rotation) = 0;
        virtual glm::vec3& getPosition() = 0;
        virtual bool isVisible(Volume& volume) = 0;
};

class PerspectiveCamera: public Camera{
    private:
        Uniform<glm::mat4> projectionMatrix;
        Uniform<glm::mat4> viewMatrix;
        Uniform<glm::mat4> modelMatrix;

        Uniform<glm::vec3> position;
        glm::vec3 direction = glm::vec3(1,0,0);
        glm::vec3 up = glm::vec3(0,1,0);

        glm::vec3 modelPosition = glm::vec3(0);
        glm::vec3 modelRotation = glm::vec3(0);

        int screenWidth = 1920;
        int screenHeight = 1080;

        float zNear = 0.01f;
        float zFar = 10000.0f;

        float pitch = 0;
        float yaw = 0;

        float FOV = 90.0f;
        float aspect;

        Frustum frustum;
        Frustum localFrustum;

        void calculateFrustum();

    public:
        PerspectiveCamera(std::string name);
        void resizeScreen(int width, int height, float FOV);
        void adjustFOV(float FOV);

        void setModelPosition(const glm::vec3& position);
        void setModelRotation(const glm::vec3& rotation);
        void setPosition(float x, float y, float z) {setPosition(glm::vec3(x,y,z));};
        void setPosition(glm::vec3 pos);
        void setRotation(float pitch, float yaw);

        bool isVisible(Volume& volume){
            return volume.isOnFrustum(*this);
        }

        glm::vec3& getPosition() {return position.getValue();}
        glm::vec3& getDirection() {return direction;}
        glm::vec3& getUp() {return up;}
        Frustum& getFrustum() {return frustum;}
        Frustum& getLocalFrustum() {return localFrustum;}
        
        int getScreenWidth(){return screenWidth;}
        int getScreenHeight(){return screenHeight;}

        float getPitch(){return pitch;};
        float getYaw(){return yaw;};
};

class DepthCamera: public Camera{
    private:
        ShaderProgram program = ShaderProgram("shaders/graphical/depth.vs","shaders/graphical/depth.fs");
        std::unique_ptr<GLDepthTexture> texture;

        Uniform<glm::mat4> projectionMatrix;
        Uniform<glm::mat4> viewMatrix;
        Uniform<glm::mat4> lightSpaceMatrix;
        Uniform<glm::mat4> modelMatrix;

        glm::vec3 position = glm::vec3(0,0,0);
        glm::vec3 target = glm::vec3(0,0,0);

        glm::vec3 modelPosition = glm::vec3(0);
        glm::vec3 modelRotation = glm::vec3(0);
        
        uint depthMapFBO;
        const uint SHADOW_WIDTH = 1024 * 4, SHADOW_HEIGHT = 1024 * 4;
        uint depthMap;

        float size = 256;

    public:
        DepthCamera(std::string name);

        void updateProjection();
        void prepareForRender();

        void setModelPosition(const glm::vec3& position);
        void setModelRotation(const glm::vec3& rotation);

        void setPosition(float x, float y, float z);
        void setPosition(glm::vec3& pos) {this->position = pos;};
        void setTarget(float x, float y, float z);
        void setTarget(glm::vec3& t) {this->target = t;}
        bool isVisible(Volume&){return true;}
        
        glm::vec3& getPosition() {return position;}
        GLDepthTexture* getTexture() const {return texture.get();}
        ShaderProgram& getProgram() {return program;}
        
        void setCaptureSize(float size) {this->size = size;};
};
#include <game/chunk.hpp>

#endif