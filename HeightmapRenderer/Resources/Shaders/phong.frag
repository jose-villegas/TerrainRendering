#version 330

in vec3 vertNormal;
in vec3 vertViewDir;
in vec3 lightPos;
in float height;

out vec4 fragColor;

float blinnPhongSpecular(vec3 lightDirection, vec3 viewDirection,
                         vec3 surfaceNormal, float shininess)
{
    //Calculate Blinn-Phong power
    vec3 H = normalize(viewDirection + lightDirection);
    return pow(max(0.0, dot(surfaceNormal, H)), shininess);
}

float lambert(vec3 surfaceNormal, vec3 lightDirection)
{
    return max(dot(surfaceNormal, lightDirection), 0.0f);
}

void main(void)
{
    vec3 matColor = vec3(log(height + 1), 0.1, 0.1);
    vec3 normal = normalize(vertNormal);
    float amb = 0.2;
    float diff = 0.0;
    float spec = 0.0;
    // calculate shading
    diff += lambert(normal, lightPos);

    if(diff > 0.0f)
    {
        spec += blinnPhongSpecular(lightPos, vertViewDir, normal, 16.0f);
    }

    fragColor = vec4(matColor, 1.0) * ((diff + spec) + amb);
}