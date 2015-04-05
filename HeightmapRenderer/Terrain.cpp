#include "Commons.h"
#include "Terrain.h"
#include "TransformationMatrices.h"
using namespace boost::algorithm;

static glm::vec3 lerp(const glm::vec3& A, const glm::vec3& B, float t)
{
    return A * t + B * (1.f - t);
}

glm::vec3 Terrain::calculateLightDir(float time)
{
    float dirX = std::sin(time);
    float dirY = std::cos(time) + 0.7f;
    float dirZ = 0.7;
    return glm::vec3(dirX, std::abs(dirY), dirZ);
}

void Terrain::calculateLightDir(float time, glm::vec3 &outDir,
                                glm::vec3 &outColor)
{
    outColor = glm::vec3(1.0f, 1.0f, 0.86f);
    float dirX = std::sin(time);
    float dirY = std::cos(time) + 0.7f;
    float dirZ = 0.7f;

    if(enableTimeOfTheDayColorGrading)
    {
        // day light gradually turn to dawn
        if(dirY >= 0.3f)
        {
            outColor =
                lerp(
                    glm::vec3(1.0f, 1.00f, 0.86f),
                    glm::vec3(1.0f, 0.23f, 0.23f),
                    (dirY - 0.3f) / 1.4
                );
        }

        if(dirY < 0.3 && dirY > 0.0f)
        {
            outColor =
                lerp(
                    glm::vec3(1.0f, 0.23f, 0.23f),
                    glm::vec3(0.01, 0.01, 0.01),
                    dirY * (1.0f / 0.3f)
                );
        }

        // night moonlight
        if(dirY <= 0.0f)
        {
            float moonFactor = (1.0 / 0.3) * std::abs(dirY);
            outColor = glm::vec3(0.07, 0.23, 0.63) * moonFactor;
        }
    }

    outDir = glm::vec3(dirX, std::abs(dirY), dirZ);
}

void Terrain::display()
{
    if(!meshCreated) return;

    // reset original state
    {
        program.Use();
        bindBuffers();
        gl.Enable(Capability::DepthTest);
        gl.Enable(Capability::CullFace);
        gl.FrontFace(FaceOrientation::CW);
        gl.CullFace(Face::Back);
    }
    // set shader uniforms
    {
        this->terrainTextures.SetUniforms(program);
        static glm::vec3 lightColor, lightDir;
        // lighting calculations
        calculateLightDir(glfwGetTime() * timeScale, lightDir, lightColor);
        glm::vec4 lightDirectionCameraSpace = TransformationMatrices::View()
                                              * glm::vec4(lightDir, 0.0f);
        // lighting
        Uniform<glm::vec3>(program, "directionalLight.direction").Set(
            glm::vec3(lightDirectionCameraSpace)
        );
        Uniform<glm::vec3>(program, "directionalLight.base.color").Set(
            lightColor
        );
        // set scene matrices uniforms
        Uniform<glm::mat4>(program, "matrix.modelViewProjection").Set(
            TransformationMatrices::ModelViewProjection()
        );
        Uniform<glm::mat4>(program, "matrix.modelView").Set(
            TransformationMatrices::ModelView()
        );
        Uniform<glm::mat4>(program, "matrix.normal").Set(
            TransformationMatrices::Normal()
        );
        // shader time handler
        Uniform<GLfloat>(program, "currentLightmap").Set(
            glfwGetTime() * ((timeScale * (3 * M_PI + 2)) / lightmapsFrequency)
        );
    }
    // draw mesh
    gl.DrawElements(PrimitiveType::TriangleStrip, indices.size(),
                    DataType::UnsignedInt);

    if(bakingDone)
    {
        // pass data to interface texture
        gl.Bound(Texture::Target::_3D, this->terrainTOTDLightmap)
        .MinFilter(TextureMinFilter::Linear)
        .MagFilter(TextureMagFilter::Linear)
        .Anisotropy(2.0f)
        .WrapS(TextureWrap::Repeat)
        .WrapT(TextureWrap::Repeat)
        .Image3D(0, PixelDataInternalFormat::R8, terrainResolution,
                 terrainResolution, this->lightmapsFrequency, 0,
                 PixelDataFormat::Red, PixelDataType::UnsignedByte, terrainLightmapsData);
        BOOST_LOG_TRIVIAL(info) << "Baking Done, "
                                << this->lightmapsFrequency
                                << " lightmaps created, "
                                << (float)this->lightmapsFrequency / 24.0f
                                << " per hour";
        bakingDone = false;
        delete[]terrainLightmapsData;
    }
}

void Terrain::bindBuffers()
{
    buffer[0].Bind(Buffer::Target::Array);
    {
        (program | 0).Setup<GLfloat>(3).Enable();
    }
    buffer[1].Bind(Buffer::Target::Array);
    {
        (program | 1).Setup<GLfloat>(3).Enable();
    }
    buffer[2].Bind(Buffer::Target::Array);
    {
        (program | 2).Setup<GLfloat>(2).Enable();
    }
    buffer[3].Bind(Buffer::Target::ElementArray);
}

void Terrain::fastGenerateShadowmapParallel(
    glm::vec3 lightDir,
    std::vector<unsigned char> &lightmap
)
{
    fastGenerateShadowmapParallel(lightDir, lightmap, terrainResolution);
}

void Terrain::fastGenerateShadowmapParallel(glm::vec3 lightDir,
        unsigned int lightmapSize)
{
    if(!heightmapCreated) return;

    std::vector<unsigned char>lightmap;
    this->fastGenerateShadowmapParallel(lightDir, lightmap, lightmapSize);
    // pass raw data to texture
    gl.Bound(Texture::Target::_2D, this->terrainShadowmap)
    .Image2D(0, PixelDataInternalFormat::R8, lightmapSize,
             lightmapSize, 0,
             PixelDataFormat::Red, PixelDataType::UnsignedByte, &lightmap[0])
    .MinFilter(TextureMinFilter::Linear)
    .MagFilter(TextureMagFilter::Linear)
    .WrapS(TextureWrap::Repeat)
    .WrapT(TextureWrap::Repeat);
    // clear vector
    lightmap.clear();
}

void Terrain::fastGenerateShadowmapParallel(glm::vec3 lightDir,
        std::vector<unsigned char> &lightmap, unsigned int lightmapSize)
{
    if(!heightmapCreated) return;

    if(glm::length2(lightDir) == 0.0) return;

    // initialize shadow map
    lightmap = std::vector<unsigned char>(lightmapSize * lightmapSize);
    // create flag buffer to indicate where we've been
    std::vector<float> flagMap(lightmapSize * lightmapSize);
    // calculate absolute values for light direction
    float lightDirXMagnitude = lightDir[0];
    float lightDirZMagnitude = lightDir[2];

    if(lightDirXMagnitude < 0) lightDirXMagnitude *= -1;

    if(lightDirZMagnitude < 0) lightDirZMagnitude *= -1;

    float distanceStep = sqrtf(lightDir[0] * lightDir[0] + lightDir[1] *
                               lightDir[1]);
    // decide which loop will come first, the y loop or x loop
    // based on direction of light, makes calculations faster
    // outer loop
    float sFactor = (float)terrainResolution / lightmapSize;
    concurrency::parallel_for(int(0), (int)lightmapSize, [&](int y)
    {
        int *X, *Y;
        int iX, iY;
        int dirX, dirY;

        // this might seem like a waste, why calculate it here? you can calculate it before...
        // that's because threading is really picky about sharing variables. the less you share,
        // the faster it goes.
        if(lightDirXMagnitude > lightDirZMagnitude)
        {
            Y = &iX;
            X = &iY;

            if(lightDir[0] < 0)
                dirY = -1;
            else
                dirY = 1;

            if(lightDir[2] < 0)
                dirX = -1;
            else
                dirX = 1;
        }
        else
        {
            Y = &iY;
            X = &iX;

            if(lightDir[0] < 0)
                dirX = -1;
            else
                dirX = 1;

            if(lightDir[2] < 0)
                dirY = -1;
            else
                dirY = 1;
        }

        // if you decide to just do it single-threaded,
        // just copy the previous block back just above the for loop

        if(dirY < 0)
            iY = lightmapSize - y - 1;
        else
            iY = y;

        // inner loop
        for(int x = 0; x < lightmapSize; x++)
        {
            if(dirX < 0)
                iX = lightmapSize - x - 1;
            else
                iX = x;

            float px, py, height, distance, origX, origY;
            int index;
            // travel along the terrain until we:
            // (1) intersect another point
            // (2) find another point with previous collision data
            // (3) or reach the edge of the map
            px = (float) * X;
            py = (float) * Y;
            origX = px;
            origY = py;
            index = (*Y) * lightmapSize + (*X);
            distance = 0.0f;

            // travel along ray
            while(1)
            {
                px -= lightDir[0];
                py -= lightDir[2];

                // check if we've reached the boundary
                if(px < 0 || px >= lightmapSize - 1 || py < 0 ||
                   py >= lightmapSize - 1)
                {
                    flagMap[index] = -1;
                    break;
                }

                // calculate interpolated values
                int x0, x1, y0, y1;
                float du, dv;
                float interpolatedHeight, interpolatedFlagMap;
                float invdu, invdv;
                float w0, w1, w2, w3;
                x0 = int(px);
                y0 = int(py);
                x1 = x0 + 1;
                y1 = y0 + 1;
                du = px - x0;
                dv = py - y0;
                invdu = 1.0f - du;
                invdv = 1.0f - dv;
                w0 = invdu * invdv;
                w1 = invdu * dv;
                w2 = du * invdv;
                w3 = du * dv;
                // compute interpolated height value from the heightmap direction below ray
                interpolatedHeight =
                    w0 * clamp(heightmap.getValue(x0 * sFactor, y0 * sFactor), 0.0, 1.0) * 255
                    + w1 * clamp(heightmap.getValue(x0 * sFactor, y1 * sFactor), 0.0, 1.0) * 255
                    + w2 * clamp(heightmap.getValue(x1 * sFactor, y0 * sFactor), 0.0, 1.0) * 255
                    + w3 * clamp(heightmap.getValue(x1 * sFactor, y1 * sFactor), 0.0, 1.0) * 255;
                // compute interpolated flagmap value from point directly below ray
                interpolatedFlagMap = w0 * flagMap[y0 * lightmapSize + x0]
                                      + w1 * flagMap[y1 * lightmapSize + x0]
                                      + w2 * flagMap[y0 * lightmapSize + x1]
                                      + w3 * flagMap[y1 * lightmapSize + x1];
                // get distance from original point to current point
                //distance = sqrtf( (px-origX)*(px-origX) + (py-origY)*(py-origY) );
                distance += distanceStep;
                // get height at current point while traveling along light ray
                height = clamp(heightmap.getValue(*X * sFactor, *Y * sFactor), 0.0,
                               1.0) * 255 + lightDir[1] * distance;
                // check intersection with either terrain or flagMap
                // if interpolatedHeight is less than interpolatedFlagMap that means
                // we need to use the flagMap value instead
                // else use the height value
                float val;

                if(interpolatedHeight < interpolatedFlagMap) val = interpolatedFlagMap;
                else val = interpolatedHeight;

                if(height < val)
                {
                    flagMap[index] = val - height;
                    lightmap[index] = 192;
                    break;
                }

                // check if pixel we've moved to is unshadowed
                // since the flagMap value we're using is interpolated, we will be in
                // between shadowed and unshadowed areas
                // to compensate for this, simply define some epsilon value and use
                // this as an offset from -1 to decide
                // if current point under the ray is unshadowed
                static float epsilon = 0.5f;

                if(interpolatedFlagMap < -1.0f + epsilon &&
                   interpolatedFlagMap > -1.0f - epsilon)
                {
                    flagMap[index] = -1.0f;
                    break;
                }
            }
        }
    });
}

void Terrain::createTerrain(const int heightmapSize)
{
    // no need to redo the same operation
    if(heightmapSize == terrainResolution && heightmapCreated ||
       heightmapSize < 1) return;

    this->terrainResolution = heightmapSize;
    heightmap.setSize(terrainResolution, terrainResolution);
    heightmap.setBounds(0, 4, 0, 4);
    heightmap.build();
    heightmapCreated = true;
    meshCreated = false;
    program.Use();
    Uniform<glm::vec2>(program, "terrainMapSize")
    .Set(glm::vec2(terrainResolution, terrainResolution));
    // reserve memory in main thread for new lightmap data
    terrainLightmapsData = new unsigned char[
        this->terrainResolution * this->terrainResolution * 72
    ];

    // wait if still working on previous data
    if(this->bakingThread.joinable()) this->bakingThread.join();

    // bake all the lightmaps in a separate thread so it doesn't freeze the main thread
    this->bakingThread = std::thread(&Terrain::bakeTimeOfTheDayShadowmap, this, 72);
}

void Terrain::createMesh(const int meshResExponent)
{
    using namespace  boost::algorithm;

    // will not create a mesh until height data is ready
    if(!heightmapCreated) return;

    int newResolution = (int)std::pow(2, meshResExponent) + 1;

    // no need to redo the same operation
    if(newResolution == meshResolution && meshCreated) return;

    this->meshResolution = newResolution;
    // clear any previous data
    this->indices.clear();
    this->vertices.clear();
    this->texCoords.clear();
    this->normals.clear();
    // reserve space for new data
    vertices.resize(meshResolution * meshResolution);
    normals.resize(meshResolution * meshResolution);
    texCoords.resize(meshResolution * meshResolution);
    indices.resize((meshResolution - 1) * meshResolution * 2 + meshResolution);
    // load mesh positions and heights as vertices
    float textureU = (float)meshResolution * 0.1f;
    float textureV = (float)meshResolution * 0.1f;
    // index buffer restart triangle strip
    int restartIndex = meshResolution * meshResolution;
    // parallel modification
    concurrency::parallel_for(int(0), meshResolution, [&](int i)
    {
        int indexAt = i * (2 * meshResolution + 1);

        for(int j = 0; j < meshResolution; j++)
        {
            // scales to x, z [0.0, enlargeMap]
            float colScale = (float)j / (meshResolution - 1);
            float rowScale = (float)i / (meshResolution - 1);
            // height map positions
            int xCor = (int)(j * (float)terrainResolution / meshResolution);
            int yCor = (int)(i * (float)terrainResolution / meshResolution);
            float vertexHeight = heightmap.getValue(xCor, yCor);
            // transform from [-1,1] to [0,1]
            vertexHeight = clamp((vertexHeight + 1.0f) / 2.0f, 0.0f, 1.0f);
            // create vertex position
            vertices[i * meshResolution + j] =
                glm::vec3(
                    -0.5f + colScale,
                    vertexHeight,
                    -0.5f + rowScale
                );
            // also create the appropiate texcoord
            texCoords[i * meshResolution + j] =
                glm::vec2(
                    colScale,
                    rowScale
                );

            // create triangle strip indices
            if(i != meshResolution - 1)
            {
                indices[j * 2 + indexAt] = ((i + 1) * meshResolution + j);
                indices[(j + 1) * 2 - 1 + indexAt] = (i * meshResolution + j);
            }
        }

        // indices restart token
        if(i != meshResolution - 1)
        {
            int restartAt = 2 * (i + 1) * meshResolution + i;
            indices[restartAt] = restartIndex;
        }
    });
    // calculate face normals
    std::array<std::vector<std::vector<glm::vec3>>, 2> faceNormals;
    faceNormals[0] = faceNormals[1] = std::vector<std::vector<glm::vec3>>
                                      (meshResolution - 1, std::vector<glm::vec3>(meshResolution - 1));
    concurrency::parallel_for(int(0), meshResolution - 1, [&](int i)
    {
        for(int j = 0; j < meshResolution - 1; j++)
        {
            glm::vec3 triangle0[] =
            {
                vertices[i * meshResolution + j],
                vertices[(i + 1) * meshResolution + j],
                vertices[(i + 1) * meshResolution + j + 1]
            };
            glm::vec3 triangle1[] =
            {
                vertices[(i + 1) * meshResolution + j + 1],
                vertices[i * meshResolution + j + 1],
                vertices[i * meshResolution + j]
            };
            glm::vec3 t0Normal = glm::cross(triangle0[0] - triangle0[1],
                                            triangle0[1] - triangle0[2]);
            glm::vec3 t1Normal = glm::cross(triangle1[0] - triangle1[1],
                                            triangle1[1] - triangle1[2]);
            faceNormals[0][i][j] = glm::normalize(t0Normal);
            faceNormals[1][i][j] = glm::normalize(t1Normal);
        }
    });
    concurrency::parallel_for(int(0), meshResolution - 1, [&](int i)
    {
        for(int j = 0; j < meshResolution; j++)
        {
            glm::vec3 fNormal = glm::vec3(0.f, 0.f, 0.f);

            // upper left faces
            if(j != 0 && i != 0)
            {
                fNormal += faceNormals[0][i - 1][j - 1] + faceNormals[1][i - 1][j - 1];
            }

            // upper right faces
            if(i != 0 && j != meshResolution - 1)
            {
                fNormal += faceNormals[0][i - 1][j];
            }

            // bottom right faces
            if(i != meshResolution - 1 && j != meshResolution - 1)
            {
                fNormal += faceNormals[0][i][j] + faceNormals[1][i][j];
            }

            // bottom left faces
            if(i != meshResolution - 1 && j != 0)
            {
                fNormal += faceNormals[1][i][j - 1];
            }

            normals[i * meshResolution + j] = glm::normalize(fNormal);
        }
    });
    // upload position data to the gpu
    buffer[0].Bind(Buffer::Target::Array);
    {
        GLuint nPerVertex = glm::vec3().length();
        Buffer::Data(Buffer::Target::Array, vertices);
        // setup the vertex attribs array for the vertices
        (program | 0).Setup<GLfloat>(nPerVertex).Enable();
    }
    buffer[1].Bind(Buffer::Target::Array);
    {
        GLuint nPerVertex = glm::vec3().length();
        // upload the data
        Buffer::Data(Buffer::Target::Array, normals);
        // setup the vertex attribs array for the vertices
        (program | 1).Setup<GLfloat>(nPerVertex).Enable();
    }
    buffer[2].Bind(Buffer::Target::Array);
    {
        GLuint nPerVertex = glm::vec2().length();
        // upload the data
        Buffer::Data(Buffer::Target::Array, texCoords);
        // setup the vertex attribs array for the vertices
        (program | 2).Setup<GLfloat>(nPerVertex).Enable();
    }
    buffer[3].Bind(Buffer::Target::ElementArray);
    {
        Buffer::Data(Buffer::Target::ElementArray, indices);
        gl.Enable(Capability::PrimitiveRestart);
        gl.PrimitiveRestartIndex(restartIndex);
    }
    meshCreated = true;
}

void Terrain::setTextureRepeatFrequency(const glm::vec2 &value)
{
    program.Use();
    Uniform<glm::vec2>(program, "terrainUVScaling")
    .Set(value);
}

void Terrain::setTextureRange(const int index, const float start,
                              const float end)
{
    this->terrainTextures.SetTextureRange(index, start, end);
}

void Terrain::loadTexture(const int index, const std::string &filepath)
{
    this->terrainTextures.loadTexture(filepath, index);
}

GLuint Terrain::getTextureId(int index)
{
    return this->terrainTextures.UITextureId(index);
}

Terrain::Terrain() : terrainMaxHeight(1.0f), heightmapCreated(false),
    meshCreated(false), timeScale(0.1f)
{
    this->lightmapsFrequency = 24.0f;
    this->maxHeight = std::numeric_limits<float>::min();
    this->minHeight = std::numeric_limits<float>::max();
}

void Terrain::initialize()
{
    vertexShader.Source(GLSLSource::FromFile("Resources/Shaders/terrain.vert"));
    // compile it
    vertexShader.Compile();
    // set the fragment shader source
    fragmentShader.Source(GLSLSource::FromFile("Resources/Shaders/terrain.frag"));
    // compile it
    fragmentShader.Compile();
    // attach the shaders to the program
    program.AttachShader(vertexShader);
    program.AttachShader(fragmentShader);
    // link and use it
    program.Link();
    program.Use();
    // set prog uniforms
    Uniform<GLfloat>(program, "directionalLight.base.intensity").Set(
        0.85f
    );
    Uniform<glm::vec3>(program, "directionalLight.base.color").Set(
        // full sunlight
        glm::vec3(1.0f, 1.0f, 0.9f)
    );
    Uniform<glm::vec3>(program, "directionalLight.direction").Set(
        glm::vec3(0.5f, 0.7f, 0.7f)
    );
    Uniform<GLfloat>(program, "lightParams.ambientCoefficient").Set(
        0.1f
    );
    Uniform<GLint>(program, "lightParams.spotLightCount").Set(
        0
    );
    Uniform<GLint>(program, "lightParams.pointLightCount").Set(
        0
    );
    Uniform<GLfloat>(program, "material.shininess").Set(
        32
    );
    Uniform<glm::vec3>(program, "material.specular").Set(
        glm::vec3(0.2, 0.95, 0.15)
    );
    Uniform<GLfloat>(program, "material.shininessStrength").Set(
        0.016f
    );
    Uniform<GLfloat>(program, "maxHeight").Set(
        this->terrainMaxHeight
    );
    Uniform<glm::vec2>(program, "terrainUVScaling").Set(
        glm::vec2(25, 25)
    );
    TransformationMatrices::Model(
        glm::scale(glm::mat4(), glm::vec3(15, terrainMaxHeight, 15))
    );
    terrainMesh.Bind();
    // context flags
    gl.Enable(Capability::DepthTest);
    gl.Enable(Capability::CullFace);
    gl.FrontFace(FaceOrientation::CW);
    gl.CullFace(Face::Back);
}

void Terrain::bakeTimeOfTheDayShadowmap(float freq)
{
    if(!heightmapCreated) return;

    this->lightmapsFrequency = (int)std::ceil(freq);
    int sizeFreq = this->lightmapsFrequency;

    for(int i = 0; i < sizeFreq; i++)
    {
        std::vector<unsigned char> bakedLightmap;
        this->fastGenerateShadowmapParallel(
            calculateLightDir(2.0f * M_PI * (float)i / (sizeFreq - 1)),
            bakedLightmap
        );
        std::copy(
            bakedLightmap.begin(), bakedLightmap.end(),
            &this->terrainLightmapsData[
                i * this->terrainResolution * this->terrainResolution
            ]
        );
        BOOST_LOG_TRIVIAL(info) << "Baking Info: Lightmap "
                                << i + 1 << "/"
                                << lightmapsFrequency
                                << " ("
                                << (int)(100 * (float)i / (lightmapsFrequency - 1))
                                << "%) created";
        bakedLightmap.clear();
    };

    this->bakingDone = true;
}

Terrain::~Terrain()
{
    if(this->bakingThread.joinable()) this->bakingThread.join();
}