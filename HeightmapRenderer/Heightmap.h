#pragma once
#include "Commons.h"
using namespace oglplus;

class Heightmap
{
    private:
        Buffer vao;
        bool loaded;
        bool shaderLoaded;
        unsigned int rows;
        unsigned int cols;

    public:
        Heightmap();
        ~Heightmap();
};

