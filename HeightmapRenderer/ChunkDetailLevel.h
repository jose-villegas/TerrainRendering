#pragma once
using namespace oglplus;

class ChunkDetailLevel
{
    private:
        // chunk params
        int meshSize;
        int chunkSize;
    public:
        enum LodLevel
        {
            High = 0,
            Medium,
            Low,
            AvailableLevels
        };
        // chunk borders, top is the first vertex row and left the first column
        enum Side
        {
            Top = 0,
            Left,
            Down,
            Right,
            SideCount
        };
        typedef std::array<LodLevel, SideCount> SideLevels;
    private:
        // triangle strip primitive restart at
        int restartIndexToken;
        // index combinations per lod level and neighbour lod levels, the border
        // vertices snap to the coarser neighbour spacing so chunks stay watertight
        // vectors are cleared once data is uploaded to GPU
        std::unordered_map<int, std::vector<unsigned int>> indices;
        std::vector<unsigned int> stripIndices(LodLevel level, const SideLevels &sides) const;
        static int combinationKey(LodLevel level, const SideLevels &sides);
    private:
        Context gl;
        // indices count per combination
        std::unordered_map<int, int> indexSizes;
        // avoid creating indices again if mesh has the same configuration
        bool indicesCombinationGenerated;
        // indices combinations for different lod levels
        std::unordered_map<int, Buffer> indicesBuffer;
        // thresshold t_
        static float threeshold;
    public:
        // uploads index data to gpu
        void bindBufferData();
        // binds the indices for a chunk at level with neighbours at sides
        void bindBuffer(LodLevel level, const SideLevels &sides);
        // indices count for a chunk at level with neighbours at sides
        int indicesSize(LodLevel level, const SideLevels &sides);
        // generates the LoD indices configurations based on mesh and chunk size
        void generateDetailLevels(int meshSize, int chunkSize);
        // token to restart the triangle strip
        int RestartIndexToken() const { return restartIndexToken; }

        ChunkDetailLevel();
        ~ChunkDetailLevel();
        // chunks size
        int ChunkSize() const { return chunkSize; }
        int MeshSize() const { return meshSize; }
        // sets and getter for pixel threeshold among chunks
        static float Threeshold() { return threeshold; }
        static void Threeshold(float val) { threeshold = val; }
};

