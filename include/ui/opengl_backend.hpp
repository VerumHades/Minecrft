#pragma once

#include <ui/backend.hpp>
#include <general.hpp>
#include <coherency.hpp>
#include <list>

class UIOpenglBackend: public UIBackend{
    private:
        struct Batch{
            glm::vec2 clip_min;
            glm::vec2 clip_max;

            BindableTexture* texture = nullptr;
            
            size_t vertex_start;
            size_t index_start;
        };

        std::list<Batch> batches;

        AllocatedList<float> vertices;
        AllocatedList<uint> indices;

        

    public:
        void addRenderBatch(UIRenderBatch& batch) override;
        void render() override;
};