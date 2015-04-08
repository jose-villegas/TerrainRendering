#pragma once
#include "Heightmap.h"
#include "TerrainMultiTexture.h"
#include "TerrainChunksGenerator.h"
using namespace oglplus;

class Terrain
{
    public:
        TerrainChunksGenerator chunkGenerator;
        // represents the amount of time on daylight
        const float sunTime = 0.6f;
        // scales moon height and nightlight
        const float moonAltitude = 1.3f;
        // scales sun height at daylight
        const float sunAltitude = 1.9f;
        // terrain light direction on time
        glm::vec3 calculateLightDir(float time);
        // terrain light direction and color on time
        void calculateLightDir(float time, glm::vec3 &outDir, glm::vec3 &outColor);
        // mesh height at position
        float heightAt(glm::vec2 position);
    private:
        void setProgramUniforms(float time);
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
        void bakeTimeOfTheDayShadowmap(int lightmapSize);
    private:
        // terrain status indicator
        bool defaultLightmapsBaked = false;
        bool heightmapCreated;
        bool meshCreated;
        // final index size
        unsigned int indexSize;
        // mesh data gpu buffers
        std::array<Buffer, 4> buffer;
        // mesh general data
        int meshResolution;
        int terrainResolution;
        int lightmapResolution;
        glm::vec3 meshSampleSquare;
        // meshSize * meshSize = vertex count
        int meshSize;
        float heightScale;
        float terrainHorizontalScale;
        int lightmapsFrequency;
        int terrainSeed;
        // utilities
        VertexArray terrainMesh;
        FragmentShader fragmentShader;
        VertexShader vertexShader;
        Program program;
        Context gl;
        // heightmap field texture
        Texture heightmapField;
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
        void render(float time);
        // binds the terrain buffers, whole mesh data
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
        void createTerrain(const int heightmapSize, const glm::vec3 sampleSquare,
                           int seed);
        void createMesh(const int meshResExponent);
        void bakeLightmaps(float freq, int lightmapSize);

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

        // saves terrain data to a bmp greyscale file
        void saveTerrainToFile(const std::string &filename);

        Terrain();
        ~Terrain();
        void Occlusion(float occlusionStrenght);
};

