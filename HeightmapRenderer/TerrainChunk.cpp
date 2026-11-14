#include "Commons.h"
#include "TerrainChunk.h"
#include "TransformationMatrices.h"
#include "App.h"

bool TerrainChunk::debugMode = false;
bool TerrainChunk::enableFrustumCulling = true;
BoundingBox * TerrainChunk::chunkBBox = nullptr;
ChunkDetailLevel * TerrainChunk::chunkLod = nullptr;

void TerrainChunk::bindBuffer(Program &program,
                              const ChunkDetailLevel::SideLevels &neighbours)
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
        chunkLod->bindBuffer(currentLoD, neighbours);
    }
}

void TerrainChunk::chooseLoDLevel(Camera &camera, const glm::vec3 & position)
{
    distanceToEye = glm::distance2(position, camera.Position());
    float C = getCameraConstant(camera);
    // the errors are measured on the unit mesh, scale them to world heights
    float heightScale = App::Instance()->getTerrain().HeightScale();
    // highest by default
    currentLoD = ChunkDetailLevel::High;

    for(int i = 0; i < 2; i++)
    {
        // geomipmapping, use level i + 1 once the eye is farther than D = d * C
        float d = heightChange[i] * heightScale;
        entropyDistances[i] = C * C * d * d;

        if(distanceToEye > entropyDistances[i])
            currentLoD = ChunkDetailLevel::LodLevel(i + 1);
    }
}

float TerrainChunk::getCameraConstant(Camera &camera)
{
    float A = camera.NearClip() / camera.Frustum().w;
    float T = (2.0f * chunkLod->Threeshold()) / camera.ScreenSize().y;
    return A / T;
}

void TerrainChunk::updateLoD()
{
    positionCS = glm::vec3(
                     (glm::vec4(this->center, 1.0f) * TransformationMatrices::Model())
                 );
    dimensionCS = glm::vec3(
                      (glm::vec4(this->dimension,
                                 1.0f) * TransformationMatrices::Model())
                  );
    // trapezoid - cube culling per chunk
    visible = !enableFrustumCulling || App::Instance()->getCamera()
              .isBoxInFrustum(positionCS, dimensionCS / 2.0f);
    // culled chunks still choose a level, their visible neighbours stitch to it
    chooseLoDLevel(App::Instance()->getCamera(), positionCS);
}

void TerrainChunk::drawElements(Program &program,
                                const ChunkDetailLevel::SideLevels &neighbours)
{
    if(!visible) return;

    // binds the chunk mesh data
    bindBuffer(program, neighbours);
    // draw primitives to gpu
    gl.DrawElements(
        PrimitiveType::TriangleStrip,
        chunkLod->indicesSize(currentLoD, neighbours),
        DataType::UnsignedInt
    );

    // changes the current program, draw bboxes around the chunk
    if(debugMode)
    {
        chunkBBox->render(positionCS, dimensionCS);
    }
}

float TerrainChunk::levelError(int level) const
{
    /************************************************************************/
    /*
    x---x---x             x-------x
    | \ | \ |             | \     |
    x---x---x  Breaks To  |   \   |
    | \ | \ |             |     \ |
    x---x---x             x-------x

    Every full resolution vertex is compared with the height of the coarse
    triangle it falls in, the triangle strips split each cell along the
    diagonal from the first row and column to the next ones
    */
    /************************************************************************/
    int size = chunkLod->ChunkSize();
    int step = 1 << level;
    auto height = [&](int row, int col) { return vertices[row * size + col].y; };
    float maxError = 0.0f;

    for(int row = 0; row < size; row++)
    {
        for(int col = 0; col < size; col++)
        {
            int row0 = std::min(row / step * step, size - 1 - step);
            int col0 = std::min(col / step * step, size - 1 - step);
            int row1 = row0 + step, col1 = col0 + step;
            float u = (float)(col - col0) / step;
            float v = (float)(row - row0) / step;
            float coarse = u >= v
                           ? height(row0, col0) + u * (height(row0, col1) - height(row0, col0))
                           + v * (height(row1, col1) - height(row0, col1))
                           : height(row0, col0) + v * (height(row1, col0) - height(row0, col0))
                           + u * (height(row1, col1) - height(row1, col0));
            maxError = std::max(maxError, std::abs(height(row, col) - coarse));
        }
    }

    return maxError;
}

TerrainChunk::TerrainChunk(std::vector<glm::vec3> & vertices,
                           std::vector<glm::vec3> & normals, std::vector<glm::vec2> & texCoords,
                           ChunkDetailLevel * chunkLod, float maxHeight, float minHeight)
{
    this->vertices = std::move(vertices);
    this->normals = std::move(normals);
    this->texCoords = std::move(texCoords);

    // only called once, chunk bbox, used for debug
    // only one created, then rendered per chunk translating and scaling it
    if(chunkBBox == nullptr) chunkBBox = new BoundingBox();

    // chunk lod controller, shared among all terrain chunks
    this->chunkLod = chunkLod;
    // get vertex matrix center
    int halfPoint = chunkLod->ChunkSize() / 2;
    this->center = this->vertices[halfPoint * chunkLod->ChunkSize() + halfPoint];
    this->center.y = (maxHeight + minHeight) / 2.0f;
    // set bounding box chunk data
    float chunkSpatialSize = (float)(chunkLod->ChunkSize() - 1.0f) /
                             (chunkLod->MeshSize() - 1.0f);
    this->dimension =
        glm::vec3(
            chunkSpatialSize,
            maxHeight - minHeight,
            chunkSpatialSize
        );
    // geometric errors per level, kept growing with coarser levels
    this->heightChange[0] = levelError(1);
    this->heightChange[1] = std::max(this->heightChange[0], levelError(2));
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

BoundingBox::BoundingBox() : bbox(1, 1, 1),
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
    prog.Use();
    gl.PolygonMode(Face::FrontAndBack, PolygonMode::Line);
    viewMatrix.Set(TransformationMatrices::View());
    modelMatrix.Set(
        glm::scale(
            glm::translate(
                glm::mat4(1.0f),
                position
            ),
            dimensions
        )
    );
    verts.Bind(Buffer::Target::Array);
    {
        (prog | 0).Setup<GLfloat>(3).Enable();
    }
    indices.Bind(Buffer::Target::ElementArray);
    bboxInstructions.Draw(bboxIndexArray);

    if(!App::Instance()->Gui().wireframeMode) gl.PolygonMode(Face::FrontAndBack,
                PolygonMode::Fill);
}
