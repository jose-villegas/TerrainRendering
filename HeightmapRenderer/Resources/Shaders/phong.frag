#version 330

in vec3 vertNormal;
in vec3 vertViewDir;

out vec4 fragColor;
uniform vec3 LightPos[3];

void main(void)
{
    vec3 normal = normalize(vertNormal);
    float amb = 0.2;
    float diff = 0.0;
    float spec = 0.0;

    for(int i = 0; i != 3; ++i)
    {
        diff += max(dot(normal,  LightPos[i]) / dot(LightPos[i], LightPos[i]), 0.0);
        float k = dot(normal, LightPos[i]);
        vec3 r = 2.0 * k * normal - LightPos[i];
        spec += pow(max(dot(normalize(r), vertViewDir), 0.0), 32.0 * dot(r, r));
    }

    fragColor = vec4(vec3(0.8, 0.1, 0.2), 1.0) * (amb + diff) + vec4(1.0, 1.0, 1.0,
                1.0) * spec;
}