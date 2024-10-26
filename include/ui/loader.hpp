#pragma once

#include <ui/manager.hpp>
#include <tinyxml2.h>
#include <regex>
/*
    Loads a window its layers and elements from an xml source file.
*/
bool loadWindowFromXML(UIWindow& window, std::string path);


