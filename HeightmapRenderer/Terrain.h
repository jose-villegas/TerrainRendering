#pragma once
#include "Heightmap.h"
#include "TerrainPatch.h"
using namespace oglplus;

class Terrain
{
    private:
        bool heightmapCreated;
        bool meshCreated;
        float terrainMaxHeight;
        // mesh data gpu buffers
        std::array<Buffer, 4> buffer;
        // mesh data collections
        std::vector<glm::vec3> vertices;
        std::vector<glm::vec2> texCoords;
        std::vector<glm::vec3> normals;
        std::vector<unsigned int> indices;
        // mesh general data
        int terrainSize;
        float maxHeight;
        float minHeight;
        // utilities
        VertexArray terrainMesh;
        FragmentShader fragmentShader;
        VertexShader vertexShader;
        Program program;
        Context gl;
        // heightmap generator
        Heightmap heightmap;
    public:

        void initialize();
        void display();
        void bindBuffers();
        // creates a terrain of 2^sizeExponent + 1 size
        void createTerrain(int sizeExponent);
        void createMesh();

        Terrain();
        ~Terrain();
};

