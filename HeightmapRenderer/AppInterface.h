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
        oglplus::Context gl;
        float textureRepeat[2];
        bool colorGrading;
        float timeScale;
    public:
        void initialize(GLFWwindow * window);
        void draw();
        void render();
        void terminate();

        AppInterface();
        ~AppInterface();
};

