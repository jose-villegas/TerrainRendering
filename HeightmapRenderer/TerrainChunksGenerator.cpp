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
            auto it = std::max_element(chunkVertices.begin(), chunkVertices.end(),
                                       [](const glm::vec3 & x, const glm::vec3 & y)
            {
                return x.y < y.y;
            });
            this->meshChunks[y].push_back(
                new TerrainChunk(
                    chunkVertices, chunkNormals, chunkTexCoords, &chunkDetail, it->y
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
