#pragma once
#include "TerrainChunk.h"

class Node
{
    private:
        // shared data between nodes
        static float heightScale;
        static float horizontalScale;
    private:
        std::array<glm::vec3, 4> edges;
        glm::vec3 centerPoint;
        std::vector<TerrainChunk *> chunks;
    public:
        Node *parent;
        std::array<Node *, 4 > children;
        std::array<Node *, 4 > neighbor;
};

class TerrainQuadtree
{
    private:
        std::unique_ptr<Node> root;
    public:
        TerrainQuadtree();
        ~TerrainQuadtree();
};

