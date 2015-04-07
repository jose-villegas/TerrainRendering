#pragma once
#include "Terrain.h"

class AppInterface
{
    private:
        float maxHeight;
        int meshResolution;
        int heightmapResolution;
        bool wireframeMode;
        float terrainScale;
        float ranges[8];
        float terrainRange[3];
        oglplus::Context gl;
        float textureRepeat[2];
        bool colorGrading;
        float timeScale;

    public:
        bool pauseTime;
        int lightmapFreq[2];
        int terrainSeed;
        bool terrainSeedSet;
        bool useRandom;
        void initialize(GLFWwindow * window);
        void draw(float time);
        void render();
        void terminate();

        AppInterface();
        ~AppInterface();
};

