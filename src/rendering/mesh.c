#include <rendering/mesh.h>

Mesh* newMesh3D(){
    Mesh* mesh = calloc(1,sizeof(Mesh));

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

void addQuadFaceToMesh(Mesh* mesh, Vec3 vertices[4], Vec3 normals, Vertex metadata, int clockwise, int width, int height){
    int vecIndicies[4];
        
    for(int i = 0;i < 4;i++){
        Vec3 pos = vertices[i];

        Vertex vertex = {0};
        vertex.size = 12;
        
        vertex.values[0] = pos.x;
        vertex.values[1] = pos.y;
        vertex.values[2] = pos.z;

        vertex.values[3] = normals.x;
        vertex.values[4] = normals.y;
        vertex.values[5] = normals.z;

        float textureX = metadata.values[0];
        float textureY = metadata.values[1];

        float* textureCoodinates = (float[][2]){
            {textureX, textureY},
            {textureX + width, textureY},
            {textureX + width, textureY + height},
            {textureX, textureY + height},
        }[i];
        
        vertex.values[6] = textureCoodinates[0];
        vertex.values[7] = textureCoodinates[1];

        memcpy(vertex.values + 8, metadata.values + 2, (metadata.size - 2) * sizeof(float));

        vecIndicies[i] = getVertexFromMesh(mesh, vertex);
    }

    if(clockwise){
        addIndexValueToMesh(mesh, vecIndicies[0]);
        addIndexValueToMesh(mesh, vecIndicies[1]);
        addIndexValueToMesh(mesh, vecIndicies[3]);

        addIndexValueToMesh(mesh, vecIndicies[1]);
        addIndexValueToMesh(mesh, vecIndicies[2]);
        addIndexValueToMesh(mesh, vecIndicies[3]);
    }
    else{
        addIndexValueToMesh(mesh, vecIndicies[3]);
        addIndexValueToMesh(mesh, vecIndicies[1]);
        addIndexValueToMesh(mesh, vecIndicies[0]);

        addIndexValueToMesh(mesh, vecIndicies[3]);
        addIndexValueToMesh(mesh, vecIndicies[2]);
        addIndexValueToMesh(mesh, vecIndicies[1]);
    }
}   

/*
    textureX, textureY
    textureX + textureSize, textureY,
    textureX + textureSize, textureY + textureSize
    textureX, textureY + textureSize
*/