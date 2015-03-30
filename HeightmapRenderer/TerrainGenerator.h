#pragma once
using namespace noise;

class TerrainGenerator
{
    private:
        static bool setValues;

        float bottomLeft;
        float bottomRight;
        float topLeft;
        float topRigth;

        static int width;
        static int heigth;

        static module::Perlin noiseGen;
        static utils::NoiseMap heightmap;
        static utils::NoiseMapBuilderPlane heightmapBuilder;

    protected:
        float BottomLeft() const { return bottomLeft; }
        float BottomRight() const { return bottomRight; }
        float TopLeft() const { return topLeft; }
        float TopRigth() const { return topRigth; }
        int Width() const { return width; }
        int Heigth() const { return heigth; }

    protected:
        utils::NoiseMap const &Heightmap() const { return heightmap; }

    public:
        void setBounds(const float bottomLeft, const float topLeft,
                       const float bottomRight, const float topRigth);
        static void setSize(const int x, const int y);
        void build();

        TerrainGenerator();
        ~TerrainGenerator();
};

