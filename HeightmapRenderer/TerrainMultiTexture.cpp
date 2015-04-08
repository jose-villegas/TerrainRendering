#include "Commons.h"
#include "TerrainMultiTexture.h"


void TerrainMultiTexture::loadTexture(const std::string &filepath,
                                      const int index)
{
    if(index < 0 || index >= MAX_TERRAIN_TEXTURE_RANGES) return;

    FREE_IMAGE_FORMAT fif = FreeImage_GetFileType(filepath.c_str());

    //if still unknown, try to guess the file format from the file extension
    if(fif == FIF_UNKNOWN)
    {
        fif = FreeImage_GetFIFFromFilename(filepath.c_str());
    }

    //if still unkown, exit
    if(fif == FIF_UNKNOWN)
    {
        return;
    }

    // pointer to the image once loaded
    FIBITMAP *dib = 0;

    //check that the plugin has reading capabilities and load the file
    if(FreeImage_FIFSupportsReading(fif))
    {
        dib = FreeImage_Load(fif, filepath.c_str());
    }

    // highest quality rescaleing
    dib = FreeImage_Rescale(dib, 512, 512, FILTER_BICUBIC);
    // no alpha channel for terrain textures
    dib = FreeImage_ConvertTo24Bits(dib);
    // dib = FreeImage_ConvertTo32Bits(dib);
    // get raw data
    unsigned char * bits = FreeImage_GetBits(dib);
    // get image data
    unsigned int width = FreeImage_GetWidth(dib);
    unsigned int height = FreeImage_GetHeight(dib);
    unsigned int bitsPerPixel = FreeImage_GetBPP(dib);

    // If this somehow one of these failed (they shouldn't), return failure
    if((bitsPerPixel == 0) || (height == 0) || (width == 0))
    {
        FreeImage_Unload(dib);
        return;
    }

    // pass data to the texture array
    gl.Bound(Texture::Target::_2DArray, this->texture)
    .MinFilter(TextureMinFilter::LinearMipmapLinear)
    .MagFilter(TextureMagFilter::Linear)
    .GenerateMipmap()
    .WrapS(TextureWrap::Repeat)
    .WrapT(TextureWrap::Repeat)
    .Anisotropy(16.0f)
    .SubImage3D(0, 0, 0, index, 512, 512, 1, PixelDataFormat::BGR,
                PixelDataType::UnsignedByte, bits);
    // pass data to interface texture
    gl.Bound(Texture::Target::_2D, this->uiTextures[index])
    .Image2D(0, PixelDataInternalFormat::RGB8, 512, 512, 0, PixelDataFormat::BGR,
             PixelDataType::UnsignedByte, bits)
    .MinFilter(TextureMinFilter::Nearest)
    .MagFilter(TextureMagFilter::Nearest)
    .WrapS(TextureWrap::ClampToEdge)
    .WrapT(TextureWrap::ClampToEdge);
    // unload data from memory once uploaded to gpu
    FreeImage_Unload(dib);
    // set range as texture active
    this->ranges[index * 3 + 2] = 1.0f;
}

TerrainMultiTexture::TerrainMultiTexture()
{
    gl.Bound(Texture::Target::_2DArray, this->texture)
    .Image3D(0, PixelDataInternalFormat::SRGB8, 512, 512, 4, 0,
             PixelDataFormat::BGR, PixelDataType::UnsignedByte, nullptr);

    for(int i = 0; i < MAX_TERRAIN_TEXTURE_RANGES; i++)
    {
        this->ranges[i * 3] = (float)(i) / MAX_TERRAIN_TEXTURE_RANGES;
        this->ranges[i * 3 + 1] = (float)(i + 1) / (MAX_TERRAIN_TEXTURE_RANGES);
        this->ranges[i * 3 + 2] = -1.0f;
    }
}

TerrainMultiTexture::~TerrainMultiTexture()
{
}

void TerrainMultiTexture::SetTextureRange(const int index, const float start,
        const float end)
{
    if(index < 0 || index >= MAX_TERRAIN_TEXTURE_RANGES) return;

    this->ranges[index * 3] = start;
    this->ranges[index * 3 + 1] = end;
}

void TerrainMultiTexture::SetUniforms(Program &program)
{
    Uniform<Vec3f>(program, "terrainRange").SetValues(3 *
            MAX_TERRAIN_TEXTURE_RANGES, ranges);
}

GLuint TerrainMultiTexture::UITextureId(const int index)
{
    return index >= 0 && index < MAX_TERRAIN_TEXTURE_RANGES ?
           GetGLName(uiTextures[index]) : 0;
}
