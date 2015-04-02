#pragma once
#include "Heightmap.h"
using namespace oglplus;

class TerrainPatch : public Heightmap
{
    private:
        const static int LOD_LEVELS = 5;

        Context gl;
        Buffer indexBuffer[LOD_LEVELS];
        Buffer normalBuffer[LOD_LEVELS];
        Buffer vertexBuffer[LOD_LEVELS];
        Buffer texCoordsBuffer[LOD_LEVELS];

        Program prog;

        bool loaded;
        bool shaderLoaded;
        float maxHeight;
        // describes the sampling density, higher -> more polys
        int patchSize;
        // width and height for heightmap sampling
        int patchArea;

        int lodLevelsAvailable;
        int levelOfDetail;

        // mesh data
        std::vector<glm::vec3> vertices[LOD_LEVELS];
        std::vector<glm::vec2> texCoords[LOD_LEVELS];
        std::vector<glm::vec3> normals[LOD_LEVELS];
        std::vector<unsigned short> indices[LOD_LEVELS];
        void loadLodMesh(int i);
        void writeIndices();
        void writeVertexNormals(std::vector<std::vector<glm::vec3>> faceNormals[2]);
        void writeFaceNormals(std::vector<std::vector<glm::vec3>> faceNormals[2]);
        void writePositionsAndTexCoords();

        // helper function for patch sizes comprobation
        int isPowerOfTwo(unsigned int x)
        {
            return ((x != 0) && ((x & (~x + 1)) == x));
        }
    public:
        TerrainPatch();
        ~TerrainPatch();

        std::vector<glm::vec3> & getVertices(const float height,
                                             const unsigned int patchSize, const unsigned int lodLevel);
        void createPatch(Program &prog, const float height,
                         const unsigned int lodExponent);

        void display();
        //  level range from 1 - 5
        void changeLevelOfDetail(const unsigned int level);
};

