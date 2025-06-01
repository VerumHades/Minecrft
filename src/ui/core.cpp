#include <ui/core.hpp>

UICore::UICore(){
    loader().registerElement("frame", XML_ELEMENT_LAMBDA_LOAD(UIFrame));
    loader().registerElement("label", [](auto* source) {
        const char* content = source->GetText();
        std::string text = "";
        if(content) text = std::string(content);
                
        auto label = std::make_shared<UILabel>();
        label->setText(text);
        return label; 
    });
    loader().registerElement("image", [](auto* source) -> std::shared_ptr<UIFrame> {
        const char* content = source->Attribute("src");
        if(!content){
            LogError("Image element requires a 'src'.");
            return std::make_shared<UIFrame>();
        }
        return std::make_shared<UIImage>(content); 
    });
    loader().registerElement("input", XML_ELEMENT_LAMBDA_LOAD(UIInput));
    loader().registerElement("scrollable", XML_ELEMENT_LAMBDA_LOAD(UIScrollableFrame));
}



UICore& UICore::get(){
    static std::mutex singleton_mutex;
    std::lock_guard lock(singleton_mutex);
    static UICore core;
    return core;
}

void UICore::cleanup(){
    underHover = nullptr;
    inFocus = nullptr;
    underScrollHover = nullptr;

    windows.clear();
    loaded_images.clear();
    loader_ = {};
}

void UICore::resize(int width, int height){
    screenWidth = width;
    screenHeight = height;

    backend->resizeVieport(width,height);

    if(!getCurrentWindow()) return;

    updateAll();
}

void UICore::updateAll(){
    auto* window = getCurrentWindow();
    if(!window) return;

    for(auto& element: window->getCurrentLayer().getElements()){
        element->calculateTransforms();
        element->update();
        element->updateChildren();
    }
}
void UICore::stopDrawingAll(){
    auto* window = getCurrentWindow();
    if(!window) return;

    for(auto& element: window->getCurrentLayer().getElements()){
        element->stopDrawing();
        element->stopDrawingChildren();
    }
}

void UICore::setBackend(UIBackend* backend){
    if(this->backend) stopDrawingAll();
    this->backend = backend;
    updateAll();
}

void UICore::renderElementAndChildren(std::shared_ptr<UIFrame>& element){
    if(element->has_draw_batch){
        backend->renderBatch(element->draw_batch_iterator);
    }
    
    for(auto& child: element->children){
        renderElementAndChildren(child);
    }
}

void UICore::render(){
    auto* window = getCurrentWindow();
    if(!window) return;

    backend->setupRender();

    for(auto& element: window->getCurrentLayer().getElements()){
        renderElementAndChildren(element);
    }

    backend->cleanupRender();
}

void UICore::mouseMove(int x, int y){
    mousePosition.x = x;
    mousePosition.y = y;

    auto element = getElementUnder(x,y);
    underScrollHover = getElementUnder(x,y,true);
    
    if(element != underHover){
        if(underHover){
            underHover->setHover(false);
            underHover->onMouseLeave.trigger();
            underHover->update(); 
        }
        if(element){
            element->setHover(true);
            element->onMouseEnter.trigger();
            element->update();
        }

        underHover = element;
    }

    if(underHover) underHover->onMouseMove.trigger(x,y);
    if(inFocus && inFocus != underHover) inFocus->onMouseMove.trigger(x,y);
}

void UICore::mouseEvent(GLFWwindow* window, int button, int action, int mods){
    if(button == GLFW_MOUSE_BUTTON_1 && action == GLFW_PRESS){
        if(inFocus != underHover){
            if(inFocus) inFocus->setFocus(false);
            inFocus = underHover;
            if(inFocus && inFocus->focusable) inFocus->setFocus(true);
        }
        if(underHover) underHover->onClicked.trigger();
    }

    if(underHover) underHover->onMouseEvent.trigger(button,action,mods);
    if(inFocus && inFocus != underHover) inFocus->onMouseEvent.trigger(button,action,mods);

    if(underHover) underHover->update();
    if(inFocus) inFocus->update();
}

void UICore::scrollEvent(GLFWwindow* window, double xoffset, double yoffset){
    if(!underScrollHover) return;
    underScrollHover->onScroll.trigger(xoffset,yoffset);
    
    if(underScrollHover){
        underScrollHover->update();
        underScrollHover->updateChildren();
    }
}

void UICore::keyTypedEvent(GLFWwindow* window, unsigned int codepoint){
    if(!inFocus) return;
    inFocus->onKeyTyped.trigger(codepoint);
    
    if(inFocus) inFocus->update();
}
void UICore::keyEvent(GLFWwindow* window, int key, int scancode, int action, int mods){
    if(!inFocus) return;
    inFocus->onKeyEvent.trigger(key,scancode,action,mods);

    if(inFocus) inFocus->update();
}


void UICore::resetStates(){
    if(underHover){
        underHover->setHover(false);
    }
    if(inFocus){
        inFocus->setFocus(false);
    }
    inFocus = nullptr;
    underHover = nullptr;
    underScrollHover = nullptr;
}

void UICore::setCurrentWindow(UIWindowIdentifier id){
    resetStates();
    stopDrawingAll();
    currentWindow = id;
    
    if(!getCurrentWindow()) return;

    updateAll();
}
UIWindow* UICore::getCurrentWindow(){
    if(currentWindow < 0 || currentWindow >= static_cast<int>(windows.size())) return nullptr;
    return &windows[currentWindow];
}
UIWindow* UICore::getWindow(UIWindowIdentifier id){
    if(id < 0 || id >=  static_cast<int>(windows.size())) return nullptr;
    return &windows[id];
}
UIWindowIdentifier UICore::createWindow(){
    int id = windows.size();
    windows.push_back({});
    return id;
}

UIBackend& UICore::getBackend(){
    if(!backend) throw std::logic_error("Using ui core without setting a backend, use 'setBackend' to set a valid backend.");
    return *backend;
}

void UICore::loadWindowFromXML(UIWindow& window, std::string load_path){
    loader().loadWindowFromXML(window, load_path);
}

std::shared_ptr<UIFrame> UICore::getElementUnder(int x, int y, bool onlyScrollable){
    auto* current_window = getCurrentWindow();
    if(!current_window) return nullptr;

    std::queue<std::tuple<int, std::shared_ptr<UIFrame>, std::shared_ptr<UIFrame>>> elements;

    for(auto& window: current_window->getCurrentLayer().getElements()) elements.push({0,window, nullptr});
    
    std::shared_ptr<UIFrame> current = nullptr;
    int cdepth = -1;

    while(!elements.empty()){
        auto [depth,element,parent] = elements.front();
        elements.pop();

        if(!element->pointWithin({x,y})) continue;

        for(auto& child: element->children) elements.push({depth + 1, child, element});

        if(onlyScrollable && !element->isScrollable()) continue;
        if(!element->isHoverable()) continue;
        if(cdepth >= depth) continue;
        
        current = element;
    }

    return current;
}

std::shared_ptr<GLTextureArray> UICore::LoadImage(const std::string& path){
    if(UICore::get().loaded_images.contains(path))
        return UICore::get().loaded_images[path];

    Image image{path};
    if(!image.isLoaded()) return nullptr;
    
    auto texture_array = std::make_shared<GLTextureArray>();

    texture_array->setup(image.getWidth(), image.getHeight(),1);
    texture_array->putImage(0,0,0,image);

    UICore::get().loaded_images[path] = texture_array;

    return texture_array;
}

std::shared_ptr<GLTextureArray> UICore::LoadImage(const Image& image){
    if(!image.isLoaded()) return nullptr;
    
    auto texture_array = std::make_shared<GLTextureArray>();

    texture_array->setup(image.getWidth(), image.getHeight(),1);
    texture_array->putImage(0,0,0,image);
    
    return texture_array;
}