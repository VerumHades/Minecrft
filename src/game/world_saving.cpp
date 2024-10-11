#include <game/world_saving.hpp>

template <typename T>
static inline void saveValue(std::ofstream &file, T value){
    file.write(reinterpret_cast<const char*>(&value), sizeof(T));
}

template <typename T>
static inline T readValue(std::ifstream &file) {
    T value;
    file.read(reinterpret_cast<char*>(&value), sizeof(T));
    return value;
}

template <typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
void ByteArray::append(T data){
    byte* bytePtr = reinterpret_cast<byte*>(&data);
    this->data.insert(this->data.end(), bytePtr, bytePtr + sizeof(T));
}

template <typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
void ByteArray::append(std::vector<T> source){
    size_t totalSize = source.size() * sizeof(T);
    size_t currentSize = data.size();

    append<size_t>(source.size());
    
    data.reserve(currentSize + totalSize);

    byte* sourceArray = reinterpret_cast<byte*>(source.data());
    data.insert(data.end(), sourceArray, sourceArray + totalSize * sizeof(T));
}
template <typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
T ByteArray::read(){
    T out;
    if(cursor + sizeof(T) >= data.size()){
        std::cerr << "Invalid bytearray read: " << sizeof(T) << std::endl; 
        return out;
    }

    std::memcpy(&out, data.data() + cursor, sizeof(T));
    cursor += sizeof(T);
    return out;
}
template <typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
std::vector<T> ByteArray::vread(){
    size_t size = read<size_t>();
    size_t totalSize = 1 + size * sizeof(T);

    std::vector<T> out(size);
    if(cursor + totalSize >= data.size()){
        std::cerr << "Invalid bytearray read: " << sizeof(T) << std::endl; 
        return out;
    }

    T* array = reinterpret_cast<T*>(data.data() + cursor + 1);
    out.insert(out.end(), array, array + size);

    cursor += totalSize;

    return out;
}

void ByteArray::write(std::ofstream &file){
    saveValue<char>(file,'|'); // Magic start character
    saveValue<size_t>(file, data.size());
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
}

void ByteArray::read(std::ifstream &file){
    if(readValue<char>(file) != '|'){ // Check for magic start character
        std::cout << "Invalid start of byte array." << std::endl;
    }
    size_t size = readValue<size_t>(file);
    file.read(reinterpret_cast<char*>(&data), size);
}


/*
    Saves a chunk mask in this format:

    int type
    size_t compressed_24bit_count, data...
    size_t compressed_24bit_count_rotated, data...
*/
static inline void saveMask(ByteArray& out, ChunkMask& mask, int type){
    out.append<int>(type);
    out.append(bitworks::compressBitArray3D(mask.segments));
    out.append(bitworks::compressBitArray3D(mask.segmentsRotated));
}

/*
    Saved the chunk:
    size_t layerCount
    float x,y,z
    saved masks using the above function..
*/
ByteArray serializeChunk(Chunk& chunk){  
    ByteArray out;

    size_t layer_count = chunk.getMasks().size();
    out.append(layer_count);

    out.append(chunk.getWorldPosition().x);
    out.append(chunk.getWorldPosition().y);
    out.append(chunk.getWorldPosition().z);

    saveMask(out, chunk.getSolidMask(), -1);
    for(auto& [key, mask]: chunk.getMasks()) saveMask(out, mask, static_cast<int>(mask.block.type));

    return out;
}

void loadChunk(ByteArray& source, World& world){
    size_t layerCount = readValue<size_t>(file);

    glm::vec3 position = {
        readValue<float>(file),
        readValue<float>(file),
        readValue<float>(file)
    };

    world.chunks[position] = std::make_unique<Chunk>(world, position);
    Chunk* chunk = chunks.at(position).get();

    readValue<int>(file);
    BitArray3D solidNormal = bitworks::decompressBitArray3D(source.vread<compressed_24bit>());
    BitArray3D solidRotated = bitworks::decompressBitArray3D(source.vread<compressed_24bit>());

    chunk->getSolidMask().segments = solidNormal;
    chunk->getSolidMask().segmentsRotated = solidRotated;

    for(int layerIndex = 0; layerIndex < layerCount; layerIndex++){
        int type = readValue<int>(file);
        BitArray3D normal = readBitArray3D(file);
        BitArray3D rotated = readBitArray3D(file);

        ChunkMask mask;
        mask.segments = normal;
        mask.segmentsRotated = rotated;
        mask.block = {static_cast<BlockTypes>(type)};

        chunk->getMasks()[static_cast<BlockTypes>(type)] = mask;
    }
}