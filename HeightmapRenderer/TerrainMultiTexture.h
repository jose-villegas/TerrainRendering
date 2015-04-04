#pragma once
using namespace oglplus;

class TerrainMultiTexture
{
    private:
        static const int MAX_TERRAIN_TEXTURE_RANGES = 4;

        Texture texture;
        std::array<Texture, MAX_TERRAIN_TEXTURE_RANGES> uiTextures;
        Context gl;
        // range
        GLfloat ranges[MAX_TERRAIN_TEXTURE_RANGES * 3];

    public:
        void loadTexture(const std::string &filepath, const int index);
        TerrainMultiTexture();
        ~TerrainMultiTexture();

        void SetTextureRange(const int index, const float start, const float end);
        void SetUniforms(Program &program);
        GLuint UITextureId(const int index);
};

