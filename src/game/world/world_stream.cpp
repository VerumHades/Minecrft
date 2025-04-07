
#include <game/world/world_stream.hpp>

WorldStream::WorldStream(const std::string& path) : FileStream() {
    SetCallbacks(
        [this](FileStream* stream) {
            record_store.SetBuffer(this);
            record_store.ResetHeader();
        },
        [this](FileStream* stream) { record_store.SetBuffer(this); });
}

glm::ivec3 WorldStream::GetSegmentPositionFor(const glm::ivec3& position) {
    return glm::floor(glm::vec3(position) / static_cast<float>(segment_size));
}

bool WorldStream::LoadSegment(const glm::ivec3& position) {
    SegmentRecordStore::Record* record = nullptr;

    {
        std::unique_lock lock(record_mutex);
        record = record_store.Get(position);
    }

    if (!record) return false;

    ByteArray array{};
    {
        std::unique_lock lock(record_mutex);
        array.Vector().resize(record->used_size);
        Read(record->location, record->used_size, array.Data());
    }

    std::unique_ptr<SegmentPack> pack = std::make_unique<SegmentPack>();
    OctreeSerializer<Chunk>::Deserialize(pack->segment, array);

    LoadSegmentToCache(position, std::move(pack));

    return true;
}

void WorldStream::SaveSegment(const glm::ivec3& position, const SegmentPack& segment) {
    ByteArray array{};
    OctreeSerializer<Chunk>::Serialize(segment.segment, array);

    {
        std::unique_lock lock(record_mutex);
        record_store.Save(position, array.Size(), array.Data(), array.Size());
    }
}

WorldStream::SegmentPack* WorldStream::GetSegment(const glm::ivec3& position, bool set_in_use) {
    std::unique_ptr<SegmentPack>* cached = nullptr;

    std::shared_lock lock(record_mutex);
    cached = segment_cache.Get(position);
    if (!cached)
        return nullptr;
    if (set_in_use)
        (*cached)->in_use_by++;

    return cached->get();
}


void WorldStream::LoadSegmentToCache(const glm::ivec3& position, std::unique_ptr<SegmentPack> segment) {
    std::optional<std::pair<glm::ivec3, std::unique_ptr<SegmentPack>>> evicted = std::nullopt;

    {
        std::unique_lock lock(record_mutex);
        evicted = segment_cache.Load(position, std::move(segment));
    }

    if (evicted) {
        auto& [pos, seg] = evicted.value();

        if (seg->in_use_by > 0) {
            seg->marked_for_deletion = true;
            {
                std::lock_guard lock(marking_mutex);
                marked_for_deletion.insert({position, std::move(seg)});
            }
        } else
            SaveSegment(pos, *seg);
    }
}

void WorldStream::StopUsingSegment(SegmentPack* outer, const glm::ivec3& position) {
    if (outer->marked_for_deletion) {
        std::unique_ptr<SegmentPack> segment = nullptr;

        {
            std::lock_guard lock(marking_mutex);
            segment = std::move(marked_for_deletion.at(position));
            marked_for_deletion.erase(position);
        }

        SaveSegment(position, *segment);
    } else
        outer->in_use_by--;
}

void WorldStream::CreateSegment(const glm::ivec3& position) {
    LoadSegmentToCache(position, std::make_unique<SegmentPack>());
}

int WorldStream::GetSeed() const {
    return record_store.GetHeader().seed;
};
void WorldStream::SetSeed(int value) {
    record_store.GetHeader().seed = value;
}

std::string WorldStream::GetName() const {
    return std::string(record_store.GetHeader().name);
}
void WorldStream::SetName(const std::string& name) {
    if (name.length() > 256) {
        LogError("Max world name length is 256 chars");
        return;
    }
    std::strcpy(record_store.GetHeader().name, name.c_str()); // Boo unsafe
}

bool WorldStream::Save(std::unique_ptr<Chunk> chunk) {
    auto position = chunk->getWorldPosition();
    auto segment_position = GetSegmentPositionFor(position);

    SegmentPack* segment_pack = nullptr;
    {
        std::shared_lock lock(record_mutex);
        segment_pack = GetSegment(segment_position, true);
    }

    if (!segment_pack) {
        if (!LoadSegment(segment_position))
            CreateSegment(segment_position);

        segment_pack = GetSegment(segment_position, true);
    }

    if (!segment_pack) {
        LogError("Failed to load world segment.");
        return false;
    }

    {
        std::unique_lock lock(segment_pack->segment_mutex);
        segment_pack->segment.Set(position - segment_position * segment_size, std::move(chunk));
    }

    StopUsingSegment(segment_pack, position);

    return true;
}

std::unique_ptr<Chunk> WorldStream::Load(const glm::ivec3& position) {
    auto segment_position = GetSegmentPositionFor(position);

    SegmentPack* segment_pack = nullptr;
    {
        std::shared_lock lock(record_mutex);
        segment_pack = GetSegment(segment_position, true);
    }

    if (!segment_pack)
        return nullptr;

    std::unique_ptr<Chunk> chunk = nullptr;
    {
        std::unique_lock lock(segment_pack->segment_mutex);
        chunk = segment_pack->segment.Pop(position - segment_position * segment_size);
    }

    StopUsingSegment(segment_pack, position);

    return chunk;
}

bool WorldStream::HasChunkAt(const glm::ivec3& position) {
    auto segment_position = GetSegmentPositionFor(position);

    SegmentPack* segment_pack = nullptr;
    {
        std::shared_lock lock(record_mutex);
        segment_pack = GetSegment(segment_position, true);
    }
    if(!segment_pack) return false;

    bool result = false;
    {
        std::shared_lock lock(segment_pack->segment_mutex);
        result = segment_pack->segment.Get(position - segment_position * segment_size) != nullptr;
    }

    StopUsingSegment(segment_pack, position);
    return result;
}

WorldStream::~WorldStream() {}
