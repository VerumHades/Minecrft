#ifndef MESH_HEADER
#define MESH_HEADER

#include <hashmap.h>
#include <stdlib.h>

#define vertices_size(vertex) vertex->int_value

typedef struct Vertex{
    float values[16];
    int size;
} Vertex;

typedef struct Mesh{
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

void addVertexValueToMesh(Mesh* mesh, float value);
void addIndexValueToMesh(Mesh* mesh, int value);
int getVertexFromMesh(Mesh* mesh, Vertex vertex);
void addQuadFaceToMesh(Mesh* mesh, Vertex vec1, Vertex vec2, Vertex vec3, Vertex vec4);
void addTriangleFaceToMesh(Mesh* mesh, Vertex vec1, Vertex vec2, Vertex vec3);

#endif