#include "Commons.h"
#include "Heightmap.h"

int counter = 0;

void Heightmap::setBounds(const float bottomLeft, const float topLeft,
                          const float bottomRight, const float topRigth)
{
    this->bottomLeft = bottomLeft;
    this->topLeft = topLeft;
    this->bottomRight = bottomRight;
    this->topRigth = topRigth;
    heightmapBuilder.SetBounds(bottomLeft, topLeft, bottomRight, topRigth);
}

void Heightmap::setSeed(int seed)
{
    this->terrainType.SetSeed(seed);
    this->baseFlatTerrain.SetSeed(seed);
    this->baseMountainTerrain.SetSeed(seed);
    this->finalTerrain.SetSeed(seed);
    this->baseWaterZones.SetSeed(seed);
}

void Heightmap::setSize(const int x, const int y)
{
    width = x;
    heigth = y;
    heightmapBuilder.SetDestSize(width, heigth);
}

void Heightmap::build()
{
    heightmapBuilder.Build();
}

void Heightmap::writeToFile(const std::string & filename)
{
    if(width <= 0 || heigth <= 0) return;

    renderer.Render();
    utils::WriterBMP writer;
    writer.SetSourceImage(image);
    writer.SetDestFilename(filename + ".bmp");
    writer.WriteDestFile();
}

float Heightmap::getValue(int x, int y)
{
    return heightmap.GetValue(x, y);
}

Heightmap::Heightmap() : bottomLeft(0), bottomRight(0),
    topRigth(5), topLeft(5), width(0), heigth(0)
{
    // mountains
    baseMountainTerrain.SetFrequency(0.65);
    mountainTerrain.SetSourceModule(0, baseMountainTerrain);
    mountainTerrain.SetScale(0.80);
    // midlands flat terrain
    baseFlatTerrain.SetFrequency(1.75);
    flatTerrain.SetSourceModule(0, baseFlatTerrain);
    flatTerrain.SetScale(0.075);
    flatTerrain.SetBias(-0.65);
    // water zones
    baseWaterZones.SetFrequency(0.75);
    baseWaterZones.SetPersistence(0.45);
    invertBase.SetSourceModule(0, baseWaterZones);
    waterZones.SetSourceModule(0, invertBase);
    waterZones.SetScale(1.65);
    waterZones.SetBias(-3.65f);
    // terrain boundaries
    terrainType.SetFrequency(0.5);
    terrainType.SetPersistence(0.25);
    // apply water zones to flatlands
    multiplierBase.SetSourceModule(0, flatTerrain);
    multiplierBase.SetSourceModule(1, waterZones);
    flatlandsAndWater.SetSourceModule(0, multiplierBase);
    flatlandsAndWater.SetScale(0.25);
    flatlandsAndWater.SetBias(-0.75);
    // join  modules, flat and mountains
    terrainSelector.SetSourceModule(0, flatlandsAndWater);
    terrainSelector.SetSourceModule(1, mountainTerrain);
    terrainSelector.SetControlModule(terrainType);
    terrainSelector.SetBounds(-0.15, 35.0);
    terrainSelector.SetEdgeFalloff(0.35);
    // turbulence
    finalTerrain.SetSourceModule(0, terrainSelector);
    finalTerrain.SetFrequency(4.0);
    finalTerrain.SetPower(0.125);
    // finally set source for builder
    heightmapBuilder.SetSourceModule(terrainSelector);
    heightmapBuilder.SetDestNoiseMap(heightmap);
    heightmapBuilder.SetBounds(bottomLeft, topLeft, bottomRight, topRigth);
    renderer.SetSourceNoiseMap(heightmap);
    renderer.SetDestImage(image);
}

Heightmap::~Heightmap()
{
}
