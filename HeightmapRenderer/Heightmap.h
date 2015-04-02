#pragma once
using namespace noise;

class Heightmap
{
    private:

        float bottomLeft;
        float bottomRight;
        float topLeft;
        float topRigth;

        int width;
        int heigth;

        module::Perlin noiseGen;
        utils::NoiseMap heightmap;
        utils::NoiseMapBuilderPlane heightmapBuilder;
        utils::RendererImage renderer;
        utils::Image image;
        utils::WriterBMP writer;
    public:
        void setBounds(const float bottomLeft, const float topLeft,
                       const float bottomRight, const float topRigth);
        void setSize(const int x, const int y);
        void build();
        void writeToFile(const std::string & filename);
        float getValue(int x, int y);

        // private members getters
        float BottomLeft() const { return bottomLeft; }
        float BottomRight() const { return bottomRight; }
        float TopLeft() const { return topLeft; }
        float TopRigth() const { return topRigth; }
        int Width() const { return width; }
        int Heigth() const { return heigth; }

        Heightmap();
        ~Heightmap();
};

