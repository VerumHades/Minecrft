#include <ui/core.hpp>

UICore ui_core{};

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
    loader().registerElement("input", XML_ELEMENT_LAMBDA_LOAD(UIInput));
    loader().registerElement("scrollable", XML_ELEMENT_LAMBDA_LOAD(UIScrollableFrame));
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

    if(element == underHover) return;

    if(element != underHover && underHover != nullptr){
        underHover->setHover(false);
        underHover->update(); 
        if(underHover->onMouseLeave) underHover->onMouseLeave();
    }

    if(element != nullptr){
        element->setHover(true);
        element->update();

        //std::cout << "Newly under hover: " << element->identifiers.tag << " #" << element->identifiers.id << " ";
        //for(auto& classname: element->identifiers.classes) std::cout << "." << classname;
        //std::cout << " Element has state: " << element->state << std::endl;

        if(element->onMouseEnter) element->onMouseEnter();
    }

    underHover = element;

    if(underHover && underHover->onMouseMove) underHover->onMouseMove(x,y);
    if(inFocus && inFocus != underHover && inFocus->onMouseMove) inFocus->onMouseMove(x,y);
}

void UICore::mouseEvent(GLFWwindow* window, int button, int action, int mods){
    if(button == GLFW_MOUSE_BUTTON_1 && action == GLFW_PRESS){
        if(inFocus != underHover){
            if(inFocus) inFocus->setFocus(false);
            inFocus = underHover;
            if(inFocus) inFocus->setFocus(true);
        }
        if(underHover && underHover->onClicked) underHover->onClicked();
    }

    if(underHover && underHover->onMouseEvent) underHover->onMouseEvent(button,action,mods);
    if(inFocus && inFocus != underHover && inFocus->onMouseEvent) inFocus->onMouseEvent(button,action,mods);

    if(underHover) underHover->update();
    if(inFocus) inFocus->update();
}

void UICore::scrollEvent(GLFWwindow* window, double xoffset, double yoffset){
    if(!underScrollHover) return;
    if(underScrollHover->onScroll) underScrollHover->onScroll(xoffset,yoffset);
    
    if(underScrollHover){
        underScrollHover->update();
        underScrollHover->updateChildren();
    }
}

void UICore::keyTypedEvent(GLFWwindow* window, unsigned int codepoint){
    if(!inFocus) return;
    if(inFocus->onKeyTyped) inFocus->onKeyTyped(codepoint);
    
    if(inFocus) inFocus->update();
}
void UICore::keyEvent(GLFWwindow* window, int key, int scancode, int action, int mods){
    if(!inFocus) return;
    if(inFocus->onKeyEvent) inFocus->onKeyEvent(key,scancode,action,mods);

    if(inFocus) inFocus->update();
}


void UICore::resetStates(){
    if(underHover){
        underHover->setHover(false);
        //if(underHover->onMouseLeave) underHover->onMouseLeave();
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
    if(currentWindow < 0 || currentWindow >= windows.size()) return nullptr;
    return &windows[currentWindow];
}
UIWindow* UICore::getWindow(UIWindowIdentifier id){
    if(id < 0 || id >= windows.size()) return nullptr;
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