#pragma once
#include "ChunkDetailLevel.h"
#include "Camera.h"
using namespace oglplus;

class BoundingBox
{
    private:
        // wrapper around the current OpenGL context
        Context gl;
        // Vertex shader
        VertexShader vs;
        // Fragment shader
        FragmentShader fs;
        // Program
        Program prog;

        shapes::Cube bbox;
        shapes::DrawingInstructions bboxInstructions;
        shapes::Cube::IndexArray bboxIndexArray;

        Uniform<glm::mat4> modelMatrix;
        Uniform<glm::mat4> viewMatrix;
        Uniform<glm::mat4> projectionMatrix;

        // VBOs for the cube's vertices
        Buffer verts;
        Buffer indices;

    public:
        BoundingBox();
        void render(glm::vec3 position, glm::vec3 dimensions);
};

class TerrainChunk
{
    private:
        // debug drawing
        static bool debugMode;
        static BoundingBox * chunkBBox;
        // bounding box position and dimension
        glm::vec2 position;
        glm::vec3 dimension;
    private:
        friend class TerrainChunksGenerator;
        // shared detail level control between all chunks
        // chunkdetail level only stores the index combinations
        // of different lod levels
        static ChunkDetailLevel * chunkLod;
        static Context gl;
    private:
        // lod level calculations members
        float distanceToEye;
        // chunk lod level
        int currentLoD;
        // thresshold t_
        static float threeshold;
        // height change between lod levels per chunk
        std::array<float, 2> heightChange;
        // defines the distance for selecting the lod level
        std::array<float, 2> entropyDistances;
    private:
        // chunk center vertex
        glm::vec3 center;
        // mesh data, freed once uploaded to gpu
        std::vector<glm::vec3> vertices;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec2> texCoords;
        // mesh data gpu buffers
        std::array<Buffer, 4> buffer;
        // called on drawElemented
        void bindBuffer(Program &program);
    public:
        TerrainChunk(std::vector<glm::vec3> & vertices,
                     std::vector<glm::vec3> & normals,
                     std::vector<glm::vec2> & texCoords,
                     ChunkDetailLevel * chunkLod,
                     float maxHeight);
        // chunk num vertices = chunkSizeExponent ^ 2 + 1
        ~TerrainChunk() {};
        void bindBufferData(Program &program);
        // calls glDrawElements with the current lod level indices
        void drawElements(Program &program);
        // calculates appropiate lod level
        void chooseLoDLevel(Camera &camera);
        // returns the C constant for geomipmapping
        float getCameraConstant(Camera &camera);
        // generated geometric height changes for geomipmapping (d)
        void generatedEntropies();

};

