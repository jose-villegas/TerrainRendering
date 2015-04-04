#include "Commons.h"
#include "Terrain.h"
#include "TransformationMatrices.h"

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
        // camera
        glm::vec4 lightDirectionCameraSpace =
            TransformationMatrices::View()
            * glm::vec4(0.5f, 0.7f, 0.7f, 0.0f);
        // lighting
        Uniform<glm::vec3>(program, "directionalLight.direction").Set(
            glm::vec3(lightDirectionCameraSpace)
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
    }
    // draw mesh
    gl.DrawElements(PrimitiveType::TriangleStrip, indices.size(),
                    DataType::UnsignedInt);
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

void Terrain::generateShadowmap(glm::vec3 lightPos)
{
    using namespace noise::utils;

    // can't create shadowmap without height info
    if(!heightmapCreated) return;

    int mapSize = terrainResolution;
    // initialize shadow map
    std::vector<unsigned char>shadowMap(mapSize * mapSize * 3);
    // solve heightmap ray trace
    concurrency::parallel_for(int(0), mapSize, [&](int z)
    {
        for(int x = 0; x < mapSize; x++)
        {
            // height map positions
            glm::vec3 currentPos = glm::vec3((float)x, heightmap.getValue(x, z), (float)z);
            // calculate ray from light direction
            glm::vec3 lightDir = glm::normalize(lightPos - currentPos);
            shadowMap[z * mapSize * 3 + x * 3] = 255;
            shadowMap[z * mapSize * 3 + x * 3 + 1] = 255;
            shadowMap[z * mapSize * 3 + x * 3 + 2] = 255;

            // start ray cast test
            while(currentPos.x >= 0.0f
                  && currentPos.x < mapSize
                  && currentPos.z >= 0
                  && currentPos.z < mapSize
                  && currentPos != lightPos
                  && currentPos.y < 1.0f)
            {
                currentPos += lightDir;
                int lerpX = (int)std::round(currentPos.x);
                int lerpZ = (int)std::round(currentPos.z);

                // ray hit
                if(currentPos.y <= heightmap.getValue(lerpX, lerpZ))
                {
                    shadowMap[z * mapSize * 3 + x * 3] = 0;
                    shadowMap[z * mapSize * 3 + x * 3 + 1] = 0;
                    shadowMap[z * mapSize * 3 + x * 3 + 2] = 0;
                    break;
                }
            }
        }
    });
    // pass raw data to texture
    gl.Bound(Texture::Target::_2D, this->terrainShadowmap)
    .Image2D(0, PixelDataInternalFormat::RGBA8, mapSize, mapSize, 0,
             PixelDataFormat::RGB, PixelDataType::UnsignedByte, &shadowMap[0])
    .MinFilter(TextureMinFilter::Linear)
    .MagFilter(TextureMagFilter::Linear)
    .WrapS(TextureWrap::Repeat)
    .WrapT(TextureWrap::Repeat);
    // delete reserved memory once uploaded to gpu
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
                    textureU * colScale,
                    textureV * rowScale
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
    // test shadow
    generateShadowmap(glm::vec3(0.5f, 0.7f, 0.7f) * 15.0f);
}

Terrain::Terrain() : terrainMaxHeight(1.0f), heightmapCreated(false),
    meshCreated(false)
{
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
        1.0f
    );
    Uniform<glm::vec3>(program, "directionalLight.base.color").Set(
        glm::vec3(1.0f)
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
        0.0f
    );
    Uniform<GLfloat>(program, "maxHeight").Set(
        this->terrainMaxHeight
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

Terrain::~Terrain()
{
}
