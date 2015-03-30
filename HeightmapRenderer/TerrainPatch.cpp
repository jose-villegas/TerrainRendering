#include "Commons.h"
#include "TerrainPatch.h"
#include "TransformationMatrices.h"

void TerrainPatch::loadMeshData()
{
    int originalPatchSize = this->patchSize;

    // empty previous data and reserve space
    for(int i = 0; i < LOD_LEVELS; i++)
    {
        this->indices[i].clear();
        this->vertices[i].clear();
        this->texCoords[i].clear();
        this->normals[i].clear();
        this->levelOfDetail = i;

        if(patchSize < 4)
        {
            lodLevelsAvailable = i;
            break;
        }

        this->vertices[i].resize(patchSize * patchSize);
        this->texCoords[i].resize(patchSize * patchSize);
        this->normals[i].resize(patchSize * patchSize);
        // write values to position collection
        writePositionsAndTexCoords();
        // calculate face normals
        std::vector<std::vector<glm::vec3>> faceNormals[2]; // 2 triangles per quad face
        writeFaceNormals(faceNormals);
        writeVertexNormals(faceNormals);
        writeIndices();
        // create all associated buffers
        {
            // upload position data to the gpu
            vertexBuffer[i].Bind(Buffer::Target::Array);
            {
                GLuint nPerVertex = vertices[i][0].length();
                Buffer::Data(Buffer::Target::Array, vertices[i]);
                // setup the vertex attribs array for the vertices
                (prog | 0).Setup<GLfloat>(nPerVertex).Enable();
            }
            normalBuffer[i].Bind(Buffer::Target::Array);
            {
                GLuint nPerVertex = normals[i][0].length();
                // upload the data
                Buffer::Data(Buffer::Target::Array, normals[i]);
                // setup the vertex attribs array for the vertices
                (prog | 1).Setup<GLfloat>(nPerVertex).Enable();
            }
            texCoordsBuffer[i].Bind(Buffer::Target::Array);
            {
                GLuint nPerVertex = texCoords[i][0].length();
                // upload the data
                Buffer::Data(Buffer::Target::Array, texCoords[i]);
                // setup the vertex attribs array for the vertices
                (prog | 2).Setup<GLfloat>(nPerVertex).Enable();
            }
            indexBuffer[i].Bind(Buffer::Target::ElementArray);
            {
                Buffer::Data(Buffer::Target::ElementArray, indices[i]);
                gl.Enable(Capability::PrimitiveRestart);
                gl.PrimitiveRestartIndex(patchSize * patchSize);
            }
        }
        // reduce patch size for different lod levels
        this->patchSize /= 2;
    }

    this->patchSize = originalPatchSize;
    this->changeLevelOfDetail(1);
}

void TerrainPatch::writeIndices()
{
    // create index buffer data
    int restartIndex = patchSize * patchSize;

    for(int i = 0; i < patchSize - 1; i++)
    {
        for(int j = 0; j < patchSize; j++)
        {
            indices[levelOfDetail].push_back((i + 1) * patchSize + j);
            indices[levelOfDetail].push_back(i * patchSize + j);
        }

        indices[levelOfDetail].push_back(restartIndex);
    }
}

void TerrainPatch::writeVertexNormals(std::vector < std::vector<glm::vec3> >
                                      faceNormals[2])
{
    // calculate per vertex (mesh) normals, averaging face normals
    for(int i = 0; i < patchSize; i++)
    {
        for(int j = 0; j < patchSize; j++)
        {
            glm::vec3 fNormal = glm::vec3(0.f, 0.f, 0.f);

            // upper left faces
            if(j != 0 && i != 0)
            {
                fNormal += faceNormals[0][i - 1][j - 1] + faceNormals[1][i - 1][j - 1];
            }

            // upper right faces
            if(i != 0 && j != patchSize - 1)
            {
                fNormal += faceNormals[0][i - 1][j];
            }

            // bottom right faces
            if(i != patchSize - 1 && j != patchSize - 1)
            {
                fNormal += faceNormals[0][i][j] + faceNormals[1][i][j];
            }

            // bottom left faces
            if(i != patchSize - 1 && j != 0)
            {
                fNormal += faceNormals[1][i][j - 1];
            }

            normals[levelOfDetail][i * patchSize + j] = glm::normalize(fNormal);
        }
    }
}

void TerrainPatch::writeFaceNormals(
    std::vector<std::vector<glm::vec3>> faceNormals[2])
{
    faceNormals[0] = faceNormals[1] = std::vector<std::vector<glm::vec3>>
                                      (patchSize - 1,
                                       std::vector<glm::vec3>(patchSize - 1));

    for(int i = 0; i < patchSize - 1; i++)
    {
        for(int j = 0; j < patchSize - 1; j++)
        {
            glm::vec3 triangle0[] =
            {
                vertices[levelOfDetail][i * patchSize + j],
                vertices[levelOfDetail][(i + 1) * patchSize + j],
                vertices[levelOfDetail][(i + 1) * patchSize + j + 1]
            };
            glm::vec3 triangle1[] =
            {
                vertices[levelOfDetail][(i + 1) * patchSize + j + 1],
                vertices[levelOfDetail][i * patchSize + j + 1],
                vertices[levelOfDetail][i * patchSize + j]
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

void TerrainPatch::writePositionsAndTexCoords()
{
    float textureU = (float)patchSize * 0.1f;
    float textureV = (float)patchSize * 0.1f;
    float maxValue = std::numeric_limits<float>::min();
    float minValue = std::numeric_limits<float>::max();

    for(int i = 0; i < patchSize; i++)
    {
        for(int j = 0; j < patchSize; j++)
        {
            float colScale = (float)j / (patchSize - 1);
            float rowScale = (float)i / (patchSize - 1);
            // same values for width and height, whole terrain
            int xCor = i * (float)Heigth() / patchSize;
            int yCor = j * (float)Width() / patchSize;
            float vertexHeight = Heightmap().GetValue(xCor, yCor);
            // assign respective mesh vertex values and texture coords per pixel
            vertices[levelOfDetail][i * patchSize + j] = glm::vec3(-0.5f + colScale,
                    (vertexHeight + 1.0f) / 2.0f,
                    -0.5f + rowScale
                                                                  );
            texCoords[levelOfDetail][i * patchSize + j] = glm::vec2(textureU * colScale,
                    textureV * rowScale);
        }
    }
}

TerrainPatch::TerrainPatch()
{
    // all lod levels
    lodLevelsAvailable = LOD_LEVELS - 1;
}

TerrainPatch::~TerrainPatch()
{
}

void TerrainPatch::createPatch(Program &prog, const float height,
                               const unsigned int patchSize /*= 512*/)
{
    if(patchSize <= 4 || !isPowerOfTwo(patchSize)) return;

    this->prog = prog;
    this->maxHeight = height;
    this->patchSize = patchSize;
    // create all the mesh data for this patch size
    loadMeshData();
}

void TerrainPatch::display()
{
    // set scene matrices uniforms
    Uniform<glm::mat4>(prog, "matrix.modelViewProjection").Set(
        TransformationMatrices::ModelViewProjection()
    );
    Uniform<glm::mat4>(prog, "matrix.modelView").Set(
        TransformationMatrices::ModelView()
    );
    Uniform<glm::mat4>(prog, "matrix.normal").Set(
        TransformationMatrices::Normal()
    );
    gl.DrawElements(PrimitiveType::TriangleStrip, indices[levelOfDetail].size(),
                    DataType::UnsignedInt);
}
void TerrainPatch::reshape(const unsigned int width, const unsigned int height)
{
}

void TerrainPatch::changeLevelOfDetail(const unsigned int level)
{
    // invalid value
    if(level < 0 || level > 5) return;

    this->levelOfDetail = std::min(lodLevelsAvailable, (int)(level - 1));
    int tPatchSize = (int)((double)this->patchSize / std::pow(2, levelOfDetail));
    // set the appropiate buffer for the active lod mesh
    vertexBuffer[levelOfDetail].Bind(Buffer::Target::Array);
    {
        GLuint nPerVertex = vertices[0][0].length();
        // setup the vertex attribs array for the vertices
        (prog | 0).Setup<GLfloat>(nPerVertex).Enable();
    }
    normalBuffer[levelOfDetail].Bind(Buffer::Target::Array);
    {
        GLuint nPerVertex = normals[0][0].length();
        // setup the vertex attribs array for the vertices
        (prog | 1).Setup<GLfloat>(nPerVertex).Enable();
    }
    texCoordsBuffer[levelOfDetail].Bind(Buffer::Target::Array);
    {
        GLuint nPerVertex = texCoords[0][0].length();
        // setup the vertex attribs array for the vertices
        (prog | 2).Setup<GLfloat>(nPerVertex).Enable();
    }
    indexBuffer[levelOfDetail].Bind(Buffer::Target::ElementArray);
    {
        gl.PrimitiveRestartIndex(tPatchSize * tPatchSize);
    }
}
