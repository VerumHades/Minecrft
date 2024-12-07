#pragma once

#include <ui/backend.hpp>
#include <rendering/shaders.hpp>
#include <general.hpp>
#include <coherency.hpp>
#include <list>
#include <ui/font.hpp>


class UIOpenglBackend: public UIBackend{
    private:
        const int vertex_size = 9;

        FontManager fontManager;
        Font mainFont = Font("fonts/JetBrainsMono/fonts/variable/JetBrainsMono[wght].ttf", 128);

        GLVertexArray vao;

        Uniform<glm::mat4> projection_matrix = Uniform<glm::mat4>("gl_ui_projection_matrix");
        ShaderProgram shader_program = ShaderProgram("shaders/graphical/ui/opengl_backend.vs","shaders/graphical/ui/opengl_backend.fs");

        AllocatedList<float> vertices = AllocatedList<float>(1000 * vertex_size);
        AllocatedList<uint> indices = AllocatedList<uint>(1000 * 6);

        GLBuffer<uint,  GL_ELEMENT_ARRAY_BUFFER> index_buffer;
        GLBuffer<float, GL_ARRAY_BUFFER> vertex_buffer;

        bool needs_update = false;

        void proccessRenderCommand(UIRenderCommand& command, float*& vertices, uint*& indices, int& index_offset);
        void processTextCommand(UIRenderCommand& command, float*& vertices, uint*& indices, int& index_offset);

    public:
        UIOpenglBackend();
        std::list<Batch>::iterator addRenderBatch(UIRenderBatch& batch) override;
        void setupRender() override;
        void cleanupRender() override;
        void renderBatch(std::list<Batch>::iterator batch_iter) override;
        void removeBatch(std::list<Batch>::iterator batch_iter) override;
        void resizeVieport(int width, int height) override;
};