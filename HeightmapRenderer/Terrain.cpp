#include "Commons.h"
#include "Terrain.h"
#include "TransformationMatrices.h"

int left = -1;
int top = 2;

void Terrain::display()
{
    for(int i = left; i < top; i++)
    {
        int detail = 1;

        for(int j = left; j < top; j++)
        {
            terrain[i][j].changeLevelOfDetail(detail++);
            TransformationMatrices::Model(glm::translate(glm::mat4(1),
                                          glm::vec3((float)j * 1.0, 0.0f, (float)i * 1.0)));
            terrain[i][j].display();
        }
    }
}

Terrain::Terrain() : terrainMaxHeight(1.0f)
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
        0.065f
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
    //wireframe
    glPolygonMode(GL_FRONT, GL_LINE);
    glPolygonMode(GL_BACK, GL_LINE);
    // detail level, vertex cout = detail * detail
    int detailLevel = 3;
    float increment = 0.25;
    float sqDetailLevel = (float)(detailLevel * detailLevel);
    float boundStep = increment * (sqDetailLevel - 1.0f) / sqDetailLevel;
    // set whole terrain size
    TerrainGenerator::setSize(513, 513);

    // gen test terraim
    for(int i = left; i < top; i++)
    {
        for(int j = left; j < top; j++)
        {
            terrain[i][j].setBounds(boundStep * i,
                                    boundStep * i + increment,
                                    boundStep * j,
                                    boundStep * j + increment);
            terrain[i][j].build();
            terrain[i][j].createPatch(program, terrainMaxHeight, detailLevel);
        }
    }
}

void Terrain::test(int cc)
{
    terrain[0][1].setBounds(0, 0 + 0.5, (float)cc / 128, (float)cc / 128 + 0.5);
    terrain[0][1].build();
    terrain[0][1].createPatch(program, terrainMaxHeight, 64);
}

Terrain::~Terrain()
{
}
