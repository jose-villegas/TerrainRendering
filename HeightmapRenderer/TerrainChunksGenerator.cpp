#include "Commons.h"
#include "TerrainChunksGenerator.h"
#include "ChunkDetailLevel.h"

glm::vec3 & TerrainChunksGenerator::getVertex(int x, int y)
{
    return vertices[y * meshSize + x];
}

glm::vec2 & TerrainChunksGenerator::getTexCoord(int x, int y)
{
    return texCoords[y * meshSize + x];
}

glm::vec3 & TerrainChunksGenerator::getNormal(int x, int y)
{
    return normals[y * meshSize + x];
}

void TerrainChunksGenerator::generateChunks(std::vector<glm::vec3>
        &meshVertices, std::vector<glm::vec3> &meshNormals,
        std::vector<glm::vec2> &meshTexCoords, unsigned int meshSizeExponent,
        unsigned int chunkSizeExponent)
{
    this->vertices = std::move(meshVertices);
    this->normals = std::move(meshNormals);
    this->texCoords = std::move(meshTexCoords);
    // set mesh params
    this->meshSizeExponent = meshSizeExponent;
    this->chunkSizeExponent = chunkSizeExponent;
    this->meshSize = std::pow(2, meshSizeExponent) + 1;
    this->chunkSize = std::pow(2, chunkSizeExponent) + 1;
    this->restartIndexToken = meshSize * meshSize;
    // calculate chunk count
    chunkCount = (this->meshSize - 1) / (this->chunkSize - 1);
    // delete previous chunks and reserve memory for new ones
    deleteMeshChunks();
    this->meshChunks.resize(chunkCount);
    // create lod controller levels
    chunkDetail.generateDetailLevels(meshSize, chunkSize);

    for(int y = 0; y < chunkCount; y++)
    {
        for(int x = 0; x < chunkCount; x++)
        {
            std::vector<glm::vec3> chunkVertices;
            std::vector<glm::vec3> chunkNormals;
            std::vector<glm::vec2> chunkTexCoords;

            for(int i = 0; i < chunkSize; i++)
            {
                for(int j = 0; j < chunkSize; j++)
                {
                    int xCoord = j + x * (chunkSize - 1);
                    int yCoord = i + y * (chunkSize - 1);
                    chunkVertices.push_back(getVertex(xCoord, yCoord));
                    chunkNormals.push_back(getNormal(xCoord, yCoord));
                    chunkTexCoords.push_back(getTexCoord(xCoord, yCoord));
                }
            }

            // get maximim height for current chunk
            float chunkMaxHeight = 0.0f;
            float chunkMinHeight = 1.0f;

            for each(glm::vec3 vertex in chunkVertices)
            {
                chunkMaxHeight = vertex.y > chunkMaxHeight ? vertex.y : chunkMaxHeight;
                chunkMinHeight = vertex.y < chunkMinHeight ? vertex.y : chunkMinHeight;
            }

            this->meshChunks[y].push_back(
                new TerrainChunk(
                    chunkVertices, chunkNormals, chunkTexCoords,
                    &chunkDetail, chunkMaxHeight, chunkMinHeight
                )
            );
        }
    }

    chunkDetail.bindBufferData();
    // we don't need these collections anymore
    this->vertices.clear();
    this->texCoords.clear();
    this->normals.clear();
}

void TerrainChunksGenerator::bindBufferData(Program &program)
{
    for each(std::vector<TerrainChunk *> hLineChunks in this->meshChunks)
    {
        for(unsigned int i = 0; i < hLineChunks.size(); i++)
        {
            hLineChunks[i]->bindBufferData(program);
        }
    }
}

void TerrainChunksGenerator::drawChunks(Program &program)
{
    for(auto &hLineChunks : this->meshChunks)
    {
        for(TerrainChunk *chunk : hLineChunks)
        {
            chunk->updateLoD();
        }
    }

    int count = (int)chunkCount;

    for(int y = 0; y < count; y++)
    {
        for(int x = 0; x < count; x++)
        {
            TerrainChunk &chunk = *meshChunks[y][x];
            // the first vertex row borders the previous chunk row, chunks on
            // the mesh border keep their own level on that side
            auto levelAt = [&](int nx, int ny)
            {
                return nx < 0 || ny < 0 || nx >= count || ny >= count
                       ? chunk.LoD() : meshChunks[ny][nx]->LoD();
            };
            ChunkDetailLevel::SideLevels neighbours;
            neighbours[ChunkDetailLevel::Top] = levelAt(x, y - 1);
            neighbours[ChunkDetailLevel::Left] = levelAt(x - 1, y);
            neighbours[ChunkDetailLevel::Down] = levelAt(x, y + 1);
            neighbours[ChunkDetailLevel::Right] = levelAt(x + 1, y);

            // bounding boxes switch the program, restore it before drawing
            if(TerrainChunk::DrawingBoundingBoxes()) program.Use();

            chunk.drawElements(program, neighbours);
        }
    }
}

TerrainChunksGenerator::~TerrainChunksGenerator()
{
    // deallocate chunks
    deleteMeshChunks();
}

void TerrainChunksGenerator::deleteMeshChunks()
{
    for each(std::vector<TerrainChunk *> hLineChunks in this->meshChunks)
    {
        for each(TerrainChunk * chunkPtr in hLineChunks)
        {
            delete chunkPtr;
        }

        hLineChunks.clear();
    }

    meshChunks.clear();
}
