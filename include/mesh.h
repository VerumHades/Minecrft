#ifndef MESH_HEADER
#define MESH_HEADER

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <posmap.h>

#define vertices_size(vertex) vertex->int_value

typedef struct Vertex{
    float values[16];
    int size;
} Vertex;

typedef struct MeshFace{
    Vec3 vertices[4];
    Vec3 normals;
    Vertex metadata;
} MeshFace;

typedef struct Mesh{
    PositionMap* vertexMap;
    
    MeshFace* faces;
    size_t faceCount;
    size_t faceSize;

    // Constructed mesh data
    unsigned constructed: 1;
    float* vertices;
    int vertices_size;
    int vertices_count;

    unsigned int* indices;
    int indices_size;
    int indices_count;

    int* vertex_format;
    int format_size;
    int vertex_size;
} Mesh;

Mesh* newMesh3D();
void setVertexFormat(Mesh* mesh, int sizes[], int count);
void destoryMesh(Mesh* mesh);

void addQuadFaceToMesh(Mesh* mesh, Vec3 a, Vec3 b, Vec3 c, Vec3 d, Vec3 normals, Vertex metadata);
void constructMesh(Mesh* mesh);

#include <chunk.h>

#endif