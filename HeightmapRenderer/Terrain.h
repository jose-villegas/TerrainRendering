#pragma once
#include "Heightmap.h"
#include "TerrainMultiTexture.h"
using namespace oglplus;

class Terrain
{
    public:
        // terrain light direction on time
        glm::vec3 calculateLightDir(float time);
        // terrain light direction and color on time
        void calculateLightDir(float time, glm::vec3 &outDir, glm::vec3 &outColor);
        // mesh height at position
        float heightAt(glm::vec2 position);
    private:
        // multiplies for current time
        float timeScale;
        // temporal baked ligthmaps data, deleted once
        // baking is done
        unsigned char * terrainLightmapsData;
        // flag to tell the main thread when lightmap baking is done
        // upload data to GPU once done
        std::atomic<bool> bakingDone = false;
        std::atomic<bool> bakingInProgress = false;
        std::atomic<bool> earlyExit = false; // used on exit() to stop thread early
        // if enabled changes directional light color based on time
        bool enableTimeOfTheDayColorGrading = true;
        // thread for baking all time of the day lightmaps
        std::thread bakingThread;
        // freq represents the number of sampler per day
        // for example 24 == 1 shadowmap per hour
        // call using bakingThread, this is a heavy operation
        void bakeTimeOfTheDayShadowmap();
    private:
        bool heightmapCreated;
        bool meshCreated;
        float terrainMaxHeight;
        unsigned int indexSize;
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
        float heightScale;
        float terrainHorizontalScale;
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
        void createTOTD3DTexture();
    public:
        void initialize();
        void display(float time);
        void bindBuffers();

        // lightmap as output, uses terrain resolution for lightmap size
        void fastGenerateShadowmapParallel(
            glm::vec3 lightDir,
            std::vector<unsigned char> &lightmap
        );
        // lightmap as out with custom lightmap size
        void fastGenerateShadowmapParallel(
            glm::vec3 lightDir,
            std::vector<unsigned char> &lightmap,
            unsigned int lightmapSize
        );
        // writes lightmap to extra lightmap texture, clears vector data
        void fastGenerateShadowmapParallel(
            glm::vec3 lightDir,
            unsigned int lightmapSize
        );

        // creates a terrain of 2^sizeExponent + 1 size
        void createTerrain(const int heightmapSize);
        void createMesh(const int meshResExponent);
        void bakeLightmaps(float freq);

        // getters
        Heightmap &Heightmap() { return heightmap; }
        Program &Program() { return program; }

        // terrain multi texture control functions
        void setTextureRepeatFrequency(const glm::vec2 &value);
        void setTextureRange(const int index, const float start, const float end);
        void loadTexture(const int index, const std::string &filepath);

        // time parameters
        void EnableTimeOfTheDayColorGrading(bool val) { enableTimeOfTheDayColorGrading = val; }
        void TimeScale(float val) { timeScale = val; }

        // returns terrain multitexture at index
        GLuint getTextureId(int index);
        // returns terrain extra lightmap, created with fastGenerateShadowmapParallel
        GLuint getLightmapId() { return oglplus::GetName(this->terrainShadowmap); };

        // mesh vertical scaling
        void HeightScale(float val);
        // mesh horizontal scaling (scales x and z)
        void TerrainHorizontalScale(float val);

        Terrain();
        ~Terrain();
};

