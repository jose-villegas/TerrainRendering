#include "Commons.h"
#include "TerrainChunk.h"

ChunkDetailLevel * TerrainChunk::chunkLod = nullptr;

void TerrainChunk::bindBuffer(Program &program)
{
    // upload position data to the gpu
    buffer[0].Bind(Buffer::Target::Array);
    {
        // vertices
        (program | 0).Setup<GLfloat>(3).Enable();
    }
    buffer[1].Bind(Buffer::Target::Array);
    {
        // normals
        (program | 1).Setup<GLfloat>(3).Enable();
    }
    buffer[2].Bind(Buffer::Target::Array);
    {
        // texture coords
        (program | 2).Setup<GLfloat>(2).Enable();
    }

    if(chunkLod)
    {
        chunkLod->bindBuffer(2);
    }
}

void TerrainChunk::drawElements()
{
    gl.DrawElements(
        PrimitiveType::TriangleStrip,
        chunkLod->indicesSize(2),
        DataType::UnsignedInt
    );
}

TerrainChunk::TerrainChunk(std::vector<glm::vec3> &vertices,
                           std::vector<glm::vec3> &normals, std::vector<glm::vec2> &texCoords)
{
    this->vertices = std::move(vertices);
    this->normals = std::move(normals);
    this->texCoords = std::move(texCoords);
}

void TerrainChunk::bindBufferData(Program &program)
{
    // upload position data to the gpu
    buffer[0].Bind(Buffer::Target::Array);
    {
        Buffer::Data(Buffer::Target::Array, vertices);
        // vertices
        (program | 0).Setup<GLfloat>(3).Enable();
    }
    buffer[1].Bind(Buffer::Target::Array);
    {
        // upload the data
        Buffer::Data(Buffer::Target::Array, normals);
        // normals
        (program | 1).Setup<GLfloat>(3).Enable();
    }
    buffer[2].Bind(Buffer::Target::Array);
    {
        // upload the data
        Buffer::Data(Buffer::Target::Array, texCoords);
        // texture coords
        (program | 2).Setup<GLfloat>(2).Enable();
    }
    // free memory once uploaded to gpu
    this->vertices.clear();
    this->normals.clear();
    this->texCoords.clear();
}
