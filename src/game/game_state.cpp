#include <game/game_state.hpp>

GameState::GameState(const std::string& path, int worldSeed) : save_structure(path) {
    Entity player = Entity(glm::vec3(0, 30, 0), glm::vec3(0.6, 1.8, 0.6));
    player.addTag("player");
    entities.push_back(player);

    world_stream = save_structure.RegisterSave<WorldStream>("world", "", "world");
    player_stream = save_structure.RegisterSave<FileStream>(
        "player", "", [this](FileStream*) { this->savePlayer(); }, [this](FileStream* stream) { this->loadPlayer(); });

    entity_stream = save_structure.RegisterSave<FileStream>(
        "entities", "", [this](FileStream*) { this->saveEntities(); }, [this](FileStream*) { this->loadEntities(); });

    save_structure.Open();

    if (worldSeed != -1)
        world_stream->SetSeed(worldSeed);
}

void GameState::savePlayer() {
    ByteArray array{};

    Serializer::Serialize<LogicalItemInventory>(player_inventory, array);
    Serializer::Serialize<LogicalItemInventory>(player_hotbar, array);

    player_stream->SetCursor(0);
    array.WriteToStream(*player_stream);
}
void GameState::loadPlayer() {
    ByteArray array{};

    player_stream->SetCursor(0);
    array.LoadFromStream(*player_stream);

    Serializer::Deserialize<LogicalItemInventory>(player_inventory, array);
    Serializer::Deserialize<LogicalItemInventory>(player_hotbar, array);
}

void GameState::saveEntities() {
    ByteArray array{};

    array.Append<size_t>(entities.size());
    for (auto& entity : entities)
        Serializer::Serialize<Entity>(entity, array);

    entity_stream->SetCursor(0);
    array.WriteToStream(*entity_stream);
}

void GameState::loadEntities() {
    entities.clear();

    ByteArray array{};

    entity_stream->SetCursor(0);
    array.LoadFromStream(*entity_stream);

    auto count_opt = array.Read<size_t>();
    if (!count_opt) {
        LogError("Entity file is corrupted, failed to load.");
        return;
    }

    size_t count = count_opt.value();
    for (size_t i = 0; i < count; i++) {
        Entity entity{};
        Serializer::Deserialize<Entity>(entity, array);
        entities.emplace_back(entity);
    }
}

void GameState::giveItemToPlayer(ItemRef item) {
    if (!player_hotbar.addItem(item))
        player_inventory.addItem(item);
}

static int rotation = 0;
static float position = 0;
static float position_mult = 1;

void GameState::drawEntity(Entity& entity) {
    if (!entity.getModel())
        return;

    entity.getModel()->requestDraw(entity.getPosition() + glm::vec3{0, position, 0}, {0.3, 0.3, 0.3}, {0, rotation, 0},
                                   entity.getModel()->getRotationCenterOffset());
}

bool GameState::entityCollision(Entity& checked_entity, const glm::vec3& offset) {
    if (checked_entity.shouldGetDestroyed())
        return false;

    auto& checked_collider = checked_entity.getCollider();
    auto position = checked_entity.getPosition() + offset;

    if (terrain.collision(position, &checked_collider)) {
        if (checked_entity.onTerrainCollision)
            checked_entity.onTerrainCollision(&checked_entity);
        return true;
    }

    for (auto& entity : entities) {
        if (entity.shouldGetDestroyed())
            continue;
        if (&entity == &checked_entity)
            continue;

        auto& entity_collider = entity.getCollider();

        float distance = glm::distance(entity_collider.center, checked_collider.center);
        if (distance > checked_collider.bounding_sphere_radius + entity_collider.bounding_sphere_radius)
            continue; // Definitly dont collide

        if (entity_collider.collidesWith(&checked_collider, entity.getPosition(), position)) {
            if (checked_entity.onCollision)
                checked_entity.onCollision(&checked_entity, &entity);
            if (entity.onCollision)
                entity.onCollision(&entity, &checked_entity);

            if (entity.isSolid())
                return true;
        }
    }

    return false;
}

void GameState::loadChunk(const glm::ivec3& position) {
    auto* chunk = terrain.getChunk(position);
    if (chunk)
        return;

    if (world_stream && world_stream->HasChunkAt(position)) {
        terrain.addChunk(position, world_stream->Load(position));
        return;
    }
}

void GameState::unloadChunk(const glm::ivec3& position) {
    auto chunk = terrain.takeChunk(position);
    if (!chunk)
        return;

    if (world_stream)
        world_stream->Save(chunk);
}

void GameState::unload() {
    for (auto it = terrain.chunks.begin(); it != terrain.chunks.end(); ) {
        auto current = it++;
        unloadChunk(current->first);
    }

    savePlayer();
    saveEntities();
}

void GameState::updateEntity(Entity& entity, float deltatime) {
    if (entity.data && entity.data->do_update)
        entity.data->update(this);

    if (entity.hasGravity)
        entity.accelerate(glm::vec3(0, -(15.0 + entity.friction * 2), 0), deltatime);

    if (entity.velocity.x != 0 || entity.velocity.y != 0 || entity.velocity.z != 0)
        entity.on_ground = false;

    glm::vec3& vel = entity.velocity;

    float relative_friction = entity.friction * deltatime;

    float len = glm::length(vel);
    if (len > relative_friction)
        vel = glm::normalize(vel) * (len - relative_friction);
    else
        vel = glm::vec3(0);

    if (vel.x != 0 && entityCollision(entity, {vel.x * deltatime, 0, 0}))
        vel.x = 0;
    if (vel.y != 0 && entityCollision(entity, {0, vel.y * deltatime, 0}))
        vel.y = 0;
    if (vel.z != 0 && entityCollision(entity, {0, 0, vel.z * deltatime}))
        vel.z = 0;

    entity.position += vel * deltatime;
}

void GameState::updateEntities(float deltatime) {
    std::list<Entity>::iterator i = entities.begin();
    while (i != entities.end()) {
        updateEntity(*i, deltatime);
        if (i->shouldGetDestroyed()) {
            i = entities.erase(i);
            continue;
        }

        drawEntity(*i);
        ++i;
    }

    const float position_addition = 0.002;

    position += position_mult;
    if (position <= 0.1)
        position_mult = position_addition;
    if (position >= 0.4)
        position_mult = -position_addition;

    rotation = (rotation + 1) % 360;
}

/*void GameState::save(){
    for(auto& [position,chunk]: chunks){
        if(!chunk->wasAltered()) continue;
        chunk->resetAlteredFlag();
        stream->save(*chunk);
    }
}*/
