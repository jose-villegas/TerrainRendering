#include "Commons.h"
#include "Heightmap.h"

Heightmap::Heightmap()
{
    vs.Source((GLSLString)
              "");
    vs.Compile();
    fs.Source((GLSLString)
              "#version 330\n"
              "out vec3 fragColor; "
              "void main(void)"
              " {fragColor = vec3(0, 1, 0);}");
    fs.Compile();
    prog.AttachShader(vs);
    prog.AttachShader(fs);
    prog.Link();
    prog.Use();
}


Heightmap::~Heightmap()
{
}

bool Heightmap::loadFromFile(const std::string &srFilename)
{
    int rows = 512;
    int cols = 512;
    // load mesh position data for vertex buffer object
    std::vector<std::vector<glm::vec3>> positionData(rows,
                                     std::vector<glm::vec3>(cols));
    std::vector<std::vector<glm::vec2>> coordsData(rows,
                                     std::vector<glm::vec2>(cols));
    float textureU = (float)cols * 0.1f;
    float textureV = (float)rows * 0.1f;

    for(int i = 0; i < rows; i++)
    {
        for(int j = 0; j < cols; j++)
        {
            float colScale = (float)j / (cols - 1);
            float rowScale = (float)i / (rows - 1);
            float vertexHeight = (float)(std::pow(std::sin(i), 2.0f)
                                         + std::pow(std::cos(j), 2.0f));
            // assign respective mesh vertex values and texture coords per pixel
            positionData[i][j] = glm::vec3(-0.5f + colScale, vertexHeight,
                                           -0.5f + rowScale);
            coordsData[i][j] = glm::vec2(textureU * colScale, textureV * rowScale);
        }
    }

    // calculate face normals
    std::vector<std::vector<glm::vec3>> faceNormals[2]; // 2 triangles per quad face
    faceNormals[0] = faceNormals[1] = std::vector<std::vector<glm::vec3>>(rows - 1,
                                      std::vector<glm::vec3>(cols - 1));

    for(int i = 0; i < rows - 1; i++)
    {
        for(int j = 0; j < cols - 1; j++)
        {
            glm::vec3 triangle0[] =
            {
                positionData[i][j],
                positionData[i + 1][j],
                positionData[i + 1][j + 1]
            };
            glm::vec3 triangle1[] =
            {
                positionData[i + 1][j + 1],
                positionData[i][j + 1],
                positionData[i][j]
            };
            glm::vec3 t0Normal = glm::cross(triangle0[0] - triangle0[1],
                                            triangle0[1] - triangle0[2]);
            glm::vec3 t1Normal = glm::cross(triangle1[0] - triangle1[1],
                                            triangle1[1] - triangle1[2]);
            faceNormals[0][i][j] = glm::normalize(t0Normal);
            faceNormals[1][i][j] = glm::normalize(t1Normal);
        }
    }

    // now we calculate the vertex normals averaging the face normals
    std::vector<std::vector<glm::vec3>> vertexNormals(rows,
                                     std::vector<glm::vec3>(cols));

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

            vertexNormals[i][j] = glm::normalize(fNormal);
        }
    }

    // join all the vertex data, bind and feed
    // the vbo with all the vertex data
    std::vector<Vertex> fvertexData;

    for(int i = 0; i < rows; i++)
    {
        for(int j = 0; j < cols; j++)
        {
            fvertexData.push_back(Vertex(positionData[i][j], coordsData[i][j],
                                         vertexNormals[i][j]));
        }
    }

    this->vertexBuffer.Bind(Buffer::Target::Array);
    Buffer::Data(Buffer::Target::Array, fvertexData.size(), &fvertexData[0]);
    // bind to shader program
    (prog | 0).Pointer(3, DataType::Float, false, sizeof(Vertex), 0);
    (prog | 1).Pointer(2, DataType::Float, false, sizeof(Vertex),
                       (const GLvoid *)12);
    (prog | 2).Pointer(2, DataType::Float, false, sizeof(Vertex),
                       (const GLvoid *)20);
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

    this->indexBuffer.Bind(Buffer::Target::ElementArray);
    Buffer::Data(Buffer::Target::ElementArray, indices.size(), &indices[0]);
    gl.ClearColor(0.0, 0.0, 0.0, 0.0);
    return true;
}

void Heightmap::display(int width, int height)
{
    gl.Viewport(0, 0, width, height);
    gl.Clear().ColorBuffer();
    this->indexBuffer.Bind(Buffer::Target::ElementArray);
    gl.DrawElements(PrimitiveType::Triangles, indices.size(),
                    DataType::UnsignedInt);
}
