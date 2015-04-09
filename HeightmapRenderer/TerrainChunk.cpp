#include "Commons.h"
#include "TerrainChunk.h"
#include "TransformationMatrices.h"
#include "App.h"

bool TerrainChunk::debugMode = false;
BoundingBox * TerrainChunk::chunkBBox = nullptr;
ChunkDetailLevel * TerrainChunk::chunkLod = nullptr;
// pixel error
float TerrainChunk::threeshold = 0.25;

void TerrainChunk::bindBuffer(Program &program)
{
    // upload position data to the gpu
    buffer[0].Bind(Buffer::Target::Array);
    {
        // vertices
        (program | 0).Setup<GLfloat>(3).Enable();
    }
    buffer[1].Bind(Buffer::Target::Array);
    {
        // normals
        (program | 1).Setup<GLfloat>(3).Enable();
    }
    buffer[2].Bind(Buffer::Target::Array);
    {
        // texture coords
        (program | 2).Setup<GLfloat>(2).Enable();
    }

    if(chunkLod)
    {
        chunkLod->bindBuffer(currentLoD);
    }
}

void TerrainChunk::chooseLoDLevel(Camera &camera)
{
    glm::vec3 position =
        glm::vec3(
            glm::vec4(this->center, 1.0f)
            * TransformationMatrices::ModelViewProjection()
        );
    distanceToEye = glm::distance2(position, camera.Position());
    float C = getCameraConstant(camera);
    // lowest level by defaul, higher distance
    currentLoD = 2;

    for(int i = 0; i < 2; i++)
    {
        entropyDistances[i] = C * C * heightChange[i] * heightChange[i];

        if(distanceToEye > entropyDistances[i]) currentLoD = i;
    }
}

float TerrainChunk::getCameraConstant(Camera &camera)
{
    float A = camera.NearClip() / camera.Frustum().w;
    float T = (2.0f * threeshold) / camera.ScreenSize().y;
    return A / T;
}

void TerrainChunk::drawElements(Program &program)
{
    // calculates distance to camera for lod selection
    chooseLoDLevel(App::Instance()->getCamera());
    // binds the chunk mesh data
    bindBuffer(program);
    // draw primitives to gpu
    gl.DrawElements(
        PrimitiveType::TriangleStrip,
        chunkLod->indicesSize(currentLoD),
        DataType::UnsignedInt
    );

    // changes the current program, draw bboxes around the chunk
    if(debugMode)
    {
        float hZScale =
            App::Instance()->getTerrain().TerrainHorizontalScale();
        float hScale =
            App::Instance()->getTerrain().HeightScale();
        chunkBBox->render(
            glm::vec3(position.x * hZScale, 0.0, position.y * hZScale),
            glm::vec3(
                (1.0f - 1.0f / (chunkLod->ChunkSize() - 1.0f)),
                dimension.y * hScale,
                (1.0f - 1.0f / (chunkLod->ChunkSize() - 1.0f))
            ) / glm::vec3(
                (chunkLod->MeshSize() - 1) / ((chunkLod->ChunkSize() - 1) *
                                              (chunkLod->ChunkSize() - 1)),
                1.0f,
                (chunkLod->MeshSize() - 1) / ((chunkLod->ChunkSize() - 1) *
                                              (chunkLod->ChunkSize() - 1))
            )
        );
    }
}

TerrainChunk::TerrainChunk(std::vector<glm::vec3> & vertices,
                           std::vector<glm::vec3> & normals, std::vector<glm::vec2> & texCoords,
                           ChunkDetailLevel * chunkLod, float maxHeight)
{
    this->vertices = std::move(vertices);
    this->normals = std::move(normals);
    this->texCoords = std::move(texCoords);
    // set chunk data
    this->position = glm::vec2(this->vertices[0].x, this->vertices[0].z);
    this->dimension = glm::vec3(1.0f, maxHeight, 1.0f);

    // only called once, chunk bbox, used for debug
    // only one created, then rendered per chunk translating and scaling it
    if(chunkBBox == nullptr) chunkBBox = new BoundingBox();

    // chunk lod controller, shared among all terrain chunks
    this->chunkLod = chunkLod;
    // get vertex matrix center
    int halfPoint = chunkLod->ChunkSize() / 2;
    this->center = this->vertices[halfPoint * chunkLod->ChunkSize() + halfPoint];

    // slow calculation for entropies
    for(int i = 0; i < 2; i++)
    {
        int currentLoD = (int)std::pow(2, i);
        // calculate current entropy
        glm::vec3 position = glm::vec3();
        glm::vec3 direction = glm::vec3();
        float entropy = 0.0f;
        std::vector<glm::vec3> lVertices;
        std::vector<int> stopLerpToken;

        // extract lower lod vertices
        for each(int index in chunkLod->IndicesLoD()[i + 1])
        {
            index != chunkLod->RestartIndexToken()
            ? lVertices.push_back(this->vertices[index])
            , stopLerpToken.push_back(-1)
            : stopLerpToken.back() = lVertices.size() - 1;
        }

        // calculating the height difference is easy considering
        // the way vertex indexes are ordered, leaving the breaking
        // point of the higher lod just in the middle of the low lod
        // triangle strip lines
        std::vector<float> lowHeight;

        for(int j = 0; j < lVertices.size() - 1; j++)
        {
            if(stopLerpToken[j] == j) continue;

            // calculate the mid point height between vertex lines at this
            // point is where the geomipmap algorithm breaks the higher mesh
            lowHeight.push_back(glm::lerp(lVertices[j], lVertices[j + 1], 0.5f).y);
        }

        int nextSize = (chunkLod->ChunkSize() - 1) / std::pow(2, i) + 1;
        int stepper = 0; int stepMultiplier = std::pow(2, i);
        std::vector<float> realHeight;

        for(int y = 0; y < nextSize - 1; y = stepper * 2)
        {
            stepper++;

            for(int x = 0; x < nextSize; x++)
            {
                int vertIndex = ((y + 1) * chunkLod->ChunkSize() + x) * stepMultiplier;
                realHeight.push_back(this->vertices[vertIndex].y);
            }
        }

        float maxEntropy = 0.0f;

        for(int j = 0; j < realHeight.size(); j++)
        {
            maxEntropy = std::max(std::abs(realHeight[i] - lowHeight[i]), maxEntropy);
        }

        this->heightChange[i] = maxEntropy;
    }
}

void TerrainChunk::bindBufferData(Program &program)
{
    // upload position data to the gpu
    buffer[0].Bind(Buffer::Target::Array);
    {
        Buffer::Data(Buffer::Target::Array, vertices);
        // vertices
        (program | 0).Setup<GLfloat>(3).Enable();
    }
    buffer[1].Bind(Buffer::Target::Array);
    {
        // upload the data
        Buffer::Data(Buffer::Target::Array, normals);
        // normals
        (program | 1).Setup<GLfloat>(3).Enable();
    }
    buffer[2].Bind(Buffer::Target::Array);
    {
        // upload the data
        Buffer::Data(Buffer::Target::Array, texCoords);
        // texture coords
        (program | 2).Setup<GLfloat>(2).Enable();
    }
    // free memory once uploaded to gpu
    this->vertices.clear();
    this->normals.clear();
    this->texCoords.clear();
}

BoundingBox::BoundingBox() : bbox(1, 1, 1, 0, 0.5, 0.0),
    bboxInstructions(bbox.Instructions()),
    bboxIndexArray(bbox.Indices()), projectionMatrix(prog), viewMatrix(prog),
    modelMatrix(prog)
{
    // Set the vertex shader source and compile it
    vs.Source((GLSLString)
              "#version 330\n"
              "uniform mat4 ProjectionMatrix, CameraMatrix, ModelMatrix;"
              "in vec4 Position;"
              "void main(void)"
              "{"
              "	gl_Position = "
              "		ProjectionMatrix *"
              "		CameraMatrix *"
              "		ModelMatrix *"
              "		Position;"
              "}"
             ).Compile();
    // set the fragment shader source and compile it
    fs.Source((GLSLString)
              "#version 330\n"
              "out vec4 fragColor;"
              "void main(void)"
              "{"
              "	fragColor = vec4(1.0f);"
              "}"
             ).Compile();
    // attach the shaders to the program
    prog.AttachShader(vs).AttachShader(fs);
    // link and use it
    prog.Link().Use();
    // initialize the uniforms
    projectionMatrix.BindTo("ProjectionMatrix");
    viewMatrix.BindTo("CameraMatrix");
    modelMatrix.BindTo("ModelMatrix");
    // bind the VBO for the cube vertices
    verts.Bind(Buffer::Target::Array);
    {
        std::vector<GLfloat> data;
        GLuint n_per_vertex = bbox.Positions(data);
        // upload the data
        Buffer::Data(Buffer::Target::Array, data);
        // setup the vertex attribs array for the vertices
        VertexArrayAttrib attr(prog, "Position");
        attr.Setup<GLfloat>(n_per_vertex).Enable();
    }
    indices.Bind(Buffer::Target::ElementArray);
    {
        Buffer::Data(Buffer::Target::ElementArray, bboxIndexArray);
    }
    projectionMatrix.Set(
        TransformationMatrices::Projection()
    );
}

void BoundingBox::render(glm::vec3 position, glm::vec3 dimensions)
{
    gl.Enable(Capability::DepthTest);
    gl.Disable(Capability::CullFace);
    prog.Use();
    gl.PolygonMode(Face::FrontAndBack, PolygonMode::Line);
    viewMatrix.Set(TransformationMatrices::View());
    modelMatrix.Set(
        glm::translate(
            glm::scale(
                glm::translate(
                    glm::mat4(1.0f),
                    glm::vec3(position.x - 0.5, -0.5, position.z - 0.5)
                    + (1.0f - glm::vec3(dimensions.x, 0.0f, dimensions.z)) / 2.0f
                ),
                dimensions
            ),
            glm::vec3(1.0, 0.0, 1.0)
        )
    );
    verts.Bind(Buffer::Target::Array);
    {
        (prog | 0).Setup<GLfloat>(3).Enable();
    }
    indices.Bind(Buffer::Target::ElementArray);
    bboxInstructions.Draw(bboxIndexArray);
    gl.PolygonMode(Face::FrontAndBack, PolygonMode::Fill);
}
