#ifndef CAMERA_H
#define CAMERA_H

#include <optional>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>
#include <memory>
#include <rendering/texture.hpp>

struct Plane
{
	glm::vec3 normal = { 0.f, 1.f, 0.f }; // unit vector
	float     distance = 0.f;        // Distance with origin

	Plane() = default;

	Plane(const glm::vec3& p1, const glm::vec3& norm)
		: normal(glm::normalize(norm)),
		distance(glm::dot(normal, p1))
	{}

	float getSignedDistanceToPlane(const glm::vec3& point) const
	{
		return glm::dot(normal, point) - distance;
	}
};


struct Frustum
{
    Plane topFace;
    Plane bottomFace;

    Plane rightFace;
    Plane leftFace;

    Plane farFace;
    Plane nearFace;
};

class PerspectiveCamera;
struct Volume
{
    virtual bool isOnFrustum(PerspectiveCamera& camera) const = 0;
};

#include <rendering/shaders.hpp>

class Camera{
    public:
        //~Camera() {std::cout << "Camera destroyed:" << this << std::endl;}
        virtual void setModelPosition(const glm::vec3& position) = 0;
        virtual glm::vec3& getPosition() = 0;
        virtual bool isVisible(Volume& volume) = 0;
};

class PerspectiveCamera: public Camera{
    private:
        Uniform<glm::mat4> projectionMatrix = Uniform<glm::mat4>("projectionMatrix");
        Uniform<glm::mat4> viewMatrix = Uniform<glm::mat4>("viewMatrix");;
        Uniform<glm::mat4> modelMatrix = Uniform<glm::mat4>("modelMatrix");;

        glm::vec3 position = glm::vec3(0,0,0);
        glm::vec3 direction = glm::vec3(1,0,0);
        glm::vec3 up = glm::vec3(0,1,0);

        int screenWidth = 1920;
        int screenHeight = 1080;

        float zNear = 0.01f;
        float zFar = 1000.0f;

        float pitch = 0;
        float yaw = 0;

        float FOV = 90.0f;
        float aspect;

        Frustum frustum;

        void calculateFrustum();

    public:
        PerspectiveCamera();
        void resizeScreen(int width, int height, float FOV);
        void adjustFOV(float FOV);
        void initialize(std::vector<std::reference_wrapper<ShaderProgram>> programs);

        void setModelPosition(const glm::vec3& position);
        void setPosition(float x, float y, float z) {setPosition(glm::vec3(x,y,z));};
        void setPosition(glm::vec3 pos);
        void setRotation(float pitch, float yaw);

        bool isVisible(Volume& volume){
            return volume.isOnFrustum(*this);
        }

        glm::vec3& getPosition() {return position;}
        glm::vec3& getDirection() {return direction;}
        glm::vec3& getUp() {return up;}
        Frustum& getFrustum() {return frustum;}
        
        int getScreenWidth(){return screenWidth;}
        int getScreenHeight(){return screenHeight;}

        float getPitch(){return pitch;};
        float getYaw(){return yaw;};

        Uniform<glm::mat4>& getProjectionUniform() {return projectionMatrix;}
        Uniform<glm::mat4>& getViewUniform() {return viewMatrix;}
};

class DepthCamera: public Camera{
    private:
        ShaderProgram program;
        std::unique_ptr<GLDepthTexture> texture;

        Uniform<glm::mat4> projectionMatrix = Uniform<glm::mat4>("projectionMatrix");
        Uniform<glm::mat4> viewMatrix = Uniform<glm::mat4>("viewMatrix");
        Uniform<glm::mat4> lightSpaceMatrix = Uniform<glm::mat4>("lightSpaceMatrix");
        Uniform<glm::mat4> modelMatrix = Uniform<glm::mat4>("modelMatrix");

        glm::vec3 position = glm::vec3(0,0,0);
        glm::vec3 target = glm::vec3(0,0,0);
        
        float zNear = -500.0f;
        float zFar = 500.0f;

        uint32_t depthMapFBO;
        const uint32_t SHADOW_WIDTH = 1024 * 6, SHADOW_HEIGHT = 1024 * 6;
        uint32_t depthMap;

    public:
        void updateProjection();
        void initialize();
        void prepareForRender();

        void setModelPosition(const glm::vec3& position);
        void setPosition(float x, float y, float z);
        void setPosition(glm::vec3& pos) {this->position = pos;};
        void setTarget(float x, float y, float z);
        void setTarget(glm::vec3& t) {this->target = t;}
        bool isVisible(Volume&){return true;}
        
        glm::vec3& getPosition() {return position;}
        GLDepthTexture* getTexture() const {return texture.get();}
        glm::mat4& getLightSpaceMatrix() {return lightSpaceMatrix.getValue();}
        ShaderProgram& getProgram() {return program;}

        Uniform<glm::mat4>& getProjectionUniform() {return projectionMatrix;}
        Uniform<glm::mat4>& getViewUniform() {return viewMatrix;}
};
#include <chunk.hpp>

#endif