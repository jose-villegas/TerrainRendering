#pragma once
#include "Commons.h"
using namespace oglplus;

struct Vertex
{
    glm::vec3 position;
    glm::vec2 uv;
    glm::vec3 normal;
    Vertex(glm::vec3 pos, glm::vec2 uv, glm::vec3 norm)
    {
        this->position = pos;
        this->uv = uv;
        this->normal = norm;
    }
};

class Heightmap
{
    private:
        Context gl;
        Buffer vertexBuffer;
        Buffer indexBuffer;

        VertexShader vs;
        FragmentShader fs;
        Program prog;

        std::vector<int> indices;

        bool loaded;
        bool shaderLoaded;
        unsigned int rows;
        unsigned int cols;

    public:
        Heightmap();
        ~Heightmap();

        bool loadFromFile(const std::string &srFilename);
        void display(int width, int height);
};

