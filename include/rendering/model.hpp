#pragma once

#include <unordered_map>
#include <memory>
#include <mutex>
#include <atomic>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <array>
#include <rendering/opengl/buffer.hpp>
#include <rendering/opengl/texture.hpp>
#include <rendering/opengl/shaders.hpp>
#include <rendering/mesh.hpp>

#include <structure/pool.hpp>

#include <synchronization.hpp>
#include <coherency.hpp>
#include <vec_hash.hpp>

#include <unordered_set>

class Model;

/**
 * @brief An existing instance of a model
 * 
 */
class ModelInstance {
    public:
        virtual ~ModelInstance() = default;

        virtual void MoveTo(const glm::vec3& position) = 0;
        virtual void Scale(const glm::vec3& scale) = 0;
        virtual void Rotate(const glm::quat& rotation) = 0;
        virtual void MoveRotationOffset(const glm::vec3& rotation_center) = 0;
        virtual bool IsOfModel(Model& model) = 0;
};

/**
 * @brief The definition of a model that holds all the actual data
 * 
 */
class Model{
    private:
        class Instance: public ModelInstance  {
            private:
                Model& model;
                size_t index;

            public:
                Instance(Model& model, size_t index): model(model), index(index) {}

                void MoveTo(const glm::vec3& position) override;
                void Scale(const glm::vec3& scale) override;
                void Rotate(const glm::quat& rotation) override;
                void MoveRotationOffset(const glm::vec3& rotation_center) override;
                bool IsOfModel(Model& model) override;
        };

        struct Request {
            glm::vec3 position;
            glm::vec3 scale;
            glm::vec4 rotation;
            glm::vec3 rotation_offset;
        };
        Pool<Request> request_pool = {};

        std::mutex swap_mutex;

        std::atomic<bool> upload_data = false;
        std::atomic<int> selected = 0;

        std::array<GLBuffer<float, GL_ARRAY_BUFFER>,3> instance_buffers = {};

        int backIndex() {return (selected + 1) % 3; }
        int lastIndex() {return (selected + 2) % 3; }

        GLBuffer<float, GL_ARRAY_BUFFER>& getBackInstanceBuffer()  { return instance_buffers[backIndex()]; }
        GLBuffer<float, GL_ARRAY_BUFFER>& getFrontInstanceBuffer() { return instance_buffers[selected]; }

        std::vector<std::unique_ptr<LoadedMesh>> loaded_meshes;

        static std::unordered_set<Model*>& getModelSet(){
            static std::unordered_set<Model*> models{};
            return models;
        };

        Request& GetRequest(size_t index);

    protected:
        glm::vec3 rotation_center_offset = {0,0,0};

        void addMesh(Mesh& mesh);
        Mesh createMesh();

        //Model<float, GL_ARRAY_BUFFER> vertex_buffer;
        //Model<uint , GL_ELEMENT_ARRAY_BUFFER> index_buffer;

    public:
        Model();
        ~Model();

        // Delete the copy constructor to make it non-copyable
        Model(const Model& other) = delete;
        Model& operator=(const Model& other) = delete;
        Model(Model&& other) = delete;
        Model& operator=(Model&& other) = delete;

        /**
         * @brief Create an instance of the model
         * 
         * @return std::shared_ptr<ModelInstance> 
         */
        std::shared_ptr<ModelInstance> NewInstance();

        /**
         * @brief Draw all instances of this model
         * 
         */
        void drawAllRequests();

        const glm::vec3& getRotationCenterOffset(){return rotation_center_offset;}

        /**
         * @brief Draw all instances of all models
         * 
         */
        static void DrawAll();

        /**
         * @brief Cleanup all models
         * 
         */
        static void CleanupAll();
};
