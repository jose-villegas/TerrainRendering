#include "Commons.h"
#include "Heightmap.h"

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

    for(int i = 0; i < rows; i++)
    {
        for(int j = 0; j < cols; j++)
        {
            float colScale = (float)j / (cols - 1);
            float rowScale = (float)i / (rows - 1);
            float vertexHeight = (std::sin((i * M_PI) / rows)
                                  + std::sin((j * M_PI) / cols)) / 4;
            // assign respective mesh vertex values and texture coords per pixel
            vertices[i * cols + j] = glm::vec3(-0.5f + colScale, vertexHeight,
                                               -0.5f + rowScale);
            texCoords[i * cols + j] = glm::vec2(textureU * colScale, textureV * rowScale);
        }
    }
}

Heightmap::Heightmap()
{
    vs.Source(GLSLSource::FromFile("Resources/Shaders/phong.vert"));
    // compile it
    vs.Compile();
    // set the fragment shader source
    fs.Source(GLSLSource::FromFile("Resources/Shaders/phong.frag"));
    // compile it
    fs.Compile();
    // attach the shaders to the program
    prog.AttachShader(vs);
    prog.AttachShader(fs);
    // link and use it
    prog.Link();
    prog.Use();
    // testing
    VertexShader vss;
    FragmentShader fss;
    Program pp;
    vss.Source(GLSLSource::FromFile("Resources/Shaders/terrain.vert"));
    vss.Compile();
    fss.Source(GLSLSource::FromFile("Resources/Shaders/terrain.frag"));
    fss.Compile();
    pp.AttachShader(vss);
    pp.AttachShader(fss);
    pp.Link();
}


Heightmap::~Heightmap()
{
}

void Heightmap::loadFromFile(const std::string &srFilename)
{
    rows = 512;
    cols = 512;
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
    Uniform<Vec3f>(prog, "LightPos").Set(Vec3f(0.0, 1.0f, 0.0));
    //
    gl.ClearColor(0.8f, 0.8f, 0.7f, 0.0f);
    //gl.ClearDepth(1.0f);
    //gl.Enable(Capability::DepthTest);
    //gl.Enable(Capability::CullFace);
    //gl.FrontFace(FaceOrientation::CCW);
    //gl.CullFace(Face::Back);
    //wireframe
    glPolygonMode(GL_FRONT, GL_LINE);
    glPolygonMode(GL_BACK, GL_LINE);
}

void Heightmap::display(double time)
{
    gl.Clear().ColorBuffer().DepthBuffer();
    // Uniform<Vec3f>(prog, "LightPos").Set(Vec3f(0.0, 0.5f, std::sin(time)));
    // set the matrix for camera orbiting the origin
    Uniform<Mat4f>(prog, "CameraMatrix").Set(
        //CamMatrixf::Orbiting(
        //    Vec3f(),
        //    0.75,
        //    FullCircles(time / 17.0),
        //    Degrees(SineWave(time / 19.0) * 90)
        //)
        CamMatrixf::LookingAt(
            Vec3f(0.0, 1.0 / 1.0, 1.0 / 1.0),
            Vec3f(),
            Vec3f(0, 1, 0)
        )
    );
    //Uniform<Mat4f>(prog, "CameraMatrix")
    //.Set
    //(
    //    CamMatrixf::LookingAt
    //    (
    //        Vec3f(0, 0, 1),
    //        Vec3f(0),
    //        Vec3f(0, 1, 0)
    //    )
    //);
    gl.DrawElements(PrimitiveType::TriangleStrip, indices.size(),
                    DataType::UnsignedInt);
}

void Heightmap::reshape(const unsigned int width, const unsigned int height)
{
    gl.Viewport(width, height);
    prog.Use();
    Uniform<Mat4f>(prog, "ProjectionMatrix").Set(
        CamMatrixf::PerspectiveX(
            Degrees(60),
            double(width) / height,
            0.01, 30
        )
    );
}
