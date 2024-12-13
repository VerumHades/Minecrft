#include <rendering/ssao.hpp>

float GLSSAO::lerp(float a, float b, float f)
{
    return a + f * (b - a);
}  

GLSSAO::GLSSAO(){
    std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f); // random floats between [0.0, 1.0]
    std::default_random_engine generator;

    for (unsigned int i = 0; i < kernel_count; ++i)
    {
        glm::vec3 sample(
            randomFloats(generator) * 2.0f - 1.0f, 
            -randomFloats(generator) , 
            randomFloats(generator) * 2.0f - 1.0f
        );
        sample  = glm::normalize(sample);
        float scale = (float)i / 64.0; 
        scale   = lerp(0.1f, 1.0f, scale * scale);
        sample *= scale;
        kernel_uniform.getValue().push_back(sample);  
    }

    std::vector<glm::vec3> noise;
    for (unsigned int i = 0; i < 16; i++)
    {
        glm::vec3 noise_vector(
            randomFloats(generator) * 2.0 - 1.0, 
            0.0f,
            randomFloats(generator) * 2.0 - 1.0
        ); 
        noise.push_back(noise_vector);
    }  

    noiseTexture.configure(GL_RGBA16F, GL_RGB, GL_FLOAT, 4, 4, noise.data());
    noiseTexture.parameter(GL_TEXTURE_WRAP_S, GL_REPEAT);
    noiseTexture.parameter(GL_TEXTURE_WRAP_T, GL_REPEAT);  

    shader_program.setSamplerSlot("gPosition", 0);
    shader_program.setSamplerSlot("gNormal" , 1);
    shader_program.setSamplerSlot("texNoise", 2);
}

void GLSSAO::render(GLTexture2D& gPositionTexture, GLTexture2D& gNormalTexture, GLFullscreenQuad& quad){
    framebuffer.bind();
    glDisable(GL_DEPTH_TEST);
    glViewport(0,0,framebuffer.getWidth(), framebuffer.getHeight());
    
    shader_program.updateUniforms();
    gPositionTexture.bind(0);
    gNormalTexture.bind(1);
    noiseTexture.bind(2);

    quad.render();

    framebuffer.unbind();
}