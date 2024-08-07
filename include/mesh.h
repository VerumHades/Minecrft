#ifndef MESH_HEADER
#define MESH_HEADER

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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

void addQuadFaceToMesh(Mesh* mesh, Vertex vec1, Vertex vec2, Vertex vec3, Vertex vec4);
void addTriangleFaceToMesh(Mesh* mesh, Vertex vec1, Vertex vec2, Vertex vec3);

#endif