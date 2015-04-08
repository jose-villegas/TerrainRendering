#pragma once
#include "ChunkDetailLevel.h"
using namespace oglplus;

class TerrainChunk
{
    private:
        friend class TerrainChunksGenerator;
        // shared detail level control between all chunks
        // chunkdetail level only stores the index combinations
        // of different lod levels
        static ChunkDetailLevel *chunkLod;
        static Context gl;
    private:
        // chunk lod level
        int currentLoD;
        // mesh data, freed once uploaded to gpu
        std::vector<glm::vec3> vertices;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec2> texCoords;
        // mesh data gpu buffers
        std::array<Buffer, 4> buffer;
    public:
        TerrainChunk() {};
        TerrainChunk(std::vector<glm::vec3> &vertices,
                     std::vector<glm::vec3> &normals,
                     std::vector<glm::vec2> &texCoords);
        // chunk num vertices = chunkSizeExponent ^ 2 + 1
        ~TerrainChunk() {};
        void bindBufferData(Program &program);
        void bindBuffer(Program &program);
        // calls glDrawElements with the current lod level indices
        void drawElements();
};

