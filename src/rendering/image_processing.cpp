#include <rendering/image_processing.hpp>

#include <stb_image_write.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Image::Image(std::string path){
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 4);
   
    if (!data) {
        std::cerr << "Failed to load image: " << path << std::endl;
        return;
    }

    nrChannels = 4;
    this->data = std::vector(data, data + (width * height * nrChannels));
    stbi_image_free(data);
}

unsigned char* Image::getPixel(int x, int y){
    if(x < 0 || x >= width || y < 0 || y >= height) return nullptr;

    return data.data() + (x + y * width) * nrChannels;
}

Image::Image(std::vector<unsigned char> data, int width, int height, int nrChannels):
    data(data), width(width), height(height), nrChannels(nrChannels) {}

Image::Image(unsigned char* data, int width, int height, int nrChannels): width(width), height(height), nrChannels(nrChannels)
{
    this->data = std::vector(data, data + width * height * nrChannels);
}

Image::Image(int width, int height, int nrChannels): width(width), height(height), nrChannels(nrChannels)
{
    this->data = std::vector<unsigned char>(width * height * nrChannels, 255);
}

void Image::save(std::string path){
    stbi_write_png(path.c_str(), width, height, nrChannels, data.data(), width);
}

std::optional<Image> Image::reduceToPerfectPixelsAuto(Image& input){
    if(input.width != input.height) return std::nullopt;

    int pixel_size = 1;
    unsigned char* first_pixel = input.getPixel(0,0);
    while(
        pixel_size < input.width &&
        memcmp(first_pixel, input.getPixel(pixel_size, 0), input.nrChannels) == 0 &&
        memcmp(first_pixel, input.getPixel(0, pixel_size), input.nrChannels) == 0
    ) pixel_size++;
    
    int scaled_size = input.width / pixel_size;
    Image output(scaled_size, scaled_size, input.nrChannels);

    for(int x = 0;x < scaled_size;x++)
    for(int y = 0;y < scaled_size;y++)
    {
        unsigned char* pixel = input.getPixel(x * pixel_size, y * pixel_size);

        memcpy(output.getPixel(x,y), pixel, input.nrChannels);
    }

    return output;
}

Image Image::perfectPixelReduce(Image& input, int width, int height){
    if(input.width == width && input.height == height) return input;
    
    int pixel_width  = input.width /  width;
    int pixel_height = input.height / height;

    Image output(width, height, input.nrChannels);

    for(int x = 0;x < width;x++)
    for(int y = 0;y < height;y++)
    {
        unsigned char* pixel = input.getPixel(x * pixel_width, y * pixel_height);

        memcpy(output.getPixel(x,y), pixel, input.nrChannels);
    }

    //std::cout << "Image: " << input.nrChannels << std::endl;

    //output.save("out_temp.png");

    return output;
}