#version 330

#define PI 3.14159265

const int MAX_POINT_LIGHTS = 2;
const int MAX_SPOT_LIGHTS = 2;

struct Attenuation
{
    float constant;
    float linear;
    float quadratic;
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
    float ambientCoefficient;
    uint spotLightCount;
    uint pointLightCount;
} light;

float lambertDiffuse(vec3 lightDirection, vec3 surfaceNormal)
{
    return max(0.0, dot(lightDirection, surfaceNormal));
}

float orenNayarDiffuse(vec3 lightDirection, vec3 viewDirection,
                       vec3 surfaceNormal, float roughness, float albedo)
{
    float LdotV = dot(lightDirection, viewDirection);
    float NdotL = dot(lightDirection, surfaceNormal);
    float NdotV = dot(surfaceNormal, viewDirection);
    float s = LdotV - NdotL * NdotV;
    float t = mix(1.0, max(NdotL, NdotV), step(0.0, s));
    float sigma2 = roughness * roughness;
    float A = 1.0 + sigma2 * (albedo / (sigma2 + 0.13) + 0.5 / (sigma2 + 0.33));
    float B = 0.45 * sigma2 / (sigma2 + 0.09);
    return albedo * max(0.0, NdotL) * (A + B * s / t) / PI;
}

float blinnPhongSpecular(vec3 lightDirection, vec3 viewDirection,
                         vec3 surfaceNormal, float shininess)
{
    //Calculate Blinn-Phong power
    vec3 halfVector = normalize(viewDirection + lightDirection);
    return pow(max(0.0, dot(surfaceNormal, halfVector)), shininess);
}

float phongSpecular(vec3 lightDirection, vec3 viewDirection, vec3 surfaceNormal,
                    float shininess)
{
    //Calculate Phong power
    vec3 R = -reflect(lightDirection, surfaceNormal);
    return pow(max(0.0, dot(viewDirection, R)), shininess);
}

vec3 calculatePointLight(PointLight lightSource, vec3 position,
                         vec3 surfaceNormal, vec3 surfaceColor, vec3 materialSpecular)
{
    // light direction from fragment to light
    vec3 lightDirection = lightSource.position - position;
    // distance from fragment to light position
    float lightDistance = length(lightDirection);
    lightDirection = normalize(lightDirection);
    // calculate attenuation factor
    float attenuationFactor = 1.0f / (1.0f + lightSource.attenuation.constant
                                      + lightSource.attenuation.linear * lightDistance
                                      + lightSource.attenuation.quadratic * lightDistance * lightDistance);
    // calculate lighting
    vec3 ambient = surfaceColor * light.ambientCoefficient *
                   lightSource.base.color;
    vec3 specular = vec3(0.0f);
    vec3 diffuse = vec3(0.0f);
    // calculate lambertian for diffuse factor
    float diffuseFactor = lambertDiffuse(lightDirection, surfaceNormal);

    if(diffuseFactor > 0.0f)
    {
        diffuse = lightSource.base.color * lightSource.base.intensity * surfaceColor *
                  diffuseFactor;
        // calculate blinn-phong specular
        vec3 viewDirection = normalize(-position);
        float specularFactor = blinnPhongSpecular(lightDirection, viewDirection,
                               surfaceNormal, 16);

        if(specularFactor > 0.0f)
        {
            specular = lightSource.base.color * lightSource.base.intensity *
                       materialSpecular * specularFactor;
        }
    }

    return ambient + (specular + diffuse) * attenuationFactor;
}

vec3 calculateSpotLight(SpotLight lightSource, vec3 position,
                        vec3 surfaceNormal, vec3 surfaceColor, vec3 materialSpecular)
{
    vec3 lightDirection = normalize(lightSource.base.position - position);
    vec3 spotDirection = normalize(lightSource.direction);
    float cosAngle = dot(-lightDirection, spotDirection);

    // do not calculate complete lighting outside the light frustum
    if(cosAngle <= lightSource.cosOuterAngle) return vec3(0.0);

    float cosInnerMinusOuter = lightSource.cosInnerAngle -
                               lightSource.cosOuterAngle;
    // final spot light factor smooth translation between outer angle and inner angle
    float spotLightFactor = smoothstep(0.0f, 1.0f,
                                       (cosAngle - lightSource.cosOuterAngle) / cosInnerMinusOuter);
    // same calculation as spotlight for cone values
    return calculatePointLight(lightSource.base, position, surfaceNormal,
                               surfaceColor, materialSpecular) * spotLightFactor;
}

vec3 calculateDirectionalLight(DirectionalLight lightSource, vec3 position,
                               vec3 surfaceNormal, vec3 surfaceColor, vec3 materialSpecular)
{
    // calculate lighting
    vec3 ambient = surfaceColor * light.ambientCoefficient *
                   lightSource.base.color;
    vec3 specular = vec3(0.0f);
    vec3 diffuse = vec3(0.0f);
    // calculate lambertian for diffuse factor
    float diffuseFactor = lambertDiffuse(lightSource.direction, surfaceNormal);

    if(diffuseFactor > 0.0f)
    {
        diffuse = lightSource.base.color * lightSource.base.intensity * surfaceColor *
                  diffuseFactor;
        // calculate blinn-phong specular
        vec3 viewDirection = normalize(-position);
        float specularFactor = blinnPhongSpecular(lightSource.direction, viewDirection,
                               surfaceNormal, 16);

        if(specularFactor > 0.0f)
        {
            specular = lightSource.base.color * lightSource.base.intensity *
                       materialSpecular * specularFactor;
        }
    }

    return ambient + (specular + diffuse);
}

void main()
{
}