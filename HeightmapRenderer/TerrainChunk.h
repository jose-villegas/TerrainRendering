#pragma once
class TerrainChunk
{
    private:
        std::array<std::vector<unsigned int>, 3> indices;
    public:
        // chunk num vertices = chunkSizeExponent ^ 2 + 1
        TerrainChunk(unsigned int chunkSizeExponent);
        ~TerrainChunk();
};

