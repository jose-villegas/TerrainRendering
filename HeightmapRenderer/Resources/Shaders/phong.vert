#version 330

uniform mat4 ProjectionMatrix, CameraMatrix;
uniform vec3 LightPos;

layout(location = 0) in vec4 Position;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec3 TexCoords;

out vec3 vertNormal;
out vec3 vertViewDir;
out vec3 lightPos;
out float height;

void main(void)
{
    vertNormal = Normal;
    vertViewDir = (vec4(0.0, 0.0, 1.0, 1.0) * CameraMatrix).xyz;
    gl_Position = ProjectionMatrix * CameraMatrix * Position;
    lightPos = (ProjectionMatrix * CameraMatrix * vec4(LightPos, 1.0)).xyz;
    height = Position.y;
}