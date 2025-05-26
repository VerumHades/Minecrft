#pragma once

#include <string>
#include <iostream>
#include <optional>
#include <cstring>
#include <vector>

#include <logging.hpp>

/**
 * @brief A generic class for handling images and image data
 * 
 */
class Image{
    private:
        std::vector<unsigned char> data;
        int width = 0;
        int height = 0;
        int nrChannels = 0;

        bool loaded = true;
    
    public:
        Image(const std::string& path);
        Image(){loaded = false;}
        Image(int width, int height, int nrChannels);
        Image(unsigned char* data, int width, int height, int nrChannels);
        Image(std::vector<unsigned char> data, int width, int height, int nrChannels);

        /**
         * @brief Returns a pointer to the first value in the pixels data
         * 
         * @param x 
         * @param y 
         * @return unsigned* 
         */
        unsigned char* getPixel(int x, int y);

        int getWidth() const {return width;}
        int getHeight() const {return height;}
        int getChannels() const {return nrChannels;}

        void save(const std::string& path) const;

        /**
         * @brief Reduces a scaled pixel art image down to its original size
         * 
         * @param input 
         * @return std::optional<Image> 
         */
        static std::optional<Image> reduceToPerfectPixelsAuto(Image& input);

        /**
         * @brief Reduces an image to a set size by sampling individual pixels
         * 
         * @param input 
         * @param width 
         * @param height 
         * @return Image 
         */
        static Image perfectPixelReduce(Image& input, int width, int height);

        static Image pixelPerfectUpscale(Image& image, int width, int height);

        /**
         * @brief Loads an image and automatically adjusts its size to the desired one
         * 
         * @param path 
         * @param width 
         * @param height 
         * @return Image 
         */
        static Image LoadWithSize(const std::string& path, int width, int height);

        const unsigned char* getData() const {return data.data();};
        bool isLoaded() const {return loaded;}
};