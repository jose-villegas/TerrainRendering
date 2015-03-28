#include "Commons.h"
#include "Heightmap.h"
#include "TransformationMatrices.h"

void Heightmap::loadMeshData()
{
    // empty previous data and reserve space
    this->vertices.clear();
    this->texCoords.clear();
    this->normals.clear();
    this->vertices.resize(rows * cols);
    this->texCoords.resize(rows * cols);
    this->normals.resize(rows * cols);
    // write values to position collection
    writePositionsAndTexCoords();
    // calculate face normals
    std::vector<std::vector<glm::vec3>> faceNormals[2]; // 2 triangles per quad face
    writeFaceNormals(faceNormals);
    writeVertexNormals(faceNormals);
    writeIndices();
}

void Heightmap::writeIndices()
{
    // create index buffer data
    indices.clear();
    int restartIndex = rows * cols;

    for(int i = 0; i < rows - 1; i++)
    {
        for(int j = 0; j < cols; j++)
        {
            indices.push_back((i + 1) * cols + j);
            indices.push_back(i * cols + j);
        }

        indices.push_back(restartIndex);
    }
}

void Heightmap::writeVertexNormals(std::vector < std::vector<glm::vec3> >
                                   faceNormals[2])
{
    // calculate per vertex (mesh) normals, averaging face normals
    for(int i = 0; i < rows; i++)
    {
        for(int j = 0; j < cols; j++)
        {
            glm::vec3 fNormal = glm::vec3(0.f, 0.f, 0.f);

            // upper left faces
            if(j != 0 && i != 0)
            {
                fNormal += faceNormals[0][i - 1][j - 1] + faceNormals[1][i - 1][j - 1];
            }

            // upper right faces
            if(i != 0 && j != cols - 1)
            {
                fNormal += faceNormals[0][i - 1][j];
            }

            // bottom right faces
            if(i != rows - 1 && j != cols - 1)
            {
                fNormal += faceNormals[0][i][j] + faceNormals[1][i][j];
            }

            // bottom left faces
            if(i != rows - 1 && j != 0)
            {
                fNormal += faceNormals[1][i][j - 1];
            }

            normals[i * cols + j] = glm::normalize(fNormal);
        }
    }
}

void Heightmap::writeFaceNormals(
    std::vector<std::vector<glm::vec3>> faceNormals[2])
{
    faceNormals[0] = faceNormals[1] = std::vector<std::vector<glm::vec3>>(rows - 1,
                                      std::vector<glm::vec3>(cols - 1));

    for(int i = 0; i < rows - 1; i++)
    {
        for(int j = 0; j < cols - 1; j++)
        {
            glm::vec3 triangle0[] =
            {
                vertices[i * cols + j],
                vertices[(i + 1) * cols + j],
                vertices[(i + 1) * cols + j + 1]
            };
            glm::vec3 triangle1[] =
            {
                vertices[(i + 1) * cols + j + 1],
                vertices[i * cols + j + 1],
                vertices[i * cols + j]
            };
            glm::vec3 t0Normal = glm::cross(triangle0[0] - triangle0[1],
                                            triangle0[1] - triangle0[2]);
            glm::vec3 t1Normal = glm::cross(triangle1[0] - triangle1[1],
                                            triangle1[1] - triangle1[2]);
            faceNormals[0][i][j] = glm::normalize(t0Normal);
            faceNormals[1][i][j] = glm::normalize(t1Normal);
        }
    }
}

void Heightmap::writePositionsAndTexCoords()
{
    float textureU = (float)cols * 0.1f;
    float textureV = (float)rows * 0.1f;
    float maxValue = std::numeric_limits<float>::min();
    float minValue = std::numeric_limits<float>::max();
    noise::module::Perlin noiseGen;
    noise::utils::NoiseMap heightmap;
    noise::utils::NoiseMapBuilderPlane heightmapBuilder;
    heightmapBuilder.SetSourceModule(noiseGen);
    heightmapBuilder.SetDestNoiseMap(heightmap);
    heightmapBuilder.SetDestSize(rows, cols);
    heightmapBuilder.SetBounds(2.0, 6.0, 1.0, 5.0);
    heightmapBuilder.Build();

    for(int i = 0; i < rows; i++)
    {
        for(int j = 0; j < cols; j++)
        {
            float colScale = (float)j / (cols - 1);
            float rowScale = (float)i / (rows - 1);
            float vertexHeight = heightmap.GetValue(i, j);
            maxValue = vertexHeight > maxValue ? vertexHeight : maxValue;
            minValue = vertexHeight <= minValue ? vertexHeight : minValue;
            // assign respective mesh vertex values and texture coords per pixel
            vertices[i * cols + j] = glm::vec3(-0.5f + colScale,
                                               vertexHeight,
                                               -0.5f + rowScale
                                              );
            texCoords[i * cols + j] = glm::vec2(textureU * colScale, textureV * rowScale);
        }
    }

    // scale all values to 0 - 1
    std::for_each(vertices.begin(), vertices.end(),
                  [minValue, maxValue](glm::vec3 & value)
    {
        value.y += std::abs(minValue);
        value.y /= std::abs(minValue) + maxValue;
        // for testing
        value.y /= 3;
    });
}

Heightmap::Heightmap()
{
    vs.Source(GLSLSource::FromFile("Resources/Shaders/terrain.vert"));
    // compile it
    vs.Compile();
    // set the fragment shader source
    fs.Source(GLSLSource::FromFile("Resources/Shaders/terrain.frag"));
    // compile it
    fs.Compile();
    // attach the shaders to the program
    prog.AttachShader(vs);
    prog.AttachShader(fs);
    // link and use it
    prog.Link();
    prog.Use();
}


Heightmap::~Heightmap()
{
}

void Heightmap::loadFromFile(const std::string &srFilename)
{
    rows = 128;
    cols = 128;
    loadMeshData();
    terrainMesh.Bind();
    prog.Use();
    // upload position data to the gpu
    vertexBuffer.Bind(Buffer::Target::Array);
    {
        GLuint nPerVertex = vertices[0].length();
        Buffer::Data(Buffer::Target::Array, vertices);
        // setup the vertex attribs array for the vertices
        (prog | 0).Setup<GLfloat>(nPerVertex).Enable();
    }
    normalBuffer.Bind(Buffer::Target::Array);
    {
        GLuint nPerVertex = normals[0].length();
        // upload the data
        Buffer::Data(Buffer::Target::Array, normals);
        // setup the vertex attribs array for the vertices
        (prog | 1).Setup<GLfloat>(nPerVertex).Enable();
    }
    texCoordsBuffer.Bind(Buffer::Target::Array);
    {
        GLuint nPerVertex = texCoords[0].length();
        // upload the data
        Buffer::Data(Buffer::Target::Array, texCoords);
        // setup the vertex attribs array for the vertices
        (prog | 2).Setup<GLfloat>(nPerVertex).Enable();
    }
    indexBuffer.Bind(Buffer::Target::ElementArray);
    {
        Buffer::Data(Buffer::Target::ElementArray, indices);
        gl.Enable(Capability::PrimitiveRestart);
        gl.PrimitiveRestartIndex(rows * cols);
    }
    Uniform<GLfloat>(prog, "directionalLight.base.intensity").Set(
        1.0f
    );
    Uniform<glm::vec3>(prog, "directionalLight.base.color").Set(
        glm::vec3(1.0f)
    );
    Uniform<glm::vec3>(prog, "directionalLight.direction").Set(
        glm::vec3(0.5f, 0.7f, 0.7f)
    );
    Uniform<GLfloat>(prog, "lightParams.ambientCoefficient").Set(
        0.1f
    );
    Uniform<GLint>(prog, "lightParams.spotLightCount").Set(
        0
    );
    Uniform<GLint>(prog, "lightParams.pointLightCount").Set(
        0
    );
    Uniform<GLfloat>(prog, "material.shininess").Set(
        32
    );
    Uniform<glm::vec3>(prog, "material.specular").Set(
        glm::vec3(0.2, 0.95, 0.15)
    );
    Uniform<GLfloat>(prog, "material.shininessStrength").Set(
        0.065f
    );
    // Uniform<Vec3f>(prog, "LightPos").Set(Vec3f(0.0, 1.0f, 0.0));
    gl.ClearColor(0.8f, 0.8f, 0.7f, 0.0f);
    //gl.ClearDepth(1.0f);
    gl.Enable(Capability::DepthTest);
    gl.Enable(Capability::CullFace);
    gl.FrontFace(FaceOrientation::CW);
    gl.CullFace(Face::Back);
    //wireframe
    glPolygonMode(GL_FRONT, GL_LINE);
    glPolygonMode(GL_BACK, GL_LINE);
}

void Heightmap::display(double time)
{
    //Uniform<glm::vec3>(prog, "directionalLight.direction").Set(
    //    glm::vec3(
    //        std::cos(time / 1000),
    //        1.0,
    //        std::sin(time / 1000)
    //    )
    //);
    // clean color and depth buff
    gl.Clear().ColorBuffer().DepthBuffer();
    // set scene matrixes
    TransformationMatrices::Model(
        (glm::mat4)glm::scale(glm::mat4(1.0), glm::vec3(1.0, 1.0, 1.0))
    );
    TransformationMatrices::View(
        glm::lookAt(
            glm::vec3(std::cos(time * 0.25), 1, std::sin(time * 0.25)),
            glm::vec3(0, 0.0, 0),
            glm::vec3(0, 1, 0)
        )
    );
    Uniform<glm::mat4>(prog, "matrix.modelViewProjection").Set(
        TransformationMatrices::ModelViewProjection()
    );
    Uniform<glm::mat4>(prog, "matrix.modelView").Set(
        TransformationMatrices::ModelView()
    );
    Uniform<glm::mat4>(prog, "matrix.normal").Set(
        TransformationMatrices::Normal()
    );
    // draw mesh
    gl.DrawElements(PrimitiveType::TriangleStrip, indices.size(),
                    DataType::UnsignedInt);
}

void Heightmap::reshape(const unsigned int width, const unsigned int height)
{
    gl.Viewport(width, height);
    prog.Use();
    TransformationMatrices::Projection(
        glm::perspective(
            glm::radians(60.0f),
            (float)width / height,
            0.01f, 30.0f
        )
    );
    //Uniform<glm::mat4>(prog, "matrix.projection").Set(
    //    TransformationMatrices::Projection()
    //);
}
