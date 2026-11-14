#version 330

// Marches every view ray from the camera to the scene depth through a height
// fog over the terrain, and sums the sunlight scattered towards the camera
// where the sun is not blocked by the terrain (the shadow height texture).
// Runs at half resolution, output: scattering, transmittance, view distance.

in vec2 uv;

uniform sampler2D sceneDepth;
uniform sampler2D shadowHeight;
uniform mat4 inverseViewProjection;
uniform vec3 cameraPosition;
// world bounds of the fog: the terrain footprint, from the ground up
uniform vec3 boxMin;
uniform vec3 boxMax;
// terrain world scale: horizontal, vertical
uniform vec2 terrainScale;
// extinction per world unit at the ground
uniform float density;
// fog density falls by exp(-heightFalloff) at the terrain's top height
uniform float heightFalloff;

layout(location = 0) out vec4 result;

const int STEPS = 64;
// stands for the sky, kept within half float range
const float SKY_DISTANCE = 60000.0;
// width of the fog fade at the terrain borders, relative to the terrain
const float EDGE_FADE = 0.15;

// interleaved gradient noise, offsets the samples per pixel against banding
float stepOffset(vec2 pixel)
{
    return fract(52.9829189 * fract(dot(pixel, vec2(0.06711056, 0.00583715))));
}

void main()
{
    float depth = texture(sceneDepth, uv).r;
    vec4 world = inverseViewProjection * vec4(vec3(uv, depth) * 2.0 - 1.0, 1.0);
    vec3 ray = world.xyz / world.w - cameraPosition;
    float sceneDistance = length(ray);
    ray /= sceneDistance;

    if(depth >= 1.0) sceneDistance = SKY_DISTANCE;

    // ray against the fog box
    vec3 safeRay = mix(ray, vec3(1e-6), lessThan(abs(ray), vec3(1e-6)));
    vec3 t0 = (boxMin - cameraPosition) / safeRay;
    vec3 t1 = (boxMax - cameraPosition) / safeRay;
    vec3 tMin = min(t0, t1);
    vec3 tMax = max(t0, t1);
    float tNear = max(max(tMin.x, tMin.y), max(tMin.z, 0.0));
    float tFar = min(min(tMax.x, tMax.y), min(tMax.z, sceneDistance));
    float scattering = 0.0;
    float transmittance = 1.0;

    if(tFar > tNear)
    {
        float size = float(textureSize(shadowHeight, 0).x);
        float last = size - 1.0;
        float stepLength = (tFar - tNear) / float(STEPS);
        float t = tNear + stepLength * stepOffset(gl_FragCoord.xy);

        for(int i = 0; i < STEPS; i++)
        {
            vec3 position = cameraPosition + ray * t;
            float height = position.y / terrainScale.y;
            // the fog thins out towards the box sides and top, where the
            // terrain ends, so the box itself does not show
            vec2 side = min(position.xz - boxMin.xz, boxMax.xz - position.xz);
            float edgeFade = smoothstep(0.0, EDGE_FADE * terrainScale.x,
                                        min(side.x, side.y));
            float topFade = 1.0 - smoothstep(0.6, 1.0, (position.y - boxMin.y)
                                             / (boxMax.y - boxMin.y));
            float sigma = density * exp(-heightFalloff * max(height, 0.0))
                          * edgeFade * topFade;
            // terrain u, v to the texel of the vertex at that position
            vec2 terrainUV = position.xz / terrainScale.x + 0.5;
            float shadow = texture(shadowHeight,
                                   (terrainUV * last + 0.5) / size).r;
            float sunVisibility = smoothstep(shadow - 0.004, shadow + 0.004, height);
            float stepTransmittance = exp(-sigma * stepLength);
            // light scattered in this step and attenuated on its way out
            scattering += transmittance * sunVisibility * (1.0 - stepTransmittance);
            transmittance *= stepTransmittance;
            t += stepLength;
        }
    }

    result = vec4(scattering, transmittance, min(sceneDistance, SKY_DISTANCE), 1.0);
}
