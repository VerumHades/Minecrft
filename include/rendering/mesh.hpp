#ifndef MESH_HEADER
#define MESH_HEADER

#include <cstdlib>
#include <string>
#include <iostream>

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
void destroyMesh(Mesh* mesh);

void addQuadFaceToMesh(Mesh* mesh, Vec3 vertices[4], Vec3 normals, Vertex metadata, int clockwise, int width, int height);
#include <chunk.hpp>

#endif