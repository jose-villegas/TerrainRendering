#pragma once
#include "Terrain.h"

class AppInterface
{
    private:
        float maxHeight;
        int meshResolution;
        int heightmapResolution;
        bool wireframeMode;
        oglplus::Context gl;
    public:
        void initialize(GLFWwindow * window);
        void draw();
        void render();
        void terminate();

        AppInterface();
        ~AppInterface();
};

