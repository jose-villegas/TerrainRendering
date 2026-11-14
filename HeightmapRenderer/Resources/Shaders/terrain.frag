#version 330

#define PI 3.14159265

const int MAX_POINT_LIGHTS = 2;
const int MAX_SPOT_LIGHTS = 2;

// time of the day in [0, 1), the slice coordinate of the baked lightmaps
uniform float currentLightmap = 0.0f;
// scales the baked sky light and the baked bounced sun light
uniform float skyLightStrength = 0.2f;
uniform float bounceStrength = 1.0f;

// path traced lighting per time of the day, r: sun disk visibility,
// g: square root of the sun light bounced by the terrain
uniform sampler3D bakedLightmaps;
// path traced sky light with its bounces, [0, 1] maps to [0, skyLightRange]
uniform sampler2D bakedSkyLight;
const float skyLightRange = 1.25;
uniform vec2 terrainUVScaling = vec2(25, 25);

const int MAX_TERRAIN_TEXTURE_RANGES = 4;
uniform sampler2DArray terrainTextures;
uniform vec3 terrainRange[MAX_TERRAIN_TEXTURE_RANGES] =
{
    // min, max, active > 0.0
    vec3(0.0, 0.1, -1.0),
    vec3(0.1, 0.3, -1.0),
    vec3(0.3, 0.7, -1.0),
    vec3(0.7, 1.0, -1.0)
};

vec3 heightSample[MAX_TERRAIN_TEXTURE_RANGES] =
{
    vec3(0.1, 0.7, 1.0),
    vec3(0.7, 0.7, 0.5),
    vec3(0.3, 0.5, 0.1),
    vec3(1.0, 0.9, 0.9)
};

struct Attenuation
{
    float constant;
    float linear;
    float quadratic;
};

struct BaseLight
{
    vec3 intensities;
};

uniform struct DirectionalLight
{
    BaseLight base;
    vec3 direction;
} directionalLight;

uniform struct PointLight
{
    BaseLight base;
    vec3 position;
    Attenuation attenuation;
} pointLight[MAX_POINT_LIGHTS];

uniform struct SpotLight
{
    PointLight base;
    vec3 direction;
    float cosInnerAngle;
    float cosOuterAngle;
} spotLight[MAX_SPOT_LIGHTS];

uniform struct Material
{
    vec3 specular;
    vec3 diffuse;
    vec3 ambient;
    // material opacity 0..1
    float opacity;
    // phong shininess
    float shininess;
    // scales the specular color
    float shininessStrength;
} material;

uniform struct Matrices
{
    mat4 modelView;
    mat4 modelViewProjection;
    mat4 model;
    mat4 view;
    mat4 projection;
    mat4 normal;
} matrix;

uniform struct LightParams
{
    float ambientCoefficient;
    int spotLightCount;
    int pointLightCount;
} lightParams;

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
    vec3 ambient = surfaceColor * lightParams.ambientCoefficient *
                   lightSource.base.intensities;
    vec3 specular = vec3(0.0f);
    vec3 diffuse = vec3(0.0f);
    // calculate lambertian for diffuse factor
    float diffuseFactor = lambertDiffuse(lightDirection, surfaceNormal);

    if(diffuseFactor > 0.0f)
    {
        diffuse = lightSource.base.intensities * surfaceColor * diffuseFactor;
        // calculate blinn-phong specular
        vec3 viewDirection = normalize(-position);

        // specular scaling factor
        if(material.shininessStrength > 0.0f)
        {
            float specularFactor = blinnPhongSpecular(lightDirection, viewDirection,
                                   surfaceNormal, material.shininess);
            specular = lightSource.base.intensities * materialSpecular * specularFactor;
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
                               vec3 surfaceNormal, vec3 surfaceColor, vec3 materialSpecular,
                               float sunVisibility, float indirect)
{
    // baked sky and bounced light instead of a constant ambient term
    vec3 ambient = surfaceColor * indirect * lightSource.base.intensities;
    vec3 specular = vec3(0.0f);
    vec3 diffuse = vec3(0.0f);
    // calculate lambertian for diffuse factor
    float diffuseFactor = lambertDiffuse(lightSource.direction, surfaceNormal);

    if(diffuseFactor > 0.0f)
    {
        diffuse = lightSource.base.intensities * surfaceColor * diffuseFactor;
        // calculate blinn-phong specular
        vec3 viewDirection = normalize(-position);

        // specular scaling factor
        if(material.shininessStrength > 0.0f)
        {
            float specularFactor = blinnPhongSpecular(lightSource.direction, viewDirection,
                                   surfaceNormal, material.shininess);
            specular = lightSource.base.intensities * materialSpecular * specularFactor;
        }
    }

    return ambient + (specular + diffuse) * sunVisibility;
}

vec3 generateTerrainColor(float height, vec2 texCoord)
{
    vec3 terrainColor = vec3(0.0);
    vec3 terrainSurfaceColor = vec3(0.0);
    float regionMin = 0.0;
    float regionMax = 0.0;
    float regionRange = 0.0;
    float regionWeight = 0.0;
    vec2 terrainUV = texCoord * terrainUVScaling;

    for(int i = 0; i < MAX_TERRAIN_TEXTURE_RANGES; i++)
    {
        regionMin = terrainRange[i].x;
        regionMax = terrainRange[i].y;
        regionRange = regionMax - regionMin;
        // the lowest and highest regions keep full weight past their ends,
        // otherwise the deepest valleys and highest peaks fade to black
        float regionOffset = height - regionMax;

        if((i == 0 && regionOffset < 0.0)
           || (i == MAX_TERRAIN_TEXTURE_RANGES - 1 && regionOffset > 0.0))
        {
            regionOffset = 0.0;
        }

        regionWeight = max(0.0, (regionRange - abs(regionOffset)) / regionRange);
        terrainSurfaceColor = terrainRange[i].z > 0.0
                              ? texture(terrainTextures, vec3(terrainUV, i)).rgb
                              : heightSample[i];
        terrainColor += regionWeight * terrainSurfaceColor;
    }

    return terrainColor;
}

// Vertex shader inputs
in vec2 texCoord;
in vec3 normal;
in vec3 position;
in float height;

layout(location = 0) out vec4 fragColor;

void main()
{
    vec3 surfaceNormal = normalize(normal);
    vec3 surfaceColor = generateTerrainColor(height, texCoord);
    vec3 materialSpecular = material.specular * material.shininessStrength * height;
    // baked lighting at this time of the day
    vec2 baked = texture(bakedLightmaps, vec3(texCoord, currentLightmap)).rg;
    float sunVisibility = baked.r;
    // the bounce is baked for a unit sun, the direction length is its strength
    float bounce = baked.g * baked.g * length(directionalLight.direction);
    float sky = texture(bakedSkyLight, texCoord).r * skyLightRange;
    float indirect = sky * skyLightStrength + bounce * bounceStrength;
    // total light from all light sources
    vec3 totalLight = calculateDirectionalLight(directionalLight, position,
                      surfaceNormal, surfaceColor, materialSpecular,
                      sunVisibility, indirect);

    for(int i = 0; i < lightParams.pointLightCount; i++)
    {
        totalLight += calculatePointLight(pointLight[i], position, surfaceNormal,
                                          surfaceColor, materialSpecular);
    }

    for(int i = 0; i < lightParams.spotLightCount; i++)
    {
        totalLight += calculateSpotLight(spotLight[i], position, surfaceNormal,
                                         surfaceColor, materialSpecular);
    }

    // the colors stay in the texture space without gamma correction like
    // the original screenshot
    fragColor = vec4(totalLight, 1.0f);
}
