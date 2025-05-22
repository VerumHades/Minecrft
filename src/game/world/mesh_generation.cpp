#include <basetsd.h>
#include <cstdint>
#include <game/world/mesh_generation.hpp>

void ChunkMeshGenerator::addToChunkMeshLoadingQueue(glm::ivec3 position, std::unique_ptr<MeshInterface> mesh) {
    std::lock_guard<std::mutex> lock(meshLoadingMutex);
    meshLoadingQueue.push({position, std::move(mesh)});
}
bool ChunkMeshGenerator::loadMeshFromQueue(RegionCuller& buffer, size_t limit) {
    if (!meshes_pending)
        return false;

    std::lock_guard<std::mutex> lock(meshLoadingMutex);

    if (meshLoadingQueue.empty()) {
        meshes_pending = false;
        return false;
    }

    for (size_t i = 0; i < std::min(limit, meshLoadingQueue.size()); i++) {
        auto& [position, mesh] = meshLoadingQueue.front();

        if (buffer.addMesh(mesh.get(), position)) {
            meshLoadingQueue.pop();
        }
    }

    if (meshLoadingQueue.empty())
        meshes_pending = false;

    return true;
}

bool ChunkMeshGenerator::syncGenerateAsyncUploadMesh(Chunk* chunk,
                                                     std::unique_ptr<MeshInterface> mesh,
                                                     BitField3D::SimplificationLevel simplification_level) {
    //auto start = std::chrono::high_resolution_clock::now();

    auto world_position = chunk->getWorldPosition();
    bool result         = generateChunkMesh(world_position, mesh.get(), chunk, simplification_level);

    if (!result)
        return false;

    addToChunkMeshLoadingQueue(world_position, std::move(mesh));
    meshes_pending = true;

    return true;
}

bool ChunkMeshGenerator::syncGenerateSyncUploadMesh(Chunk* chunk,
                                                    RegionCuller& buffer,
                                                    std::unique_ptr<MeshInterface> mesh,
                                                    BitField3D::SimplificationLevel simplification_level) {
    auto world_position = chunk->getWorldPosition();
    bool result         = generateChunkMesh(world_position, mesh.get(), chunk, simplification_level);

    if (!result)
        return false;
    buffer.addMesh(mesh.get(), chunk->getWorldPosition());

    return true;
}

/*
    Generate greedy meshed faces from a plane of bits
*/
std::vector<ChunkMeshGenerator::Face>& ChunkMeshGenerator::greedyMeshPlane(BitPlane<64> rows, int start_row, int end_row) {
    const int size = 64;

    static ThreadLocal<std::vector<ChunkMeshGenerator::Face>> greedy_mesh_plane_threadlocal{};

    auto& greedy_mesh_plane_out = greedy_mesh_plane_threadlocal.Get();
    greedy_mesh_plane_out.clear();

    /*
        Moves bits to from the right to the left

        if size = 4
        00001111
        then it gets converted to this for meshing (because count_leading_zeros
    starts from the left ) 11110000

    if(size != 64){
        int size_to_real_size_adjustment = 64 - size;
        for(int i = 0;i < size;i++) rows[i] <<= size_to_real_size_adjustment;
    }
    */

    int currentRow = start_row;

    while (currentRow < end_row) {
        uint64_t row = rows[currentRow];
        /*
            0b00001101

            'start' is 4
        */
        uint8_t start = std::min(count_leading_zeros(row), (uint8_t)size); // Find the first
        if (start == size) {
            currentRow++;
            continue;
        }
        /*
            2. 0b11010000 shift by 4
            1. 0b00101111 negate

            'width' is 2
        */
        row <<= start; // Shift so the faces start
        uint8_t width = std::min(count_leading_zeros(~row),
                                 static_cast<uint8_t>(size - start)); // Calculate width (negated counts '1')
        row >>= start;                                                // Return to original position

        /*
                Create a mask of all ones
                1. 0b11111111
                2. 0b00000011 shift beyond the face (start + width)
                3. 0b11111100 negate

                    0b11111100 & 0b00001101

                4. 0b00001100 AND with the row to create the faces mask
        */
        uint64_t mask = ~0ULL;

        //  Shifting by 64 is undefined behaviour for some reason ¯\_(ツ)_/¯
        if ((start + width) != size)
            mask = ~(mask >> (start + width));
        mask &= row;

        int height = 0;
        while (currentRow + height < end_row && (mask & rows[currentRow + height]) == mask) {
            rows[currentRow + height] &= ~mask; // Remove this face part from the rows
            height++;
        }

        greedy_mesh_plane_out.push_back({// Add the face to the face list
                                         start,
                                         currentRow,
                                         width,
                                         height});

        // return out;
    }

    return greedy_mesh_plane_out;
}

std::tuple<ChunkMeshGenerator::OccludedPlane, bool, ChunkMeshGenerator::OccludedPlane, bool> ChunkMeshGenerator::segregatePlane(
    OccludedPlane& source_plane, OcclusionPlane& occlusion_plane, std::array<bool, 4> affects, glm::ivec2 lookup_offset) {
    OccludedPlane true_plane  = {source_plane.occlusion};
    OccludedPlane false_plane = {source_plane.occlusion};

    true_plane.occlusion[0] += affects[0];
    true_plane.occlusion[1] += affects[1];
    true_plane.occlusion[2] += affects[2];
    true_plane.occlusion[3] += affects[3];

    bool true_plane_empty  = true;
    bool false_plane_empty = true;

    for (int i = source_plane.start; i < source_plane.end; i++) {
        if (source_plane.plane[i] == 0)
            continue;

        int lookup_y = (i + lookup_offset.y) + 1; // occlusion plane is offset relative to source plane so +1

        uint64_t occlusion_row = occlusion_plane.rows[lookup_y];

        // occlusion_row =
        //     (lookup_offset.x > 0) ?
        //         (occlusion_row <<  lookup_offset.x):
        //         (occlusion_row >> -lookup_offset.x);

        if (lookup_offset.x > 0) {
            occlusion_row <<= lookup_offset.x;

            if (lookup_y == 0)
                occlusion_row |= occlusion_plane.top_right_corner;
            else if (lookup_y == 65)
                occlusion_row |= occlusion_plane.bottom_right_corner;
            else
                occlusion_row |= ((occlusion_plane.right >> (lookup_y - 1)) & 1ULL);
        } else {
            occlusion_row >>= -lookup_offset.x;

            if (lookup_y == 0)
                occlusion_row |= occlusion_plane.top_left_corner;
            else if (lookup_y == 65)
                occlusion_row |= occlusion_plane.bottom_left_corner;
            else
                occlusion_row |= ((occlusion_plane.left >> (lookup_y - 1)) & 1ULL);
        }

        true_plane.plane[i]  = source_plane.plane[i] & occlusion_row;
        false_plane.plane[i] = source_plane.plane[i] & ~occlusion_row;

        if (true_plane.plane[i] != 0ULL) {
            true_plane.end   = i + 1;
            true_plane_empty = false;
        }
        if (false_plane.plane[i] != 0ULL) {
            false_plane.end   = i + 1;
            false_plane_empty = false;
        }

        if (true_plane_empty)
            true_plane.start = i;
        if (false_plane_empty)
            false_plane.start = i;
    }

    return {true_plane, true_plane_empty, false_plane, false_plane_empty};
}

struct OcclusionOffset {
    glm::ivec2 offset;
    std::array<bool, 4> affects;
};
const static std::array<OcclusionOffset, 8> occlusion_offsets = {
    // Whole sides
    OcclusionOffset{{1, 0}, {0, 1, 1, 0}},
    OcclusionOffset{{-1, 0}, {1, 0, 0, 1}},
    OcclusionOffset{{0, 1}, {0, 0, 1, 1}},
    OcclusionOffset{{0, -1}, {1, 1, 0, 0}},
    // Corners
    OcclusionOffset{{-1, 1}, {0, 0, 0, 1}},
    OcclusionOffset{{1, 1}, {0, 0, 1, 0}},
    OcclusionOffset{{1, -1}, {0, 1, 0, 0}},
    OcclusionOffset{{-1, -1}, {1, 0, 0, 0}}};

std::vector<ChunkMeshGenerator::OccludedPlane>& ChunkMeshGenerator::calculatePlaneAmbientOcclusion(BitPlane<64>& source_plane,
                                                                                                   OcclusionPlane& occlusion_plane) {

    static ThreadLocal<std::vector<ChunkMeshGenerator::OccludedPlane>> occluded_planes_threadlocal{};

    auto& occluded_planes_out = occluded_planes_threadlocal.Get();
    occluded_planes_out.clear();
    occluded_planes_out.push_back({{0, 0, 0, 0}, source_plane});

    for (auto& [offset, affects] : occlusion_offsets) {
        for (auto& plane : occluded_planes_out) {
            auto [plane_1, plane_1_empty, plane_2, plane_2_empty] = segregatePlane(plane, occlusion_plane, affects, offset);

            if (plane_1_empty) {
                plane = plane_2;
                continue;
            }

            plane = plane_1;

            if (plane_2_empty)
                continue;
            occluded_planes_out.push_back(plane_2);
        }
    }

    return occluded_planes_out;
}

static inline void processFaces(const std::vector<ChunkMeshGenerator::Face>& faces,
                                MeshInterface::FaceType face_type,
                                MeshInterface::Direction direction,
                                BlockRegistry::BlockPrototype* type,
                                MeshInterface* mesh,
                                glm::vec3 world_position,
                                int layer,
                                std::array<float, 4>& occlusion) {
    glm::vec3 face_position;
    int texture = 0;

    bool texture_index = direction == MeshInterface::Backward;

    mesh->preallocate(faces.size(), face_type);
    for (auto& face : faces) {
        int faceWidth  = face.width;
        int faceHeight = face.height;

        switch (face_type) {
        case MeshInterface::X_ALIGNED:
            face_position = glm::vec3(layer + 1, face.y + face.height, face.x) + world_position;

            texture = type->single_texture ? type->textures[0] : type->textures[4 + texture_index];

            break;
        case MeshInterface::Y_ALIGNED:
            face_position = glm::vec3(face.y, layer + 1, face.x) + world_position;

            faceWidth  = face.height;
            faceHeight = face.width;

            texture = type->single_texture ? type->textures[0] : type->textures[texture_index];
            break;
        case MeshInterface::Z_ALIGNED:
            face_position = glm::vec3(face.x, face.y + face.height, layer + 1) + world_position;

            texture = type->single_texture ? type->textures[0] : type->textures[2 + texture_index];
            break;
        case MeshInterface::BILLBOARD:
            break;
        }

        mesh->addQuadFace(face_position, faceWidth, faceHeight, texture, face_type, direction, occlusion);
    }
}

void ChunkMeshGenerator::proccessOccludedFaces(BitPlane<64>& source_plane,
                                               OcclusionPlane& occlusion_plane,
                                               MeshInterface::FaceType face_type,
                                               MeshInterface::Direction direction,
                                               BlockRegistry::BlockPrototype* type,
                                               MeshInterface* mesh,
                                               glm::vec3 world_position,
                                               int layer) {
    auto& processed_planes = calculatePlaneAmbientOcclusion(source_plane, occlusion_plane);

    for (auto& plane : processed_planes) {
        processFaces(greedyMeshPlane(plane.plane, plane.start, plane.end),
                     face_type,
                     direction,
                     type,
                     mesh,
                     world_position,
                     layer,
                     plane.occlusion);
    }
}

std::tuple<uint64_t, uint64_t> ProcessFaceRowValues(
    uint64_t solid_forward, uint64_t solid_backward, uint64_t field_forward, uint64_t field_backward, bool transparent) {

    if (transparent) {
        uint64_t allFacesX = (field_forward ^ field_backward) & ~(solid_forward | solid_backward);
        return {field_forward & allFacesX, field_backward & allFacesX};
    }
    uint64_t possible_mask = solid_forward ^ solid_backward;

    return std::tuple<uint64_t, uint64_t>{field_forward & possible_mask, field_backward & possible_mask};
}

std::tuple<uint64_t, uint64_t> ProcessFaceRow(int layer,
                                              int row,
                                              const std::function<uint64_t(int layer, int row)>& solid_getter,
                                              const std::function<uint64_t(int layer, int row)>& field_getter,
                                              bool transparent) {
    return ProcessFaceRowValues(
        solid_getter(layer, row), solid_getter(layer + 1, row), field_getter(layer, row), field_getter(layer + 1, row), transparent);
}

#define AGREGATE_TYPES(axis)                                                                                                          \
    std::unordered_set<BlockID> agregateTypes##axis{};                                                                                \
    agregateTypes##axis.insert(chunk->getPresentTypes().begin(), chunk->getPresentTypes().end());

bool ChunkMeshGenerator::generateChunkMesh(const glm::ivec3& worldPosition,
                                           MeshInterface* solidMesh,
                                           Chunk* chunk,
                                           BitField3D::SimplificationLevel simplification_level) {
    if (!chunk) {
        LogError("Missing chunk when generating mesh.");
        return false;
    }

    if (!world) {
        LogError("No world assigned, quitting mesh generation.");
        return false;
    }

    // ScopeTimer timer("Generated mesh");
    chunk->current_simplification = simplification_level;

    // this->solidMesh->setVertexFormat(VertexFormat({3,1,2,1,1}));  // Unused

    glm::vec3 world_position = worldPosition * CHUNK_SIZE;
    int size                 = CHUNK_SIZE;

    // Occlusion planes have to reach into other chunks
    std::array<OcclusionPlane, 64> occlusionPlanesX{};
    std::array<OcclusionPlane, 64> occlusionPlanesY{};
    std::array<OcclusionPlane, 64> occlusionPlanesZ{};

    { // Scope becuse transposed field doesnt neccesairly remain valid when new
        // ones are created
        auto& solidField   = *chunk->getSolidField().getSimplifiedWithNone(simplification_level);
        auto* solidRotated = solidField.getTransposed();
        // 64 planes internal 65th external

        auto slock  = solidField.Guard().Shared();
        auto srlock = solidRotated->Guard().Shared();

        for (int x = 0; x < 64; x++)
            for (int y = 0; y < 64; y++) {
                occlusionPlanesX[x].rows[y + 1] = solidField.getRow(x, y);
                occlusionPlanesY[y].rows[x + 1] = solidField.getRow(x, y);
                occlusionPlanesZ[x].rows[y + 1] = solidRotated->getRow(x, y);
            }
    }
    // timer.timestamp("Generated occlusion planes");

    // std::cout << "Scale: " << scale << " size: " << size << std::endl;
    /*
        Mesh chunk faces
    */

    BitPlane planeXforward  = {};
    BitPlane planeXbackward = {};

    BitPlane planeYforward  = {};
    BitPlane planeYbackward = {};

    BitPlane planeZforward  = {};
    BitPlane planeZbackward = {};

    /**
     * @brief Generate faces for each block type
     * 
     * @param field_layer 
     */
    for (auto& field_layer : chunk->getLayers()) {
        auto& [type, block, _field] = field_layer;

        auto& field        = *field_layer.field().getSimplifiedWithNone(simplification_level);
        auto* rotatedField = field.getTransposed();

        auto& solidField   = *chunk->getSolidField().getSimplifiedWithNone(simplification_level);
        auto* solidRotated = solidField.getTransposed();

        auto l1 = field.Guard().Shared();
        auto l2 = rotatedField->Guard().Shared();
        auto l3 = solidField.Guard().Shared();
        auto l4 = solidRotated->Guard().Shared();

        auto* definition = BlockRegistry::get().getPrototype(type);
        if (!definition || definition->render_type != BlockRegistry::FULL_BLOCK) {

            if (!definition || definition->render_type != BlockRegistry::BILLBOARD || simplification_level != BitField3D::NONE)
                continue;

            for (int x = 0; x < size; x++)
                for (int y = 0; y < size; y++)
                    for (int z = 0; z < size; z++) {
                        if (!field.get(x, y, z))
                            continue;

                        glm::vec3 position = glm::vec3{x, y, z} + world_position;

                        solidMesh->addQuadFace(
                            position, 1, 1, definition->textures[0], MeshInterface::BILLBOARD, MeshInterface::Forward, {0, 0, 0, 0});
                    }

            continue;
        }

        /**
         * @brief For each layer create planes from every row for potential faces then mesh them
         * 
         */
        for (int layer = 0; layer < size - 1; layer++) {
            for (int row = 0; row < size; row++) {
                std::tie(planeXforward[row], planeXbackward[row]) = ProcessFaceRow(
                    layer,
                    row,
                    [&solidField](int layer, int row) { return solidField.getRow(layer, row); },
                    [&field](int layer, int row) { return field.getRow(layer, row); },
                    definition->transparent);

                std::tie(planeYforward[row], planeYbackward[row]) = ProcessFaceRow(
                    layer,
                    row,
                    [&solidField](int layer, int row) { return solidField.getRow(row, layer); },
                    [&field](int layer, int row) { return field.getRow(row, layer); },
                    definition->transparent);

                std::tie(planeZforward[row], planeZbackward[row]) = ProcessFaceRow(
                    layer,
                    row,
                    [solidRotated](int layer, int row) { return solidRotated->getRow(layer, row); },
                    [rotatedField](int layer, int row) { return rotatedField->getRow(layer, row); },
                    definition->transparent);
            }

            // std::cout << "Solving plane: " << getBlockTypeName(type) <<
            // std::endl; for(int j = 0;j < 64;j++) std::cout <<
            // std::bitset<64>(planes[i][j]) << std::endl;

            proccessOccludedFaces(planeXforward,
                                  occlusionPlanesX[layer + 1],
                                  MeshInterface::X_ALIGNED,
                                  MeshInterface::Forward,
                                  definition,
                                  solidMesh,
                                  world_position,
                                  layer);
            proccessOccludedFaces(planeXbackward,
                                  occlusionPlanesX[layer],
                                  MeshInterface::X_ALIGNED,
                                  MeshInterface::Backward,
                                  definition,
                                  solidMesh,
                                  world_position,
                                  layer);

            proccessOccludedFaces(planeYforward,
                                  occlusionPlanesY[layer + 1],
                                  MeshInterface::Y_ALIGNED,
                                  MeshInterface::Forward,
                                  definition,
                                  solidMesh,
                                  world_position,
                                  layer);
            proccessOccludedFaces(planeYbackward,
                                  occlusionPlanesY[layer],
                                  MeshInterface::Y_ALIGNED,
                                  MeshInterface::Backward,
                                  definition,
                                  solidMesh,
                                  world_position,
                                  layer);

            proccessOccludedFaces(planeZforward,
                                  occlusionPlanesZ[layer + 1],
                                  MeshInterface::Z_ALIGNED,
                                  MeshInterface::Forward,
                                  definition,
                                  solidMesh,
                                  world_position,
                                  layer);
            proccessOccludedFaces(planeZbackward,
                                  occlusionPlanesZ[layer],
                                  MeshInterface::Z_ALIGNED,
                                  MeshInterface::Backward,
                                  definition,
                                  solidMesh,
                                  world_position,
                                  layer);
        }
    }

    ////timer.timestamp("Inner faces");

    ////timer.timestamp("Billboards");
    /*
        Mesh cross chunk faces
    */

    Chunk* nextX = world->getChunk(worldPosition - glm::ivec3{1, 0, 0});
    Chunk* nextY = world->getChunk(worldPosition - glm::ivec3{0, 1, 0});
    Chunk* nextZ = world->getChunk(worldPosition - glm::ivec3{0, 0, 1});

    if (!nextX || !nextY || !nextZ) {
        // LogError("Mesh generating when chunks are missing?");
        return true;
    }

    std::array<float, 4> occlusion = {0, 0, 0, 0};
    // std::cout << nextX << " " << nextY << " " << nextZ << std::endl;

    AGREGATE_TYPES(X);
    AGREGATE_TYPES(Y);
    AGREGATE_TYPES(Z);

    std::unordered_set<BlockID> fullAgregate = agregateTypesX;
    fullAgregate.insert(agregateTypesY.begin(), agregateTypesY.end());
    fullAgregate.insert(agregateTypesZ.begin(), agregateTypesZ.end());

    auto& solidField       = *chunk->getSolidField().getSimplifiedWithNone(simplification_level);
    BitField3D& nextXSolid = *nextX->getSolidField().getSimplifiedWithNone(nextX->current_simplification);
    BitField3D& nextYSolid = *nextY->getSolidField().getSimplifiedWithNone(nextY->current_simplification);

    auto* solidRotated      = solidField.getTransposed();
    auto* nextZSolidRotated = nextZ->getSolidField().getSimplifiedWithNone(nextZ->current_simplification)->getTransposed();

    /**
     * @brief Mesh three of the neighbouring layers, same logic just across chunks
     * 
     * @param fullAgregate 
     */
    for (auto& type : fullAgregate) {
        auto* definition = BlockRegistry::get().getPrototype(type);
        if (!definition || definition->render_type != BlockRegistry::FULL_BLOCK)
            continue;

        auto* layer_field = chunk->getLayer(type).field().getSimplifiedWithNone(simplification_level);

        for (int row = 0; row < size; row++) {
            const uint64_t localMaskRowX = chunk->hasLayerOfType(type) ? layer_field->getRow(0, row) : 0ULL;
            const uint64_t otherMaskRowX =
                nextX->hasLayerOfType(type)
                    ? nextX->getLayer(type).field().getSimplifiedWithNone(nextX->current_simplification)->getRow(size - 1, row)
                    : 0ULL;

            const uint64_t localMaskRowY = chunk->hasLayerOfType(type) ? layer_field->getRow(row, 0) : 0ULL;
            const uint64_t otherMaskRowY =
                nextY->hasLayerOfType(type)
                    ? nextY->getLayer(type).field().getSimplifiedWithNone(nextY->current_simplification)->getRow(row, size - 1)
                    : 0ULL;

            uint64_t localMaskRowZ = chunk->hasLayerOfType(type) ? layer_field->getTransposed()->getRow(0, row) : 0ULL;
            uint64_t otherMaskRowZ = nextZ->hasLayerOfType(type) ? nextZ->getLayer(type)
                                                                       .field()
                                                                       .getSimplifiedWithNone(nextZ->current_simplification)
                                                                       ->getTransposed()
                                                                       ->getRow(size - 1, row)
                                                                 : 0ULL;
            ;

            std::tie(planeXforward[row], planeXbackward[row]) = ProcessFaceRowValues(
                solidField.getRow(0, row), nextXSolid.getRow(63, row), localMaskRowX, otherMaskRowX, definition->transparent);

            std::tie(planeYforward[row], planeYbackward[row]) = ProcessFaceRowValues(
                solidField.getRow(row, 0), nextYSolid.getRow(row, 63), localMaskRowY, otherMaskRowY, definition->transparent);

            std::tie(planeZforward[row], planeZbackward[row]) = ProcessFaceRowValues(solidRotated->getRow(0, row),
                                                                                     nextZSolidRotated->getRow(63, row),
                                                                                     localMaskRowZ,
                                                                                     otherMaskRowZ,
                                                                                     definition->transparent);
        }

        processFaces(greedyMeshPlane(planeXforward),
                     MeshInterface::X_ALIGNED,
                     MeshInterface::Backward,
                     definition,
                     solidMesh,
                     world_position,
                     -1,
                     occlusion);
        processFaces(greedyMeshPlane(planeXbackward),
                     MeshInterface::X_ALIGNED,
                     MeshInterface::Forward,
                     definition,
                     solidMesh,
                     world_position,
                     -1,
                     occlusion);

        processFaces(greedyMeshPlane(planeYforward),
                     MeshInterface::Y_ALIGNED,
                     MeshInterface::Backward,
                     definition,
                     solidMesh,
                     world_position,
                     -1,
                     occlusion);
        processFaces(greedyMeshPlane(planeYbackward),
                     MeshInterface::Y_ALIGNED,
                     MeshInterface::Forward,
                     definition,
                     solidMesh,
                     world_position,
                     -1,
                     occlusion);

        processFaces(greedyMeshPlane(planeZforward),
                     MeshInterface::Z_ALIGNED,
                     MeshInterface::Backward,
                     definition,
                     solidMesh,
                     world_position,
                     -1,
                     occlusion);
        processFaces(greedyMeshPlane(planeZbackward),
                     MeshInterface::Z_ALIGNED,
                     MeshInterface::Forward,
                     definition,
                     solidMesh,
                     world_position,
                     -1,
                     occlusion);
    }
    // std::cout << "Vertices:" << solidMesh->getIndices().size() << std::endl;

    solidMesh->shrink();

    return true;
}
