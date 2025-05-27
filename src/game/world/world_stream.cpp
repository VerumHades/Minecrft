
#include <game/world/world_stream.hpp>

WorldStream::WorldStream(const std::shared_ptr<KeyedStorage<glm::ivec3>>& storage): record_store(storage){

}

glm::ivec3 WorldStream::GetSegmentPositionFor(const glm::ivec3& position) {
    return glm::floor(glm::vec3(position) / static_cast<float>(segment_size));
}

bool WorldStream::LoadSegment(const glm::ivec3& position) {
    ByteArray array{};

    {
        std::unique_lock lock(record_mutex);
        if (!record_store->Get(position, array.Vector())){
            return false;
        }
    }
    std::shared_ptr<SegmentPack> pack = std::shared_ptr<SegmentPack>(new SegmentPack(), [this,position](SegmentPack* pack){
        SaveSegment(position, pack);
        delete pack;
    });
    OctreeSerializer<Chunk>::Deserialize(pack->segment, array);

    LoadSegmentToCache(position, std::move(pack));

    return true;
}

void WorldStream::SaveSegment(const glm::ivec3& position, SegmentPack* segment) {
    ByteArray array{};
    OctreeSerializer<Chunk>::Serialize(segment->segment, array);

    std::unique_lock lock(record_mutex);
    record_store->Save(position, array.Size(), array.Data());
}

std::shared_ptr<WorldStream::SegmentPack> WorldStream::GetSegment(const glm::ivec3& position, bool set_in_use) {
    std::shared_ptr<SegmentPack>* cached = nullptr;

    std::shared_lock lock(record_mutex);
    cached = segment_cache.Get(position);
    if (!cached)
        return nullptr;

    return *cached;
}

void WorldStream::LoadSegmentToCache(const glm::ivec3& position, std::shared_ptr<SegmentPack> segment) {
    segment_cache.Load(position, segment);
}

void WorldStream::CreateSegment(const glm::ivec3& position) {
    LoadSegmentToCache(position, std::make_unique<SegmentPack>());
}

bool WorldStream::Save(std::unique_ptr<Chunk> chunk) {
    if(chunk->GetSimplificationStep() != 1) return true; // A simplified chunk is simply deleted, no point in storing it

    auto position = chunk->getWorldPosition();
    auto segment_position = GetSegmentPositionFor(position);

    auto segment_pack = GetSegment(segment_position, true);

    if (!segment_pack) {
        if (!LoadSegment(segment_position))
            CreateSegment(segment_position);

        segment_pack = GetSegment(segment_position, true);
    }

    if (!segment_pack) {
        LogError("Failed to load world segment.");
        return false;
    }

    glm::ivec3 internal_position = position - segment_position * segment_size;
    {
        std::unique_lock lock(segment_pack->segment_mutex);
        segment_pack->segment.Set(internal_position, std::move(chunk));
    }

    return true;
}

std::unique_ptr<Chunk> WorldStream::Load(const glm::ivec3& position) {
    auto segment_position = GetSegmentPositionFor(position);

    auto segment_pack = GetSegment(segment_position, true);

    if (!segment_pack) {
        if (!LoadSegment(segment_position)) return nullptr;
        segment_pack = GetSegment(segment_position, true);
    }

    std::unique_ptr<Chunk> chunk = nullptr;
    {
        std::unique_lock lock(segment_pack->segment_mutex);
        chunk = segment_pack->segment.Pop(position - segment_position * segment_size);
    }

    return chunk;
}

bool WorldStream::HasChunkAt(const glm::ivec3& position) {
    auto segment_position = GetSegmentPositionFor(position);

    auto segment_pack = GetSegment(segment_position, true);

    if (!segment_pack) {
        if (!LoadSegment(segment_position)) return false;
        segment_pack = GetSegment(segment_position, true);
    }

    glm::ivec3 internal_position = position - segment_position * segment_size;
    bool result = false;
    {
        std::shared_lock lock(segment_pack->segment_mutex);
        result = segment_pack->segment.Get(internal_position) != nullptr;
    }

    return result;
}

void WorldStream::Flush() {
    segment_cache.Clear([this](auto key, auto segment) { SaveSegment(key, segment.get()); });
}

WorldStream::~WorldStream() {
    Flush();
}
