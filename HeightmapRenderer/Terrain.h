#pragma once
#include "Heightmap.h"
#include "TerrainMultiTexture.h"
#include "TerrainChunksGenerator.h"
using namespace oglplus;

class Terrain
{
    private:
        // terrain shader uniforms used in the render loop
        Uniform<glm::vec3> lightDirection;
        Uniform<glm::vec3> lightIntensities;
        Uniform<glm::mat4> modelViewProjection;
        Uniform<glm::mat4> modelView;
        Uniform<glm::mat4> normalMatrix;
        Uniform<GLfloat> currentLightmap;
    public:
        TerrainChunksGenerator chunkGenerator;
        bool useLoDChunks = false;
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
        // true once the baking thread finished, until render uploads the data
        bool LightmapsBaked() const { return bakingDone; }
    private:
        void setProgramUniforms(float time);
        // multiplies for current time
        float timeScale;
        // baked lightmaps waiting for the upload, two bytes per texel and
        // time of the day: sun disk visibility and the square root of the
        // bounced sun light
        std::vector<unsigned char> terrainLightmapsData;
        // baked sky light waiting for the upload, one byte per texel
        std::vector<unsigned char> skyLightData;
        // flag to tell the main thread when lightmap baking is done
        // upload data to GPU once done
        std::atomic<bool> bakingDone = false;
        std::atomic<bool> bakingInProgress = false;
        std::atomic<bool> earlyExit = false; // used on exit() to stop thread early
        // if enabled changes directional light color based on time
        bool enableTimeOfTheDayColorGrading = true;
        // thread for baking all time of the day lightmaps
        std::thread bakingThread;
        // path traces lightmapsFrequency times of the day, 24 is one per
        // hour, with the terrain heights scaled by verticalScale texels and
        // the diffuse reflectance per height in albedoByHeight
        // call using bakingThread, this is a heavy operation
        void bakeTimeOfTheDayLightmaps(int lightmapSize, float verticalScale,
                                       std::vector<float> albedoByHeight);
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
        // mesh vertex heights, uploaded with the mesh
        Texture meshHeights;
        void uploadMeshHeights(const std::vector<glm::vec3> &vertices);
        // terrain shadows, generated with heightmap info
        Texture terrainShadowmap;
        // time of the day 3d texture
        Texture terrainTOTDLightmap;
        // sky light, the same at any time of the day
        Texture terrainSkyLight;
        // heightmap generator
        Heightmap heightmap;
        // multitexture handling class
        TerrainMultiTexture terrainTextures;
        void createTOTD3DTexture();
        // uploads size * size * slices lightmaps and size * size sky light
        void uploadBakedLighting(const unsigned char *lightmaps,
                                 const unsigned char *skyLight, int size, int slices);
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
        // mesh vertex heights in [0, 1], one texel per vertex, for the light shafts
        GLuint getMeshHeightsId() { return oglplus::GetName(this->meshHeights); };

        // mesh vertical scaling
        void HeightScale(float val);
        float HeightScale() const { return this->heightScale; };
        // mesh horizontal scaling (scales x and z)
        void TerrainHorizontalScale(float val);
        float TerrainHorizontalScale() const { return this->terrainHorizontalScale; };

        // saves terrain data to a bmp greyscale file
        void saveTerrainToFile(const std::string &filename);

        Terrain();
        ~Terrain();
        // strength of the baked sky light and of the bounced sun light
        void IndirectLight(float skyStrength, float bounceStrength);
};

