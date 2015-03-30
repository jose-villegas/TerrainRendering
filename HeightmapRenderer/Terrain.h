#pragma once
#include "TerrainPatch.h"

class Terrain
{
    private:
        typedef std::unordered_map<int, TerrainPatch> TerrainLocations;
        typedef std::unordered_map<int, TerrainLocations> Patches;

        float terrainMaxHeight;

        VertexArray terrainMesh;

        FragmentShader fragmentShader;
        VertexShader vertexShader;
        Program program;
        Context gl;

        Patches terrain;
    public:

        void display();
        Terrain();
        ~Terrain();
};

