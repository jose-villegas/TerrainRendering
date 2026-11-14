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

    if(!dib) return;

    // highest quality rescaleing
    FIBITMAP *rescaled = FreeImage_Rescale(dib, 512, 512, FILTER_BICUBIC);
    FreeImage_Unload(dib);
    // no alpha channel for terrain textures
    dib = FreeImage_ConvertTo24Bits(rescaled);
    FreeImage_Unload(rescaled);
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

    // mean color for the light baker, rows are bgr
    glm::dvec3 sum(0.0);

    for(unsigned int y = 0; y < height; y++)
    {
        const unsigned char *row = bits + y * FreeImage_GetPitch(dib);

        for(unsigned int x = 0; x < width; x++)
        {
            sum += glm::dvec3(row[x * 3 + 2], row[x * 3 + 1], row[x * 3]);
        }
    }

    averageColor[index] = glm::vec3(sum / (255.0 * width * height));
    // pass data to the texture array, the mipmaps are generated after the
    // layer upload, otherwise the new layer keeps its previous empty mipmaps
    gl.Bound(Texture::Target::_2DArray, this->texture)
    .MinFilter(TextureMinFilter::LinearMipmapLinear)
    .MagFilter(TextureMagFilter::Linear)
    .WrapS(TextureWrap::Repeat)
    .WrapT(TextureWrap::Repeat)
    .Anisotropy(16.0f)
    .SubImage3D(0, 0, 0, index, 512, 512, 1, PixelDataFormat::BGR,
                PixelDataType::UnsignedByte, bits)
    .GenerateMipmap();
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

float TerrainMultiTexture::Albedo(float height) const
{
    glm::vec3 color(0.0f);

    // same weights as generateTerrainColor in terrain.frag
    for(int i = 0; i < MAX_TERRAIN_TEXTURE_RANGES; i++)
    {
        float regionMin = ranges[i * 3], regionMax = ranges[i * 3 + 1];
        float regionRange = regionMax - regionMin;
        float regionOffset = height - regionMax;

        if((i == 0 && regionOffset < 0.0f)
           || (i == MAX_TERRAIN_TEXTURE_RANGES - 1 && regionOffset > 0.0f))
        {
            regionOffset = 0.0f;
        }

        if(regionRange <= 0.0f) continue;

        float weight = std::max(0.0f, (regionRange - std::abs(regionOffset)) / regionRange);
        color += weight * averageColor[i];
    }

    return util::clamp(glm::dot(color, glm::vec3(0.2126f, 0.7152f, 0.0722f)),
                       0.0f, 0.9f);
}

TerrainMultiTexture::TerrainMultiTexture()
{
    // the terrain shader colors for ranges without a texture
    averageColor[0] = glm::vec3(0.1f, 0.7f, 1.0f);
    averageColor[1] = glm::vec3(0.7f, 0.7f, 0.5f);
    averageColor[2] = glm::vec3(0.3f, 0.5f, 0.1f);
    averageColor[3] = glm::vec3(1.0f, 0.9f, 0.9f);

    // plain rgb, the terrain shader outputs without gamma correction
    gl.Bound(Texture::Target::_2DArray, this->texture)
    .Image3D(0, PixelDataInternalFormat::RGB8, 512, 512, 4, 0,
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
