#pragma once
#include "Heightmap.h"
#include "TerrainMultiTexture.h"
using namespace oglplus;

class Terrain
{
    private:
        unsigned char * terrainLightmapsData;
        bool bakingDone = false;
        std::thread bakingThread;
        // freq represents the number of sampler per day
        // for example 24 == 1 shadowmap per hour
        void bakeTimeOfTheDayShadowmap(float freq);
        glm::vec3 calculateLightDir(float time);
        void calculateLightDir(float time, glm::vec3 &outDir, glm::vec3 &outColor);
    private:
        bool heightmapCreated;
        bool meshCreated;
        float terrainMaxHeight;
        // mesh data gpu buffers
        std::array<Buffer, 4> buffer;
        // mesh data collections
        std::vector<glm::vec3> vertices;
        std::vector<glm::vec2> texCoords;
        std::vector<glm::vec3> normals;
        std::vector<unsigned int> indices;
        // mesh general data
        int meshResolution;
        int terrainResolution;
        // meshSize * meshSize = vertex count
        int meshSize;
        float maxHeight;
        float minHeight;
        int lightmapsFrequency;
        // utilities
        VertexArray terrainMesh;
        FragmentShader fragmentShader;
        VertexShader vertexShader;
        Program program;
        Context gl;
        // terrain shadows, generated with heightmap info
        Texture terrainShadowmap;
        // time of the day 3d texture
        Texture terrainTOTDLightmap;
        // heightmap generator
        Heightmap heightmap;
        // multitexture handling class
        TerrainMultiTexture terrainTextures;

    public:
        void initialize();
        void display();
        void bindBuffers();
        // lightmap as output
        void fastGenerateShadowmapParallel(
            glm::vec3 lightDir,
            std::vector<unsigned char> &lightmap
        );
        // writes lightmap to texture, clears vector data
        void fastGenerateShadowmapParallel(
            glm::vec3 lightDir
        );

        // creates a terrain of 2^sizeExponent + 1 size
        void createTerrain(const int heightmapSize);
        void createMesh(const int meshResExponent);

        // important getters
        Heightmap &Heightmap() { return heightmap; }
        Program &Program() { return program; }

        void setTextureRepeatFrequency(const glm::vec2 &value);
        void setTextureRange(const int index, const float start, const float end);
        void loadTexture(const int index, const std::string &filepath);
        GLuint getTextureId(int index);
        GLuint getLightmapId() { return oglplus::GetName(this->terrainShadowmap); };

        Terrain();
        ~Terrain();
};

