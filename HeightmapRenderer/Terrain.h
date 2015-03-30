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


    public:
        Patches terrain;
        void display();
        Terrain();

        void test(int cc);

        ~Terrain();
};

