#include "Commons.h"
#include "TerrainPatch.h"
#include "TransformationMatrices.h"

void TerrainPatch::loadLodMesh(int i)
{
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
            gl.PrimitiveRestartIndex(vertices->size());
        }
    }
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

            float h0 = getValue(j + 1, i);
            float h1 = getValue(j - 1, i);
            float h2 = getValue(j, i + 1);
            float h3 = getValue(j, i + 1);
            //normals[levelOfDetail].push_back(
            //    glm::normalize(fNormal)
            //);
            normals[levelOfDetail].push_back(
                glm::normalize(glm::vec3(h1 - h0, 2, h3 - h2))
            );
        }
    }
}

void TerrainPatch::writeFaceNormals(
    std::vector<std::vector<glm::vec3>> faceNormals[2])
{
    faceNormals[0] = faceNormals[1] = std::vector<std::vector<glm::vec3>>
                                      (patchSize - 1, std::vector<glm::vec3>(patchSize - 1));

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
            float vertexHeight = getValue(xCor, yCor) * maxHeight;
            // get maximum and minimum
            maxValue = vertexHeight > maxValue ? vertexHeight : maxValue;
            minValue = vertexHeight > minValue ? vertexHeight : minValue;
            // assign respective mesh vertex values and texture coords per pixel
            vertices[levelOfDetail].push_back(
                glm::vec3(
                    -0.5f + colScale,
                    vertexHeight,
                    -0.5f + rowScale
                )
            );
            texCoords[levelOfDetail].push_back(
                glm::vec2(textureU * colScale, textureV * rowScale)
            );
        }
    }
}

TerrainPatch::TerrainPatch()
{
    // all lod levels
    lodLevelsAvailable = LOD_LEVELS - 1;
    patchSize = 0;
    maxHeight = 1.0f;
    this->loaded = false;
}

TerrainPatch::~TerrainPatch()
{
}

std::vector<glm::vec3> & TerrainPatch::getVertices(const float height,
        const unsigned int patchSize, const unsigned int lodLevel)
{
    // invalid value
    if(lodLevel < 0 || lodLevel > LOD_LEVELS || patchSize > 256)
    {
        return std::vector<glm::vec3>();
    }

    this->patchSize = patchSize;
    this->maxHeight = height;
    this->levelOfDetail = lodLevel;
    int tPatchSize = (int)((double)this->patchSize / std::pow(2, levelOfDetail));
    // load vertices for this level of detail
    std::vector<glm::vec3> resultVertices;

    for(int i = 0; i < tPatchSize; i++)
    {
        for(int j = 0; j < tPatchSize; j++)
        {
            float colScale = (float)j / (tPatchSize - 1);
            float rowScale = (float)i / (tPatchSize - 1);
            // same values for width and height, whole terrain
            int xCor = i * (float)Heigth() / tPatchSize;
            int yCor = j * (float)Width() / tPatchSize;
            float vertexHeight = getValue(xCor, yCor) * maxHeight;
            // assign respective mesh vertex values and texture coords per pixel
            resultVertices.push_back(
                glm::vec3(
                    -0.5f + colScale,
                    (vertexHeight + 1.0f) / 2.0f,
                    -0.5f + rowScale
                )
            );
        }
    }

    return resultVertices;
}

void TerrainPatch::createPatch(Program &prog, const float height,
                               const unsigned int lodExponent)
{
    if(lodExponent > 256) return;

    this->prog = prog;
    this->maxHeight = height;
    this->patchSize = std::pow(2, lodExponent) + 1;
    // create all the mesh data for this patch size
    int originalPatchSize = this->patchSize;
    int currentExponent = lodExponent;

    for(int i = 0; i < LOD_LEVELS; i++)
    {
        // empty previous data
        this->indices[i].clear();
        this->vertices[i].clear();
        this->texCoords[i].clear();
        this->normals[i].clear();
        this->levelOfDetail = i;

        if(this->patchSize < 2)
        {
            lodLevelsAvailable = i - 1;
            break;
        }

        loadLodMesh(i);
        // reduce patch size for different lod levels
        this->patchSize = std::pow(2, --currentExponent) + 1;
        // this->patchSize = (float)(this->patchSize) / 2.0;
    }

    this->patchSize = originalPatchSize;
    this->changeLevelOfDetail(1);
    this->loaded = true;
}

//void TerrainPatch::createPatch(Program &prog, const float height,
//                               const unsigned int patchSize, const float increment,
//                               const glm::vec4 & increments, const float bottomLeft, const float topLeft,
//                               const float bottomRight, const float topRigth)
//{
//    // patchSize is 2^n + 1
//    if( || patchSize > 256 || !isPowerOfTwo(patchSize - 1)) return;
//
//    this->prog = prog;
//    this->maxHeight = height;
//    this->patchSize = patchSize;
//    // create all the mesh data for this patch size
//    int originalPatchSize = this->patchSize;
//
//    for(int i = 0; i < LOD_LEVELS; i++)
//    {
//        float cBoundStep = increment *
//                           ((float)this->patchSize - 1.0f) / (float)this->patchSize;
//        glm::vec4 boundsVec = increments * cBoundStep;
//        this->setBounds(bottomLeft + boundsVec.x,
//                        topLeft + boundsVec.y,
//                        bottomRight + boundsVec.z,
//                        topRigth + boundsVec.w);
//        this->build();
//        // empty previous data
//        this->indices[i].clear();
//        this->vertices[i].clear();
//        this->texCoords[i].clear();
//        this->normals[i].clear();
//        this->levelOfDetail = i;
//
//        if(patchSize < 4)
//        {
//            lodLevelsAvailable = i;
//            return;
//        }
//
//        loadLodMesh(i);
//        // reduce patch size for different lod levels
//        this->patchSize /= 2;
//    }
//
//    this->patchSize = originalPatchSize;
//    this->changeLevelOfDetail(1);
//    this->loaded = true;
//}

void TerrainPatch::display()
{
    if(!loaded) return;

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
                    DataType::UnsignedShort);
}

void TerrainPatch::changeLevelOfDetail(const unsigned int level)
{
    // invalid value
    if(!loaded) return;

    this->levelOfDetail =
        std::max(0, std::min(lodLevelsAvailable, (int)(level - 1)));
    //int tPatchSize = (int)((double)this->patchSize / std::pow(2, levelOfDetail));
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
        gl.PrimitiveRestartIndex(vertices[levelOfDetail].size());
    }
}
