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
    topRigth(5), topLeft(5)
{
    heightmapBuilder.SetSourceModule(noiseGen);
    heightmapBuilder.SetDestNoiseMap(heightmap);
    heightmapBuilder.SetBounds(bottomLeft, topLeft, bottomRight, topRigth);
    renderer.SetSourceNoiseMap(heightmap);
    renderer.SetDestImage(image);
}

Heightmap::~Heightmap()
{
}
