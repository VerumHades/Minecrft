#include <ui/loader.hpp>

using namespace tinyxml2;

bool loadWindowFromXML(std::string path){
    XMLDocument doc;

    if (doc.LoadFile(path) != XML_SUCCESS) {
        std::cerr << "Error loading ui XML file." << std::endl;
        return false;
    }

    // Get the root element (bookstore)
    XMLElement* root = doc.FirstChildElement("window");
    if (!root) {
        std::cerr << "No root window element found." << std::endl;
        return false;
    }

    // Iterate over all 'book' elements in 'bookstore'
    for (XMLElement* book = root->FirstChildElement("book"); book != nullptr; book = book->NextSiblingElement("book")) {
        const char* title = book->FirstChildElement("title")->GetText();
        const char* author = book->FirstChildElement("author")->GetText();
        const char* year = book->FirstChildElement("year")->GetText();

        std::cout << "Title: " << title << "\n"
                  << "Author: " << author << "\n"
                  << "Year: " << year << "\n" << std::endl;
    }

    return true;
}