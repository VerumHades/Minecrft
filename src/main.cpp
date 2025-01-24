#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define _GLIBCXX_DEBUG 1
#include <iostream>
#include <cmath>

#include <rendering/instanced_mesh.hpp>


/*void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS){
        if(menuOpen){
            UICore.setScene("main");
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
        else{
            UICore.setScene("internal_default");
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
        menuOpen = !menuOpen;
    }
    UICore.keyEvent(key,action);
}*/

void APIENTRY GLDebugMessageCallback(GLenum source, GLenum type, GLuint id,
                            GLenum severity, GLsizei length,
                            const GLchar *message, const void *param)
{
	const char *source_, *type_, *severity_;

	switch (source)
	{
	case GL_DEBUG_SOURCE_API:             source_ = "API";             break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   source_ = "WINDOW_SYSTEM";   break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER: source_ = "SHADER_COMPILER"; break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:     source_ = "THIRD_PARTY";     break;
	case GL_DEBUG_SOURCE_APPLICATION:     source_ = "APPLICATION";     break;
	case GL_DEBUG_SOURCE_OTHER:           source_ = "OTHER";           break;
	default:                              source_ = "<SOURCE>";        break;
	}

	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR:               type_ = "ERROR";               break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: type_ = "DEPRECATED_BEHAVIOR"; break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  type_ = "UDEFINED_BEHAVIOR";   break;
	case GL_DEBUG_TYPE_PORTABILITY:         type_ = "PORTABILITY";         break;
	case GL_DEBUG_TYPE_PERFORMANCE:         type_ = "PERFORMANCE";         break;
	case GL_DEBUG_TYPE_OTHER:               type_ = "OTHER";               break;
	case GL_DEBUG_TYPE_MARKER:              type_ = "MARKER";              break;
	default:                                type_ = "<TYPE>";              break;
	}

	switch (severity)
	{
	case GL_DEBUG_SEVERITY_HIGH:         severity_ = "HIGH";         break;
	case GL_DEBUG_SEVERITY_MEDIUM:       severity_ = "MEDIUM";       break;
	case GL_DEBUG_SEVERITY_LOW:          severity_ = "LOW";          break;
	case GL_DEBUG_SEVERITY_NOTIFICATION: severity_ = "NOTIFICATION"; break;
	default:                             severity_ = "<SEVERITY>";   break;
	}

	printf("%d: GL %s %s (%s): %s\n",
		   id, severity_, type_, source_, message);
}

void printOpenGLLimits() {
    GLint maxWorkGroupCount[3]; // [X, Y, Z]
    GLint maxWorkGroupSize[3];  // [X, Y, Z]
    GLint maxInvocations;       // Total invocations per work group
    GLint maxSharedMemorySize;  // Total shared memory size for compute shaders
    
    // Query GL_MAX_COMPUTE_WORK_GROUP_COUNT
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &maxWorkGroupCount[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &maxWorkGroupCount[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &maxWorkGroupCount[2]);
    
    // Query GL_MAX_COMPUTE_WORK_GROUP_SIZE
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &maxWorkGroupSize[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &maxWorkGroupSize[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &maxWorkGroupSize[2]);
    
    // Query GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &maxInvocations);
    
    // Query GL_MAX_COMPUTE_SHARED_MEMORY_SIZE
    glGetIntegerv(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE, &maxSharedMemorySize);
    
    // Output the queried values
    std::cout << "GL_MAX_COMPUTE_WORK_GROUP_COUNT:" << std::endl;
    std::cout << "  X: " << maxWorkGroupCount[0] << ", Y: " << maxWorkGroupCount[1] << ", Z: " << maxWorkGroupCount[2] << std::endl;
    
    std::cout << "GL_MAX_COMPUTE_WORK_GROUP_SIZE:" << std::endl;
    std::cout << "  X: " << maxWorkGroupSize[0] << ", Y: " << maxWorkGroupSize[1] << ", Z: " << maxWorkGroupSize[2] << std::endl;
    
    std::cout << "GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS: " << maxInvocations << std::endl;
    
    std::cout << "GL_MAX_COMPUTE_SHARED_MEMORY_SIZE: " << maxSharedMemorySize << " bytes" << std::endl;
}

int main() {
    GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit()) {
        std::cout << "Failed to initialize glfw!" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwWindowHint(GLFW_SAMPLES, 4); // anti-alliasing
    
    ///glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_FALSE);
    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(1920, 1080, "Hello Terrain", NULL, NULL);

    if (!window) {
        std::cout << "Failed to initialize glfw window!" << std::endl;
        glfwTerminate();
        return -1;
    }
    /* Make the window's context current */
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);
    
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize glad!" << std::endl;
        return -1;
    }
    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
    
    //printOpenGLLimits();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    
    glEnable(GL_CULL_FACE);  // Enable backface culling
    glCullFace(GL_BACK);     // Cull back faces
    glFrontFace(GL_CCW);     // Set counterclockwise winding order as front*/

    //glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_MULTISAMPLE);  // Redundant perhaps
    //glDepthMask(GL_FALSE);
    /*
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(GLDebugMessageCallback, NULL);
    */

    {
        //InstancedMesh mesh{};
        //mesh.addQuadFace({0,0,0}, 10,10, 0, InstancedMesh::BILLBOARD,InstancedMesh::Forward,{0.0f,0.0f,0.0f,0.0f});
        
        //GLDrawCallBuffer d_buffer{};

        
        InstancedMeshBuffer buffer{};
        //std::array<GLVertexArray,4> vao{};

        //buffer.loadMesh(mesh);
        //Terrain terrain{};
        //WorldGenerator generator{};
        //ChunkMeshGenerator mesh_generator{};
        //ChunkMeshRegistry registry{5};

        //mesh_generator.setWorld(&terrain);
        
        //generator.generateTerrainChunkAccelerated(terrain.createEmptyChunk({0,0,0}),{0,0,0});
        //mesh_generator.syncGenerateSyncUploadMesh(terrain.getChunk({0,0,0}), registry);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}