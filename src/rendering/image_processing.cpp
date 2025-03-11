#include <rendering/image_processing.hpp>

#include <stb_image_write.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Image::Image(const std::string& path){
    int channels_originaly = 0;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels_originaly, 4);
   
    if (!data) {
        LogError("Failed to load image (Image): '{}' error: {}", path, stbi_failure_reason());
        loaded = false;
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

void Image::save(const std::string& path) const{
    stbi_write_png(path.c_str(), width, height, nrChannels, data.data(), width * nrChannels);
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

    for(int y = 0;y < scaled_size;y++)
    for(int x = 0;x < scaled_size;x++)
    {
        unsigned char* pixel = input.getPixel(x * pixel_size, y * pixel_size);

        memcpy(output.getPixel(x,y), pixel, input.nrChannels);
    }

    return output;
}

Image Image::perfectPixelReduce(Image& input, int width, int height){
    int pixel_width  = input.width /  width;
    int pixel_height = input.height / height;

    Image output(width, height, input.nrChannels);

    for(int y = 0;y < height;y++)
    for(int x = 0;x < width;x++)
    {
        unsigned char* pixel = input.getPixel(x * pixel_width, y * pixel_height);

        memcpy(output.getPixel(x,y), pixel, input.nrChannels);
    }

    //std::cout << "Image: " << input.nrChannels << std::endl;

    //output.save("out_temp.png");

    return output;
}

Image Image::pixelPerfectUpscale(Image& input, int width, int height){
    if(input.width > width || input.height > height) {
        LogError("Cannot upsale to a smaller size.");
        return input;
    }
    if(!input.isLoaded()){
        LogError("Cannot upsale to image that isnt loaded.");
        return input;
    }
    Image output{width, height, input.nrChannels};

    int pixel_width_x = ceil((float)width  / (float)input.width);
    int pixel_width_y = ceil((float)height / (float)input.height);

    int x_counter = 0;
    int y_counter = 0;

    int source_x = 0;
    int source_y = 0;
    
    //std::cout << input.width << "x" << input.height << " " << width << "x" << height << std::endl;
    //std::cout << pixel_width_x << " " << pixel_width_y << " " << input.nrChannels << std::endl;

    for(int y = 0;y < height;y++)
    for(int x = 0;x < width;x++)
    {
        if(x_counter >= pixel_width_x){
            x_counter = 0;

            source_x++;
            if(source_x >= input.width){
                source_x = 0;
                y_counter++;
            }
        }
        if(y_counter >= pixel_width_y){
            y_counter = 0;
            source_y++;
        }
        
        unsigned char* pixel = input.getPixel(source_x, source_y);
        memcpy(output.getPixel(x,y), pixel, input.nrChannels);

        x_counter++;
    }

    return output;
}

Image Image::LoadWithSize(const std::string& path, int width, int height){
    Image image{path};

    if(image.width == width && image.height == height) return image;
    if(image.width > width) return perfectPixelReduce(image, width, height);
    return pixelPerfectUpscale(image, width, height);
}