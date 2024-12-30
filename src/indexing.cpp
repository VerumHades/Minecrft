#include <indexing.hpp>

glm::ivec2 SpiralIndexer::next(){
    auto out = current_position;

    auto& value = delta[direction];
    auto& polarity = this->polarity[direction];
    auto& position = current_position[direction];
    

    position += polarity;
    value--;

    if(value == 0){
        value = start_delta + 1;
        polarity *= -1;

        direction = !direction;
        start_delta = delta[direction];
    }
    
    total++;

    return out;
}

glm::ivec3 SpiralIndexer3D::next(){
    if(abs(current_layer) >= current_distance){
        current_distance += 1;
        current_layer = 0;
    }

    auto& current_layers = layers[current_layer_direction];
    if(current_layer >= current_layers.size()) current_layers.push_back({});

    auto& layer = current_layers[current_layer];
    
    if(layer.getTotal() >= pow(current_distance * 2, 2)){
        if(current_layer_direction) current_layer++;
                
        current_layer_direction = !current_layer_direction;

        auto& current_layers = layers[current_layer_direction];
        if(current_layer >= current_layers.size()) current_layers.push_back({});
        layer = current_layers[current_layer];
    }
    
    auto out = layer.next();
    return {out.x, current_layer * (1 + current_layer_direction * -2), out.y};
}