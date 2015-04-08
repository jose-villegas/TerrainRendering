#pragma once
#include "TerrainChunk.h"
using namespace oglplus;

class TerrainChunksGenerator
{
    private:
        Context gl;
    private:
        bool chunksGenerated = false;
        // helper for getting x, y from contiguos vectors
        glm::vec3 &getVertex(int x, int y);
        glm::vec2 &getTexCoord(int x, int y);
        glm::vec3 &getNormal(int x, int y);
        // mesh parameters data
        unsigned int chunkSize;
        unsigned int meshSize;
        unsigned int meshSizeExponent;
        unsigned int chunkSizeExponent;
        unsigned int restartIndexToken;
        unsigned int chunkCount;
        // whole mesh data
        std::vector<glm::vec3> vertices;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec2> texCoords;
        // collection of all mesh chunks
        std::vector<std::vector<TerrainChunk *>> meshChunks;
        // controller for chunk detail level
        ChunkDetailLevel chunkDetail;
        // deletes all mesh chunks
        void deleteMeshChunks();
    public:
        // generates all terrain chunks
        void generateChunks(std::vector<glm::vec3> &meshVertices,
                            std::vector<glm::vec3> &meshNormals,
                            std::vector<glm::vec2> &meshTexCoords,
                            unsigned int meshSizeExponent,
                            unsigned int chunkSizeExponent);
        // uploads all the chunks buffer objects to the gpu
        void bindBufferData(Program &program);
        // generated chunks
        TerrainChunk &MeshChunk(int x, int y) { return *meshChunks[x][y]; }
        unsigned int ChunkCount() const { return chunkCount; }

        TerrainChunksGenerator() {};
        ~TerrainChunksGenerator();
};

