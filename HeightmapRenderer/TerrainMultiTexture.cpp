#include "Commons.h"
#include "TerrainMultiTexture.h"


void TerrainMultiTexture::loadTexture(const std::string &filepath,
                                      const int index)
{
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

    // Check Image Bit Density
    PixelDataFormat pixelFormat = bitsPerPixel == 32 ? PixelDataFormat::BGRA :
                                  bitsPerPixel == 24 ? PixelDataFormat::BGR :
                                  bitsPerPixel == 16 ? PixelDataFormat::RG :
                                  PixelDataFormat::Red;
    // pass data to the texture array
    gl.Bound(Texture::Target::_2DArray, this->texture)
    .MinFilter(TextureMinFilter::Linear)
    .MagFilter(TextureMagFilter::Linear)
    .Anisotropy(2.0f)
    .WrapS(TextureWrap::Repeat)
    .WrapT(TextureWrap::Repeat)
    .SubImage3D(0, 0, 0, index, 512, 512, 1, pixelFormat,
                PixelDataType::UnsignedByte, bits);
    // pass data to interface texture
    gl.Bound(Texture::Target::_2D, this->uiTextures[index])
    .Image2D(0, PixelDataInternalFormat::RGBA8, 512, 512, 0,
             pixelFormat, PixelDataType::UnsignedByte, bits)
    .MinFilter(TextureMinFilter::Linear)
    .MagFilter(TextureMagFilter::Linear)
    .Anisotropy(2.0f)
    .WrapS(TextureWrap::Repeat)
    .WrapT(TextureWrap::Repeat);
    // unload data from memory once uploaded to gpu
    FreeImage_Unload(dib);
}

TerrainMultiTexture::TerrainMultiTexture() : rangeStart(0.0f), rangeEnd(0.0f)
{
    gl.Bound(Texture::Target::_2DArray, this->texture)
    .Image3D(0, PixelDataInternalFormat::RGBA8, 512, 512, 4, 0,
             PixelDataFormat::BGRA, PixelDataType::UnsignedByte, nullptr);
}

TerrainMultiTexture::~TerrainMultiTexture()
{
}
