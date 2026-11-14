#version 330

// Adds the light shafts to the scene: upsamples the half resolution
// scattering with the nearest view distances, applies the sun phase
// function and colour, and attenuates the scene by the fog transmittance.

in vec2 uv;

uniform sampler2D sceneColor;
uniform sampler2D sceneDepth;
uniform sampler2D shafts;
uniform mat4 inverseViewProjection;
uniform vec3 cameraPosition;
// normalized, towards the sun
uniform vec3 sunDirection;
uniform vec3 sunColor;
uniform float intensity;

layout(location = 0) out vec4 fragColor;

const float SKY_DISTANCE = 60000.0;
// henyey-greenstein asymmetry, forward scattering around the sun
const float G = 0.6;

// relative to isotropic scattering
float phase(float cosTheta)
{
    float henyeyGreenstein = (1.0 - G * G)
                             / pow(1.0 + G * G - 2.0 * G * cosTheta, 1.5);
    return mix(1.0, henyeyGreenstein, 0.7);
}

void main()
{
    vec3 color = texture(sceneColor, uv).rgb;
    float depth = texture(sceneDepth, uv).r;
    vec4 world = inverseViewProjection * vec4(vec3(uv, depth) * 2.0 - 1.0, 1.0);
    vec3 ray = world.xyz / world.w - cameraPosition;
    float distance = depth >= 1.0 ? SKY_DISTANCE : length(ray);
    ray = normalize(ray);
    // joint bilateral upsample from the four nearest half resolution texels
    ivec2 size = textureSize(shafts, 0);
    vec2 position = uv * vec2(size) - 0.5;
    ivec2 base = ivec2(floor(position));
    vec2 f = position - vec2(base);
    vec2 sum = vec2(0.0);
    float weightSum = 0.0;
    vec2 bilinear = vec2(0.0);

    for(int y = 0; y <= 1; y++)
    {
        for(int x = 0; x <= 1; x++)
        {
            ivec2 texel = clamp(base + ivec2(x, y), ivec2(0), size - 1);
            vec4 tap = texelFetch(shafts, texel, 0);
            float w = (x == 0 ? 1.0 - f.x : f.x) * (y == 0 ? 1.0 - f.y : f.y);
            float difference = abs(tap.b - distance) / max(distance, 1e-3);
            float weight = w * exp(-difference * difference * 400.0) + 1e-5 * w;
            sum += tap.rg * weight;
            weightSum += weight;
            bilinear += tap.rg * w;
        }
    }

    vec2 shaft = weightSum > 1e-4 ? sum / weightSum : bilinear;
    vec3 inScattering = sunColor * intensity * phase(dot(ray, sunDirection))
                        * shaft.r;
    fragColor = vec4(color * shaft.g + inScattering, 1.0);
}
