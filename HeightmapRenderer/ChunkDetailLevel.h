#pragma once
using namespace oglplus;

class ChunkDetailLevel
{
        // level of detail containers
    private:
        int restartIndexToken;
        std::array<std::vector<unsigned int>, 3> indicesLoD;
    public:
        void generateDetailLevels(int meshSize, int chunkSize);
        // class details
    private:
        Context gl;
        std::array<int, 3> indexSizes;
        // avoid creating indices again if mesh has the same configuration
        bool indicesCombinationGenerated;
        // indices combinations for different lod levels
        std::array<Buffer, 3> indicesBuffer;
    public:
        void bindBufferData();
        void bindBuffer(int levelOfDetail);
        int indicesSize(int levelOfDetail);

        ChunkDetailLevel();
        ~ChunkDetailLevel();
};

