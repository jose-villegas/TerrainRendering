#pragma once
#include "TerrainMultiTexture.h"
class AppInterface
{
    private:
        float maxHeight;
        int meshResolution;
        int heightmapResolution;
        bool wireframeMode;
        TerrainMultiTexture tmtt;
        oglplus::Context gl;
    public:
        void initialize(GLFWwindow * window);
        void draw();
        void render();
        void terminate();

        AppInterface();
        ~AppInterface();
};

