#include <mesh.h>

Mesh* newMesh3D(){
    Mesh* mesh = calloc(1,sizeof(Mesh));

    mesh->vertexMap = newPositionMap();

    mesh->faceCount = 0;
    mesh->faceSize = 1;
    mesh->faces = calloc(1,sizeof(MeshFace));
    
    mesh->vertices = calloc(5000,sizeof(float));
    mesh->vertices_count = 0;
    mesh->vertices_size = 5000;

    mesh->indices = calloc(5000,sizeof(float));
    mesh->indices_count = 0;
    mesh->indices_size = 5000;

    mesh->vertex_format = NULL;
    mesh->format_size = 0;

    return mesh;
}

void setVertexFormat(Mesh* mesh, int sizes[], int count){
    mesh->format_size = count;
    mesh->vertex_format = calloc(mesh->format_size, sizeof(int));

    int total = 0;

    for(int i = 0;i < count;i++){
        mesh->vertex_format[i] = sizes[i];
        total += sizes[i];
    }

    mesh->vertex_size = total;
}

void destoryMesh(Mesh* mesh){
    free(mesh->vertices);
    free(mesh->indices);
    free(mesh->faces);

    freePositionMap(mesh->vertexMap);

    if(mesh->vertex_format != NULL) free(mesh->vertex_format);

    free(mesh);
}

static inline void requireVerticesSize(Mesh* mesh, size_t size){
    while(mesh->vertices_size <= mesh->vertices_count+size){ // Real array is too small, double the size
        mesh->vertices_size  *= 2;
        mesh->vertices = realloc(mesh->vertices, mesh->vertices_size * sizeof(float));
    }
}

static inline void addIndexValueToMesh(Mesh* mesh, int value){
    if(mesh->indices_size <= mesh->indices_count+1){ // Real array is too small, double the size
        mesh->indices_size  *= 2;
        mesh->indices = realloc(mesh->indices, mesh->indices_size * sizeof(unsigned int));
    }

    mesh->indices[mesh->indices_count] = value;
    mesh->indices_count++;
}

// Return vertex index
static inline int getVertexFromMesh(Mesh* mesh, Vertex vertex){
    if(vertex.size != mesh->vertex_size){
        printf("Mesh takes vertices of size %i, but a vertex of size %i was given.", mesh->vertex_size, vertex.size);
        return -1;
    }

    int index = mesh->vertices_count / mesh->vertex_size;

    requireVerticesSize(mesh, vertex.size);
    memcpy(mesh->vertices + mesh->vertices_count, vertex.values, sizeof(float) * vertex.size);
    mesh->vertices_count += vertex.size;
    //addVertexValueToMesh(mesh, vertex.values[i]);
    
    return index;
}

static inline void normalVecOffset(Vec3* in, Vec3* offset){
    in->x += 0.1 * offset->x;
    in->y += 0.1 * offset->y;
    in->z += 0.1 * offset->z;
}

static inline void normalVecDeOffset(Vec3* in, Vec3* offset){
    in->x -= 0.1 * offset->x;
    in->y -= 0.1 * offset->y;
    in->z -= 0.1 * offset->z;
}

static inline int getNewFace(Mesh* mesh){
    if(mesh->faceSize < mesh->faceCount + 1){
        mesh->faceSize *= 2;
        mesh->faces = realloc(mesh->faces, sizeof(MeshFace) * mesh->faceSize);
    }

    return mesh->faceCount++;
}

void addQuadFaceToMesh(Mesh* mesh, Vec3 a, Vec3 b, Vec3 c, Vec3 d, Vec3 normals, Vertex metadata){
    normalVecOffset(&a, &normals);
    normalVecOffset(&b, &normals);
    normalVecOffset(&c, &normals);
    normalVecOffset(&d, &normals);
    
    Vec3 vertices[] = {a,b,c,d};
    PositionMap* vMap = mesh->vertexMap;

    for(int i = 0; i < 4; i++){
        Vec3* v1 = &vertices[i];
        Vec3* v2 = &vertices[(i + 1) % 4];

        void* vindex1 = getFromPositionMap(vMap, v1);
        void* vindex2 = getFromPositionMap(vMap, v2);

        if(vindex1 == NULL || vindex2 == NULL) continue;
        
        uintptr_t index1 = (uintptr_t) vindex1;
        uintptr_t index2 = (uintptr_t) vindex2;
        
        if(index1 != index2) continue;

        MeshFace* face = &mesh->faces[index1];
        if(face->metadata.values[0] != metadata.values[0] || face->metadata.values[1] != metadata.values[1]) continue;

        // Accessing both for debugging
        /*printf("%f %f\n", face1->vertices[(i + 3) % 4].x, vertices[(i + 3) % 4].x);
        //face1->vertices[(i + 3) % 4].x = vertices[(i + 3) % 4].x;

        */
        removeFromPositionMap(vMap, &vertices[(i) % 4]);
        removeFromPositionMap(vMap, &vertices[(i + 1) % 4]);

        face->vertices[(i + 2) % 4] = vertices[(i + 2) % 4];
        face->vertices[(i + 3) % 4] = vertices[(i + 3) % 4];
        
        //vertices[(i + 2) % 4] = face1->vertices[(i + 2) % 4];
        //vertices[(i + 3) % 4] = face1->vertices[(i + 3) % 4];

        putIntoPositionMap(vMap, &vertices[(i + 2) % 4], (void*) index1);
        putIntoPositionMap(vMap, &vertices[(i + 3) % 4], (void*) index1);

        return;
    }   

    uintptr_t index = getNewFace(mesh);
    
    MeshFace* face = &mesh->faces[index];
    face->normals = normals;
    memcpy(face->vertices, vertices, sizeof(Vec3) * 4);
    face->metadata = metadata;

    putIntoPositionMap(vMap, &a, (void*) index);
    putIntoPositionMap(vMap, &b, (void*) index);
    putIntoPositionMap(vMap, &c, (void*) index); 
    putIntoPositionMap(vMap, &d, (void*) index);
}   

/*
    textureX, textureY
    textureX + textureSize, textureY,
    textureX + textureSize, textureY + textureSize
    textureX, textureY + textureSize
*/

void constructMesh(Mesh* mesh){
    for(int i = 0;i < mesh->faceCount;i++){
        MeshFace* face = &mesh->faces[i];
        
        int vecIndicies[4];

        for(int i = 0;i < 4;i++){
            Vec3 pos = face->vertices[i];
            normalVecDeOffset(&pos, &face->normals);
            
            Vertex vertex = {0};
            vertex.size = 11;
            
            vertex.values[0] = pos.x;
            vertex.values[1] = pos.y;
            vertex.values[2] = pos.z;

            vertex.values[3] = face->normals.x;
            vertex.values[4] = face->normals.y;
            vertex.values[5] = face->normals.z;

            float textureX = face->metadata.values[0];
            float textureY = face->metadata.values[1];

            float* textureCoodinates = (float[][2]){
                {textureX, textureY},
                {textureX + textureSize, textureY},
                {textureX + textureSize, textureY + textureSize},
                {textureX, textureY + textureSize},
            }[i];
            
            vertex.values[6] = textureCoodinates[0];
            vertex.values[7] = textureCoodinates[1];

            memcpy(vertex.values + 8, face->metadata.values + 2, face->metadata.size * sizeof(float));

            vecIndicies[i] = getVertexFromMesh(mesh, vertex);
        }

        addIndexValueToMesh(mesh, vecIndicies[0]);
        addIndexValueToMesh(mesh, vecIndicies[1]);
        addIndexValueToMesh(mesh, vecIndicies[3]);

        addIndexValueToMesh(mesh, vecIndicies[1]);
        addIndexValueToMesh(mesh, vecIndicies[2]);
        addIndexValueToMesh(mesh, vecIndicies[3]);
    }
}