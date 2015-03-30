#include "Commons.h"
#include "Terrain.h"
#include "TransformationMatrices.h"

void Terrain::display()
{
    terrain[0][0].changeLevelOfDetail(1);
    //TransformationMatrices::Model(glm::translate(glm::mat4(1),
    //                              glm::vec3(0, 0.0, i)));
    terrain[0][0].display();
}

Terrain::Terrain() : terrainMaxHeight(0.6f)
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
    // set whole terrain size
    TerrainGenerator::setSize(512, 512);
    // gen test terraim
    terrain[0][0].setBounds(0, 0 + 8, 0, 0 + 8);
    terrain[0][0].build();
    terrain[0][0].createPatch(program, terrainMaxHeight, 8);
}


Terrain::~Terrain()
{
}
