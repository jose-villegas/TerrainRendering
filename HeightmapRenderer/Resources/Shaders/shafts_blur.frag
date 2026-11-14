#version 330

// Separable gaussian blur of the half resolution light shafts, weighted by
// view distance so the shafts do not bleed across silhouettes.

in vec2 uv;

uniform sampler2D source;
// one texel along the blur axis
uniform vec2 direction;

layout(location = 0) out vec4 result;

const float weights[5] = float[](0.227027, 0.194595, 0.121622, 0.054054, 0.016216);

void main()
{
    vec4 center = texture(source, uv);
    vec2 sum = center.rg * weights[0];
    float weightSum = weights[0];

    for(int i = 1; i < 5; i++)
    {
        for(int side = -1; side <= 1; side += 2)
        {
            vec4 tap = texture(source, uv + direction * float(i * side));
            float difference = abs(tap.b - center.b) / max(center.b, 1e-3);
            float weight = weights[i] * exp(-difference * difference * 400.0);
            sum += tap.rg * weight;
            weightSum += weight;
        }
    }

    result = vec4(sum / weightSum, center.b, 1.0);
}
