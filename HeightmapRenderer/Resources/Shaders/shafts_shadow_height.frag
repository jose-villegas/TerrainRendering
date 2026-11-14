#version 330

// For every mesh vertex, the height under which the air column above it is
// in the terrain's shadow. The terrain is a height field and the sun a
// direction, so a point at (u, h, v) is lit when h is above this height:
// the highest terrain towards the sun, minus the climb of the sun ray.
// Units: u, v in [0, 1] across the terrain, heights in [0, 1] of the mesh.
// Texel i of both textures holds vertex i, at u = i / (size - 1).

in vec2 uv;

uniform sampler2D meshHeights;
// sun direction in terrain units (u, height, v)
uniform vec3 sunDirection;

layout(location = 0) out float shadowHeight;

void main()
{
    float size = float(textureSize(meshHeights, 0).x);
    float last = size - 1.0;
    vec2 vertex = floor(uv * size);
    float result = texelFetch(meshHeights, ivec2(vertex), 0).r;
    float horizontal = length(sunDirection.xz);

    if(horizontal > 1e-5)
    {
        // one vertex spacing per step, in vertex units
        vec2 stepVertex = sunDirection.xz / horizontal;
        float stepRise = sunDirection.y / horizontal / last;
        int maxSteps = int(last * 1.5);

        for(int i = 1; i <= maxSteps; i++)
        {
            vec2 position = vertex + stepVertex * float(i);
            float rise = stepRise * float(i);

            // past the terrain border, or above the highest possible peak
            if(any(lessThan(position, vec2(0.0)))
               || any(greaterThan(position, vec2(last)))
               || rise >= 1.0) break;

            float height = texture(meshHeights, (position + 0.5) / size).r;
            result = max(result, height - rise);
        }
    }

    shadowHeight = result;
}
