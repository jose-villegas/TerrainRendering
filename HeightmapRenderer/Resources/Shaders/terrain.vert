#version 330

uniform struct Matrices 
{
    mat4 modelView;
    mat4 modelViewProjection;
    mat4 model;
    mat4 view;
    mat4 projection;
    mat4 normal;
} matrix;

uniform sampler3D bakedLightmaps;
uniform sampler2D realTimeLightmap;
uniform sampler2D heightmapField;
uniform vec2 terrainUVScaling = vec2(25, 25);
uniform vec2 terrainMapSize = vec2(256, 256);

// Input vertex data
layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec2 vertexTexCoords;

// Vertex shader output
out vec2 texCoord;
out vec3 normal;
out vec3 position;
out float height;

void main()
{
    vec4 vertexPos = vec4(vertexPosition, 1.0f);

    height = vertexPosition.y;

    texCoord = vertexTexCoords;
    normal = normalize(matrix.normal * vec4(vertexNormal, 0.0f)).xyz;
    position = vec3(matrix.modelView * vertexPos);

    gl_Position = matrix.modelViewProjection * vertexPos;
}