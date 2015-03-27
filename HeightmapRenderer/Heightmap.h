#pragma once
using namespace oglplus;

class Heightmap
{
    private:
        Context gl;
        Buffer indexBuffer, normalBuffer, vertexBuffer, texCoordsBuffer;

        VertexArray terrainMesh;

        VertexShader vs;
        FragmentShader fs;
        Program prog;

        bool loaded;
        bool shaderLoaded;
        int rows;
        int cols;

        // mesh data
        std::vector<glm::vec3> vertices;
        std::vector<glm::vec2> texCoords;
        std::vector<glm::vec3> normals;
        std::vector<unsigned int> indices;
        void loadMeshData();
        void writeIndices();
        void writeVertexNormals(std::vector<std::vector<glm::vec3>> faceNormals[2]);
        void writeFaceNormals(std::vector<std::vector<glm::vec3>> faceNormals[2]);
        void writePositionsAndTexCoords();
    public:
        Heightmap();
        ~Heightmap();

        void loadFromFile(const std::string &srFilename);
        void display(double time);
        void reshape(const unsigned int width, const unsigned int height);
};

