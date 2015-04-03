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

void Terrain::createTerrain(const int heightmapSize)
{
    // no need to redo the same operation
    if(heightmapSize == terrainResolution && heightmapCreated ||
       heightmapSize < 1) return;

    this->terrainResolution = heightmapSize;
    heightmap.setSize(terrainResolution, terrainResolution);
    heightmap.setBounds(0, 4, 0, 4);
    heightmap.build();
    heightmap.writeToFile("terrain");
    heightmapCreated = true;
    meshCreated = false;
}

void Terrain::createMesh(const int meshResExponent)
{
    // will not create a mesh until height data is ready
    if(!heightmapCreated) return;

    int newResolution = std::pow(2, meshResExponent) + 1;

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
    float enlargeMap = 30.0;
    // maximum height / texel space
    float heightTexelRatio = 1.0 / (meshResolution / enlargeMap);
    // parallel modification
    concurrency::parallel_for(int(0), meshResolution, [&](int i)
    {
        int indexAt = i * (2 * meshResolution + 1);

        for(int j = 0; j < meshResolution; j++)
        {
            // scales to x, z [0.0, 1.0]
            float colScale = enlargeMap * (float)j / (meshResolution - 1);
            float rowScale = enlargeMap * (float)i / (meshResolution - 1);
            // height map positions
            int xCor = j * (float)terrainResolution / meshResolution;
            int yCor = i * (float)terrainResolution / meshResolution;
            float vertexHeight = heightmap.getValue(xCor, yCor);
            // transform from [-1,1] to [0,1]
            vertexHeight = std::min(1.0f, std::max(0.0f, (vertexHeight + 1.0f) / 2.0f));
            // create vertex position
            vertices[i * meshResolution + j] =
                glm::vec3(
                    -enlargeMap / 2.0f + colScale,
                    vertexHeight,
                    -enlargeMap / 2.0f + rowScale
                );
            // also create the appropiate texcoord
            texCoords[i * meshResolution + j] =
                glm::vec2(textureU * colScale, textureV * rowScale);
            // calculate approximate normal at vertex position
            float h0 = heightmap.getValue(xCor + 1, yCor);
            float h1 = heightmap.getValue(xCor - 1, yCor);
            float h2 = heightmap.getValue(xCor, yCor + 1);
            float h3 = heightmap.getValue(xCor, yCor + 1);
            normals[i * meshResolution + j] =
                glm::normalize(glm::vec3(h1 - h0, 2.0 * heightTexelRatio, h3 - h2));

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
