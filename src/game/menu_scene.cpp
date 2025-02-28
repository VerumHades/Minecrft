#include <game/menu_scene.hpp>

void MenuScene::initialize(){
    UICore::get().loadWindowFromXML(*this->getWindow(), "resources/templates/menu.xml");
    
    auto scrollable = getElementById<UIScrollableFrame>("world_selection");
    
    getUILayer("world_menu").onEntered = [this, scrollable] {
        scrollable->clearChildren();

        auto* mainScene = static_cast<MainScene*>(sceneManager->getScene("game"));

        auto path = Paths::Get(Paths::GAME_SAVES);
        if(!path) return;

        for (const auto& dirEntry: fs::directory_iterator(path.value())){
            if(!dirEntry.is_directory()) continue;

            std::string path = dirEntry.path().string();

            GameState state{path};

            auto frame = std::make_shared<UIFrame>();
            frame->setIdentifiers({"world_option_frame"});
            
            auto temp = std::make_shared<UILabel>();
            if(state.getWorldStream()) temp->setText(state.getWorldStream()->getName());
            temp->setSize({PERCENT,100}, 40_px);
            temp->setHoverable(false);
            temp->setIdentifiers({"world_option_label"});

            auto chunkCountLabel = std::make_shared<UILabel>();
            chunkCountLabel->setText("Number of saved chunks: ");
            chunkCountLabel->setSize({PERCENT,100},40_px);
            chunkCountLabel->setPosition(0_px,45_px);
            chunkCountLabel->setHoverable(false);
            chunkCountLabel->setIdentifiers({"world_option_chunk_count_label"});

            frame->onClicked = [this, mainScene, path] {
                mainScene->setWorldPath(path);
                sceneManager->setScene("game");
            };

            frame->appendChild(temp);
            frame->appendChild(chunkCountLabel);
            scrollable->appendChild(frame);
        }
        
        scrollable->calculateTransforms();
        scrollable->update();
        scrollable->updateChildren();
    };

    auto seedInput = getElementById<UIInput>("new_world_seed");
    seedInput->inputValidation = [](char character){
        return std::isdigit(character);
    };
    seedInput->max_length = 9;

    auto previewContainer = getElementById<UIFrame>("world_preview_container");

    auto resetPreview = [this, seedInput, previewContainer](){
        static WorldGenerator generator{};
        int seed = std::stoi(seedInput->getText());
        generator.setSeed(seed);
        
        previewContainer->clearChildren();
    
        int max_size = std::min(previewContainer->getContentTransform().width, previewContainer->getContentTransform().height) - 100;

        auto preview = std::make_shared<UIImage>(
            generator.createPreview(max_size,max_size)
        );
        
        preview->setPosition(TValue::Center(), TValue::Center());
        preview->setSize(TValue::Pixels(max_size), TValue::Pixels(max_size));

        previewContainer->appendChild(preview);
        previewContainer->calculateTransforms();
        previewContainer->update();
        previewContainer->updateChildren();
    };

    auto resetPreviewButton = getElementById<UILabel>("new_world_seed_preview_reset");
    resetPreviewButton->onClicked = resetPreview;

    auto seedRegenerate = getElementById<UILabel>("new_world_seed_regenerate");
    seedRegenerate->onClicked = [this,seedInput,resetPreview](){
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dist(100000000,999999999); 

        seedInput->setText(std::to_string(dist(gen)));
        seedInput->update();

        resetPreview();
    };
    seedRegenerate->onClicked.trigger();

    auto newWorldNameInput = getElementById<UIInput>("new_world_name");
    
    auto newWorldFunc = [newWorldNameInput, seedInput, this]{
        auto name = newWorldNameInput->getText();
        newWorldNameInput->setText("");
        if(name == "") return;

        auto path = Paths::Get(Paths::GAME_SAVES);

        if(path) {
            GameState state{(path.value() / fs::path(name)).string(), std::stoi(seedInput->getText())};

            if(state.getWorldStream()) 
                state.getWorldStream()->setName(name);
        }
        
        this->setUILayer("world_menu");
    };

    newWorldNameInput->onSubmit = [newWorldFunc](std::string){newWorldFunc();};

    auto newWorldButton = getElementById<UIFrame>("create_new_world");
    newWorldButton->onClicked = newWorldFunc;
}