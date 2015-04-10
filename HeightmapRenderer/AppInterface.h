#pragma once
#include "Terrain.h"

class AppInterface
{
    public:
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
        bool pauseTime;
        int lightmapFreqAndSize[2];
        int terrainSeed;
        bool terrainSeedSet;
        bool useRandom;
        int occlusionStrenght;
        bool geomipmapping;
        float geoThreeshold;
        bool showBBoxes;
        bool frustumCulling;
        void initialize(GLFWwindow * window);
        void draw(float time);
        void render();
        void terminate();

        AppInterface();
        ~AppInterface();
};

