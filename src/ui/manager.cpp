#include <ui/manager.hpp>

void UIManager::initialize(){
    uiProgram = std::make_unique<ShaderProgram>();
    uiProgram->initialize();
    uiProgram->addShader("shaders/ui.vs", GL_VERTEX_SHADER);
    uiProgram->addShader("shaders/ui.fs", GL_FRAGMENT_SHADER);
    uiProgram->compile();
    uiProgram->use();

    drawBuffer = std::make_unique<GLBuffer>();

    projectionMatrix.attach(*uiProgram);

    resize(1920,1080);

    windows.push_back({20,20,200,200});
}

void UIManager::resize(int width, int height){
    screenWidth = width;
    screenHeight = height;

    std::cout << width << " " << height << std::endl;

    projectionMatrix = glm::ortho(
        0.0f,   // Left
        (float)width, // Right
        (float)height, // Top
        0.0f,   // Bottom
        -1.0f,  // Near plane
        1.0f    // Far plane
    );
}

void UIManager::processRenderingInformation(UIRenderInfo& info, Mesh& output){
    output.addFlatFace(
        info.x,
        info.y,
        info.width,
        info.height
    );

    //std::cout << info.x << " " << info.y << " " << info.width << " " << info.height << std::endl;
}

void UIManager::update(){
    uiProgram->use();

    Mesh temp = Mesh();
    temp.setVertexFormat({2,2});

    for(auto& window: windows){
        UIRenderInfo winfo = window.getRenderingInformation();
        processRenderingInformation(winfo, temp);
    }

    //for(auto& v: temp.getVertices()){
    //    std::cout << v << " ";
    //}
    //std::cout << std::endl;

    drawBuffer->loadMesh(temp);
}

void UIManager::draw(){
    uiProgram->updateUniforms();
    drawBuffer->draw();
}