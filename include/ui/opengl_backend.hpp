#pragma once

#include <ui/backend.hpp>
#include <rendering/opengl/shaders.hpp>
#include <general.hpp>
#include <coherency.hpp>
#include <list>
#include <ui/font.hpp>


class UIOpenglBackend: public UIBackend{
    private:
        const int vertex_size = 9;

        FontManager fontManager;
        Font mainFont = Font("resources/fonts/JetBrainsMono[wght].ttf", 32);

        GLVertexArray vao;

        Uniform<glm::mat4> projection_matrix = Uniform<glm::mat4>("ui_projection_matrix");
        ShaderProgram shader_program = ShaderProgram("resources/shaders/graphical/ui/opengl_backend.vs","resources/shaders/graphical/ui/opengl_backend.fs");

        AllocatedList<float> vertices = AllocatedList<float>(100 * vertex_size);
        AllocatedList<uint> indices = AllocatedList<uint>(100 * 6);

        GLBuffer<uint,  GL_ELEMENT_ARRAY_BUFFER> index_buffer;
        GLBuffer<float, GL_ARRAY_BUFFER> vertex_buffer;

        bool needs_update = false;

        void proccessRenderCommand(UIRenderCommand& command, float*& vertices, uint*& indices, int& index_offset);
        void processTextCommand(UIRenderCommand& command, float*& vertices, uint*& indices, int& index_offset);

        std::tuple<size_t, size_t> calculateBatchSizes(UIRenderBatch& batch);

    public:
        UIOpenglBackend();
        std::list<Batch>::iterator addRenderBatch(UIRenderBatch& batch) override;
        void setupRender() override;
        void cleanupRender() override;
        void renderBatch(std::list<Batch>::iterator batch_iter) override;
        void removeBatch(std::list<Batch>::iterator batch_iter) override;
        void resizeVieport(int width, int height) override;
        UITextDimensions getTextDimensions(std::string text, int size) override;
};