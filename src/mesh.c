#include <mesh.h>

Mesh* newMesh3D(){
    Mesh* mesh = calloc(1,sizeof(Mesh));
    
    mesh->vertices = calloc(1000,sizeof(float));
    mesh->vertices_count = 0;
    mesh->vertices_size = 1000;

    mesh->indices = calloc(1000,sizeof(float));
    mesh->indices_count = 0;
    mesh->indices_size = 1000;

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

void requireVerticesSize(Mesh* mesh, size_t size){
    while(mesh->vertices_size <= mesh->vertices_count+size){ // Real array is too small, double the size
        mesh->vertices_size  *= 2;
        //printf("Resized vertices array");
        mesh->vertices = realloc(mesh->vertices, mesh->vertices_size * sizeof(float));
    }
}

void addIndexValueToMesh(Mesh* mesh, int value){
    if(mesh->indices_size <= mesh->indices_count+1){ // Real array is too small, double the size
        mesh->indices_size  *= 2;
        mesh->indices = realloc(mesh->indices, mesh->indices_size * sizeof(unsigned int));
    }

    mesh->indices[mesh->indices_count] = value;
    mesh->indices_count++;
}

// Return vertex index
int getVertexFromMesh(Mesh* mesh, Vertex vertex){
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

void addQuadFaceToMesh(Mesh* mesh, Vertex vec1, Vertex vec2, Vertex vec3, Vertex vec4){
    int v1_index = getVertexFromMesh(mesh, vec1);
    int v2_index = getVertexFromMesh(mesh, vec2);
    int v3_index = getVertexFromMesh(mesh, vec3);
    int v4_index = getVertexFromMesh(mesh, vec4);

    /*
        1---2
        | / |
        4---3

        Two triangles clockwise
    */
    addIndexValueToMesh(mesh, v1_index);
    addIndexValueToMesh(mesh, v2_index);
    addIndexValueToMesh(mesh, v4_index);

    addIndexValueToMesh(mesh, v2_index);
    addIndexValueToMesh(mesh, v3_index);
    addIndexValueToMesh(mesh, v4_index);
}

void addTriangleFaceToMesh(Mesh* mesh, Vertex vec1, Vertex vec2, Vertex vec3){
    int v1_index = getVertexFromMesh(mesh, vec1);
    int v2_index = getVertexFromMesh(mesh, vec2);
    int v3_index = getVertexFromMesh(mesh, vec3);

    /*
        1---2
        | / 
        3

        Triangle clockwise
    */
    addIndexValueToMesh(mesh, v1_index);
    addIndexValueToMesh(mesh, v2_index);
    addIndexValueToMesh(mesh, v3_index);
}