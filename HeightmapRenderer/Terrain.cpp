#include "Commons.h"
#include "Terrain.h"
#include "TransformationMatrices.h"

void Terrain::display()
{
    if(!meshCreated) return;

    // reset original state
    {
        gl.Enable(Capability::DepthTest);
        gl.Enable(Capability::CullFace);
        gl.FrontFace(FaceOrientation::CW);
        gl.CullFace(Face::Back);
        program.Use();
        bindBuffers();
    }
    // set shader uniforms
    {
        glm::vec4 lightDirectionCameraSpace =
            TransformationMatrices::View()
            * glm::vec4(0.5f, 0.7f, 0.7f, 0.0f);
        // set prog uniforms
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

void Terrain::createTerrain(int sizeExponent)
{
    this->terrainSize = std::pow(2, sizeExponent) + 1;
    heightmap.setSize(terrainSize, terrainSize);
    heightmap.setBounds(0, 4, 0, 4);
    heightmap.build();
    heightmap.writeToFile("terrain");
    heightmapCreated = true;
}

void Terrain::createMesh()
{
    // will not create a mesh until height data is ready
    if(!heightmapCreated) return;

    // clear any previous data
    this->indices.clear();
    this->vertices.clear();
    this->texCoords.clear();
    this->normals.clear();
    // reserve space for new data
    vertices.resize(terrainSize * terrainSize);
    normals.resize(terrainSize * terrainSize);
    texCoords.resize(terrainSize * terrainSize);
    indices.resize((terrainSize - 1) * terrainSize * 2 + terrainSize);
    // load mesh positions and heights as vertices
    float textureU = (float)terrainSize * 0.1f;
    float textureV = (float)terrainSize * 0.1f;
    // index buffer restart triangle strip
    int restartIndex = terrainSize * terrainSize;
    float enlargeMap = 17.0;
    // maximum height / texel space
    float heightTexelRatio = 1.0 / enlargeMap;
    // parallel modification
    concurrency::parallel_for(int(0), terrainSize, [&](int i)
    {
        int indexAt = i * (2 * terrainSize + 1);

        for(int j = 0; j < terrainSize; j++)
        {
            // scales to x, z [0.0, 1.0]
            float colScale = enlargeMap * (float)j / (terrainSize - 1);
            float rowScale = enlargeMap * (float)i / (terrainSize - 1);
            float vertexHeight = heightmap.getValue(j, i);
            // create vertex position
            vertices[i * terrainSize + j] =
                glm::vec3(
                    -enlargeMap / 2.0f + colScale,
                    vertexHeight,
                    -enlargeMap / 2.0f + rowScale
                );
            // also create the appropiate texcoord
            texCoords[i * terrainSize + j] =
                glm::vec2(textureU * colScale, textureV * rowScale);
            // calculate approximate normal at vertex position
            float h0 = heightmap.getValue(j + 1, i);
            float h1 = heightmap.getValue(j - 1, i);
            float h2 = heightmap.getValue(j, i + 1);
            float h3 = heightmap.getValue(j, i + 1);
            normals[i * terrainSize + j] =
                glm::normalize(glm::vec3(h1 - h0, 2.0 * heightTexelRatio, h3 - h2));

            // create triangle strip indices
            if(i != terrainSize - 1)
            {
                indices[j * 2 + indexAt] = ((i + 1) * terrainSize + j);
                indices[(j + 1) * 2 - 1 + indexAt] = (i * terrainSize + j);
            }
        }

        // indices restart token
        if(i != terrainSize - 1)
        {
            int restartAt = 2 * (i + 1) * terrainSize + i;
            indices[restartAt] = restartIndex;
        }
    });
    //for(int i = 0; i < terrainSize; i++)
    //{
    //    int indexAt = i * (2 * terrainSize + 1);
    //    for(int j = 0; j < terrainSize; j++)
    //    {
    //        // scales to x, z [0.0, 1.0]
    //        float colScale = enlargeMap * (float)j / (terrainSize - 1);
    //        float rowScale = enlargeMap * (float)i / (terrainSize - 1);
    //        float vertexHeight = heightmap.getValue(j, i);
    //        // create vertex position
    //        vertices[i * terrainSize + j] =
    //            glm::vec3(
    //                -enlargeMap / 2.0f + colScale,
    //                (vertexHeight + 1.0f) / 2.0f,
    //                -enlargeMap / 2.0f + rowScale
    //            );
    //        // also create the appropiate texcoord
    //        texCoords[i * terrainSize + j] =
    //            glm::vec2(textureU * colScale, textureV * rowScale);
    //        // calculate approximate normal at vertex position
    //        float h0 = heightmap.getValue(j + 1, i);
    //        float h1 = heightmap.getValue(j - 1, i);
    //        float h2 = heightmap.getValue(j, i + 1);
    //        float h3 = heightmap.getValue(j, i + 1);
    //        normals[i * terrainSize + j] =
    //            glm::normalize(glm::vec3(h1 - h0, 2.0 * heightTexelRatio, h3 - h2));
    //        // create triangle strip indices
    //        if(i != terrainSize - 1)
    //        {
    //            //indices[i * terrainSize + j] = ((i + 1) * terrainSize + j);
    //            //indices[i * terrainSize + j + 1] = (i * terrainSize + j);
    //            indices[j * 2 + indexAt] = ((i + 1) * terrainSize + j);
    //            indices[(j + 1) * 2 - 1 + indexAt] = (i * terrainSize + j);
    //            //indices.push_back((i + 1) * terrainSize + j);
    //            //indices.push_back(i * terrainSize + j);
    //        }
    //    }
    //    // indices restart token
    //    if(i != terrainSize - 1)
    //    {
    //        int restartAt = 2 * (i + 1) * terrainSize + i;
    //        indices[restartAt] = restartIndex;
    //        //indices.push_back(restartIndex);
    //    }
    //}
    //std::vector < std::vector<glm::vec3> >faceNormals[2];
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
