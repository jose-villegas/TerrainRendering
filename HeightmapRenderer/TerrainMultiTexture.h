#pragma once
using namespace oglplus;
class TerrainMultiTexture
{
    private:
        Texture texture;
        Texture uiTextures[4];
        Context gl;
        // range
        float rangeStart;
        float rangeEnd;

    public:
        void loadTexture(const std::string &filepath, const int index);
        TerrainMultiTexture();
        ~TerrainMultiTexture();

        GLuint TexId() { return GetGLName(texture); }
        GLuint TexId(int index) { return GetGLName(uiTextures[index]); }
};

