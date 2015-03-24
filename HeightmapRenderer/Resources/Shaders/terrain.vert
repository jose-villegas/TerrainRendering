#version 330

const int MAX_POINT_LIGHTS = 2;
const int MAX_SPOT_LIGHTS = 2;

struct Attenuation
{
    float constant;
    float linear;
    float exponential;
};

struct BaseLight
{
    vec3 color;
    float intensity;
};

struct DirectionalLight
{
    BaseLight base;
    vec3 direction;
};

struct PointLight
{
    BaseLight base;
    vec3 position;
    Attenuation attenuation;
};

struct SpotLight
{
    PointLight base;
    vec3 direction;
    float cosInnerAngle;
    float cosOuterAngle;
};

uniform struct Matrices 
{
    mat4 modelView;
    mat4 modelViewProjection;
    mat4 model;
    mat4 view;
    mat4 projection;
    mat4 normal;
} matrix;

uniform struct Lights 
{
    SpotLight spot[MAX_SPOT_LIGHTS];
    PointLight point[MAX_POINT_LIGHTS];
    DirectionalLight directional;
    uint spotLightCount;
    uint pointLightCount;
} light;

// Input vertex data
layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec2 vertexTexCoords;
layout(location = 2) in vec3 vertexNormal;

// Vertex shader output
out vec2 texCoord;
out vec3 normal;
out vec3 position;

void main()
{
    vec4 vertexPos = vec4(vertexPosition, 1.0f);

    texCoord = vertexTexCoords;
    normal = normalize(matrix.normal * vec4(vertexNormal, 0.0f)).xyz;
    position = vec3(matrix.modelView * vertexPos);

    gl_Position = matrix.modelViewProjection * vertexPos;
}