#include "Commons.h"
#include "ChunkDetailLevel.h"
// pixel error, shared among all chunks
float ChunkDetailLevel::threeshold = 0.35f;

int ChunkDetailLevel::combinationKey(LodLevel level, const SideLevels &sides)
{
    // a chunk only stitches to coarser neighbours, finer ones stitch to it
    int key = level;

    for(int side = 0; side < SideCount; side++)
    {
        key = key * AvailableLevels + std::max(level, sides[side]);
    }

    return key;
}

std::vector<unsigned int> ChunkDetailLevel::stripIndices(LodLevel level,
        const SideLevels &sides) const
{
    int step = 1 << level;
    int last = chunkSize - 1;
    int cells = last / step;
    // rounds a border coordinate to the nearest vertex of a coarser neighbour,
    // monotonic so the border triangles only collapse, never fold over
    auto snap = [](int coordinate, LodLevel neighbour)
    {
        int spacing = 1 << neighbour;
        return (coordinate + spacing / 2) / spacing * spacing;
    };
    auto vertexIndex = [&](int row, int col)
    {
        int snappedRow = row, snappedCol = col;

        if(row == 0) snappedCol = snap(col, std::max(level, sides[Top]));
        else if(row == last) snappedCol = snap(col, std::max(level, sides[Down]));

        if(col == 0) snappedRow = snap(row, std::max(level, sides[Left]));
        else if(col == last) snappedRow = snap(row, std::max(level, sides[Right]));

        return (unsigned int)(snappedRow * chunkSize + snappedCol);
    };
    std::vector<unsigned int> strip;

    for(int i = 0; i < cells; i++)
    {
        for(int j = 0; j <= cells; j++)
        {
            strip.push_back(vertexIndex((i + 1) * step, j * step));
            strip.push_back(vertexIndex(i * step, j * step));
        }

        strip.push_back(restartIndexToken);
    }

    return strip;
}

void ChunkDetailLevel::generateDetailLevels(int meshSize, int chunkSize)
{
    this->meshSize = meshSize;
    this->chunkSize = chunkSize;
    restartIndexToken = meshSize * meshSize;
    indices.clear();

    for(int lodLevel = 0; lodLevel < AvailableLevels; lodLevel++)
    {
        LodLevel level = LodLevel(lodLevel);
        // every combination of equal or coarser levels on the four sides
        int combinations = 1;

        for(int side = 0; side < SideCount; side++) combinations *= AvailableLevels - lodLevel;

        for(int c = 0; c < combinations; c++)
        {
            SideLevels sides;

            for(int side = 0, rest = c; side < SideCount; side++)
            {
                sides[side] = LodLevel(lodLevel + rest % (AvailableLevels - lodLevel));
                rest /= AvailableLevels - lodLevel;
            }

            indices[combinationKey(level, sides)] = stripIndices(level, sides);
        }
    }

    indicesCombinationGenerated = true;
}

void ChunkDetailLevel::bindBufferData()
{
    indicesBuffer.clear();
    indexSizes.clear();

    for(auto &combination : indices)
    {
        Buffer &buffer = indicesBuffer[combination.first];
        buffer.Bind(Buffer::Target::ElementArray);
        {
            Buffer::Data(Buffer::Target::ElementArray, combination.second);
        }
        indexSizes[combination.first] = (int)combination.second.size();
    }

    // we don't need the indices once uploaded to gpu
    indices.clear();
    gl.Enable(Capability::PrimitiveRestart);
    gl.PrimitiveRestartIndex(restartIndexToken);
}

void ChunkDetailLevel::bindBuffer(LodLevel level, const SideLevels &sides)
{
    indicesBuffer.at(combinationKey(level, sides)).Bind(Buffer::Target::ElementArray);
}

int ChunkDetailLevel::indicesSize(LodLevel level, const SideLevels &sides)
{
    return indexSizes.at(combinationKey(level, sides));
}

ChunkDetailLevel::ChunkDetailLevel()
{
}

ChunkDetailLevel::~ChunkDetailLevel()
{
}
