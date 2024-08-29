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
        //virtual ~Camera() {}  // Add a virtual destructor
        virtual void setModelPosition(float x, float y, float z) = 0;
        virtual glm::vec3& getPosition() = 0;
        virtual bool isVisible(Volume& volume) = 0;
        virtual void updateUniforms() = 0;
};

class PerspectiveCamera: public Camera{
    private:
        std::unique_ptr<GLSkybox> skybox;
        std::string currentProgram;
        std::unordered_map<std::string,ShaderProgram> programs;

        glm::mat4 projectionMatrix;
        glm::mat4 viewMatrix = glm::mat4(1.0f);
        glm::mat4 modelMatrix = glm::mat4(1.0f);

        glm::vec3 position = glm::vec3(0,0,0);
        glm::vec3 direction = glm::vec3(1,0,0);
        glm::vec3 up = glm::vec3(0,1,0);

        float screenWidth = 1920;
        float screenHeight = 1080;

        float zNear = 0.1f;
        float zFar = 1000.0f;
        float FOV = 90.0f;
        float aspect;

        Frustum frustum;

        void calculateFrustum();

    public:
        PerspectiveCamera();
        void resizeScreen(int width, int height, float FOV);
        void adjustFOV(float FOV);
        void updateUniforms();

        void setModelPosition(float x, float y, float z);
        void setPosition(float x, float y, float z);
        void setRotation(float pitch, float yaw);

        void addShader(std::string name, std::string vertex, std::string fragment);
        void addSkybox(std::string vertex, std::string fragment, std::array<std::string,6> paths);

        bool isVisible(Volume& volume){
            return volume.isOnFrustum(*this);
        }

        ShaderProgram& getProgram(std::string name);
        void useProgram(std::string name);

        glm::vec3& getDirection() {return direction;}
        Frustum& getFrustum() {return frustum;}
        glm::vec3& getPosition() {return position;}
        void drawSkybox();
        int getScreenWidth(){return screenWidth;}
        int getScreenHeight(){return screenHeight;}
};

class DepthCamera: public Camera{
    private:
        ShaderProgram program;
        std::unique_ptr<GLDepthTexture> texture;

        glm::mat4 projectionMatrix;
        glm::mat4 viewMatrix = glm::mat4(1.0f);
        glm::mat4 lightSpaceMatrix;
        glm::mat4 modelMatrix = glm::mat4(1.0f);

        glm::vec3 position = glm::vec3(0,0,0);
        glm::vec3 target = glm::vec3(0,0,0);
        
        float zNear = -500.0f;
        float zFar = 500.0f;

        unsigned int depthMapFBO;
        const unsigned int SHADOW_WIDTH = 1024 * 3, SHADOW_HEIGHT = 1024 * 3;
        unsigned int depthMap;


    public:
        void updateUniforms();
        void updateProjection();
        void initialize();
        void prepareForRender();

        void setModelPosition(float x, float y, float z);
        void setPosition(float x, float y, float z);
        void setPosition(glm::vec3& position) {this->position = position;};
        void setTarget(float x, float y, float z);
        void setTarget(glm::vec3& target) {this->target = target;}
        bool isVisible(Volume& volume){return true;}
        
        glm::vec3& getPosition() {return position;}
        GLDepthTexture* getTexture() const {return texture.get();}
        glm::mat4& getLightSpaceMatrix() {return lightSpaceMatrix;}
};
#include <chunk.hpp>

#endif