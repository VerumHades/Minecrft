#include <test_scene.hpp>

//void TestScene::unlockedKeyEvent(GLFWwindow* /*window*/, int key, int /*scancode*/, int action, int /*mods*/){
//    if(key == GLFW_KEY_W && action == GLFW_PRESS){
//        std::cout << "Down" << std::endl;
//
//        if(stage) setUILayer("second");
//        else setUILayer("first");
//
//        stage = !stage;
//    }
//}

void TestScene::initialize(){
    auto form = std::make_shared<UIForm>(std::vector<UIForm::Field>{
        UIForm::Field{"Text: ", UIForm::TEXT}
    }, *uiManager);
    
    form->setPosition(10,10);
    form->setAttribute(&UIFrame::Style::backgroundColor, {20,20,20});
    form->setSize(200,200);

    setUILayer("first");
    addElement(form);
    setUILayer("first");
}

void TestScene::open(GLFWwindow* window){
    setUILayer("first");
}

void TestWidget::getRenderingInformation(UIRenderBatch& batch){
    batch.Rectangle(transform.x,transform.y,100,100,{255,0,0});
    batch.Text("aoiwndakijbndwkja,cfklmasdnfviqauejhfnbakjfbn,zxmcplnkcqWA I EDquf", transform.x, transform.y, 20,  {255,255,255});
}