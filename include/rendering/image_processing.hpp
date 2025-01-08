#pragma once

#include <string>
#include <iostream>
#include <optional>
#include <cstring>
#include <vector>

class Image{
    private:
        std::vector<unsigned char> data;
        int width = 0;
        int height = 0;
        int nrChannels = 0;
    
    public:
        Image(std::string path);
        Image(){}
        Image(int width, int height, int nrChannels);
        Image(unsigned char* data, int width, int height, int nrChannels);
        Image(std::vector<unsigned char> data, int width, int height, int nrChannels);

        /*
            Returns a pointer to the first value in the pixels data
        */
        unsigned char* getPixel(int x, int y);

        int getWidth() const {return width;}
        int getHeight() const {return height;}
        int getChannels() const {return nrChannels;}

        void save(std::string path);

        /*
            Reduces a scaled pixel art image down to its original size
        */
        static std::optional<Image> reduceToPerfectPixelsAuto(Image& input);

        /*
            Reduces an image to a set size by sampling individual pixels
        */
        static Image perfectPixelReduce(Image& input, int width, int height);

        static Image pixelPerfectUpscale(Image& image, int ratio);

        const unsigned char* getData() const {return data.data();};
};