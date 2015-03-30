#include "Commons.h"
#include "TerrainGenerator.h"

bool TerrainGenerator::setValues = false;
int TerrainGenerator::width = 256;
int TerrainGenerator::heigth = 256;

module::Perlin TerrainGenerator::noiseGen;
utils::NoiseMap TerrainGenerator::heightmap;
utils::NoiseMapBuilderPlane TerrainGenerator::heightmapBuilder;

void TerrainGenerator::setBounds(const float bottomLeft, const float topLeft,
                                 const float bottomRight, const float topRigth)
{
    this->bottomLeft = bottomLeft;
    this->topLeft = topLeft;
    this->bottomRight = bottomRight;
    this->topRigth = topRigth;
}

void TerrainGenerator::setSize(const int x, const int y)
{
    width = x;
    heigth = y;
    heightmapBuilder.SetDestSize(width, heigth);
}

void TerrainGenerator::build()
{
    heightmapBuilder.SetBounds(bottomLeft, topLeft, bottomRight, topRigth);
    heightmapBuilder.Build();
}

TerrainGenerator::TerrainGenerator() : bottomLeft(0), bottomRight(0),
    topRigth(1), topLeft(1)
{
    if(!setValues)
    {
        heightmapBuilder.SetSourceModule(noiseGen);
        heightmapBuilder.SetDestNoiseMap(heightmap);
        setValues = true;
    }
}

TerrainGenerator::~TerrainGenerator()
{
}
