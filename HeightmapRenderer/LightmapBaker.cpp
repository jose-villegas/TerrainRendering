#include "Commons.h"
#include "LightmapBaker.h"
#include <atomic>

const float LightmapBaker::SunAngularRadius = glm::radians(2.0f);

namespace
{
    // keeps rays from hitting the surface they start on
    const float surfaceBias = 0.05f;

    // the horizon elevation in 16 bits
    unsigned short packElevation(float elevation)
    {
        float unit = elevation / glm::pi<float>() + 0.5f;
        return (unsigned short)(util::clamp(unit, 0.0f, 1.0f) * 65535.0f + 0.5f);
    }

    float unpackElevation(unsigned short packed)
    {
        return (packed / 65535.0f - 0.5f) * glm::pi<float>();
    }

    // integer hash for the per texel sample rotation
    unsigned int hash(unsigned int x)
    {
        x ^= x >> 16;
        x *= 0x7feb352du;
        x ^= x >> 15;
        x *= 0x846ca68bu;
        x ^= x >> 16;
        return x;
    }

    float unitFloat(unsigned int x)
    {
        return (x >> 8) * (1.0f / 16777216.0f);
    }

    // orthonormal tangents around a unit vector
    void tangents(const glm::vec3 &n, glm::vec3 &t1, glm::vec3 &t2)
    {
        glm::vec3 helper = std::abs(n.y) < 0.99f ? glm::vec3(0, 1, 0)
                           : glm::vec3(1, 0, 0);
        t1 = glm::normalize(glm::cross(helper, n));
        t2 = glm::cross(n, t1);
    }
}

float LightmapBaker::heightAt(float x, float y) const
{
    const float last = (float)(resolution - 1);
    x = util::clamp(x, 0.0f, last);
    y = util::clamp(y, 0.0f, last);
    int x0 = std::min((int)x, resolution - 2);
    int y0 = std::min((int)y, resolution - 2);
    float fx = x - x0, fy = y - y0;
    const float *row0 = &heights[y0 * resolution + x0];
    const float *row1 = row0 + resolution;
    return glm::mix(glm::mix(row0[0], row0[1], fx),
                    glm::mix(row1[0], row1[1], fx), fy);
}

void LightmapBaker::computeNormals()
{
    normals.resize(heights.size());
    concurrency::parallel_for(0, resolution, [&](int y)
    {
        for(int x = 0; x < resolution; x++)
        {
            int xl = std::max(x - 1, 0), xr = std::min(x + 1, resolution - 1);
            int yl = std::max(y - 1, 0), yr = std::min(y + 1, resolution - 1);
            float dx = (heights[y * resolution + xr] - heights[y * resolution + xl])
                       / (xr - xl);
            float dy = (heights[yr * resolution + x] - heights[yl * resolution + x])
                       / (yr - yl);
            normals[y * resolution + x] = glm::normalize(glm::vec3(-dx, 1.0f, -dy));
        }
    });
}

void LightmapBaker::traceHorizons(int y, float maxHeight)
{
    const float last = (float)(resolution - 1);

    for(int x = 0; x < resolution; x++)
    {
        int index = y * resolution + x;
        float origin = heights[index] + surfaceBias;
        unsigned short *out = &horizon[(size_t)index * HorizonDirections];

        for(int a = 0; a < HorizonDirections; a++)
        {
            float azimuth = glm::two_pi<float>() * a / HorizonDirections;
            float dirX = std::cos(azimuth), dirY = std::sin(azimuth);
            // tangent of the highest elevation found so far
            float highest = -1e30f;
            float distance = 1.0f;

            while(true)
            {
                float px = x + dirX * distance, py = y + dirY * distance;

                if(px < 0.0f || py < 0.0f || px > last || py > last) break;

                highest = std::max(highest, (heightAt(px, py) - origin) / distance);

                // nothing further away can rise above the current horizon
                if((maxHeight - origin) / distance <= highest) break;

                distance += std::max(0.5f, distance * 0.04f);
            }

            out[a] = packElevation(std::atan(highest));
        }
    }
}

void LightmapBaker::traceHemisphere(int y, float maxHeight,
                                    std::vector<float> &sky)
{
    const float last = (float)(resolution - 1);
    const int strata = (int)std::sqrt((float)HemisphereRays);

    for(int x = 0; x < resolution; x++)
    {
        int index = y * resolution + x;
        const glm::vec3 &n = normals[index];
        glm::vec3 t1, t2;
        tangents(n, t1, t2);
        // random rotation of the stratified pattern per texel
        unsigned int seed = hash(index * 2u + 1u);
        float rotateU = unitFloat(seed), rotateV = unitFloat(hash(seed));
        float origin = heights[index] + surfaceBias;
        float skyVisible = 0.0f;
        unsigned int *out = &hits[(size_t)index * HemisphereRays];

        for(int s = 0; s < HemisphereRays; s++)
        {
            float u = std::fmod((s % strata + 0.5f) / strata + rotateU, 1.0f);
            float v = std::fmod((s / strata + 0.5f) / strata + rotateV, 1.0f);
            // cosine distributed direction around the normal
            float r = std::sqrt(u), phi = glm::two_pi<float>() * v;
            glm::vec3 dir = t1 * (r * std::cos(phi)) + t2 * (r * std::sin(phi))
                            + n * std::sqrt(1.0f - u);
            unsigned int hit = NoHit;
            float horizontal = std::sqrt(dir.x * dir.x + dir.z * dir.z);

            if(horizontal > 1e-4f)
            {
                float stepX = dir.x / horizontal, stepY = dir.z / horizontal;
                float rise = dir.y / horizontal;
                float distance = 0.5f;

                while(true)
                {
                    float px = x + stepX * distance, py = y + stepY * distance;

                    if(px < 0.0f || py < 0.0f || px > last || py > last) break;

                    float rayHeight = origin + rise * distance;

                    if(rise >= 0.0f && rayHeight > maxHeight) break;

                    if(rayHeight < heightAt(px, py))
                    {
                        hit = (unsigned int)(int(py + 0.5f) * resolution + int(px + 0.5f));
                        break;
                    }

                    distance += std::max(0.5f, distance * 0.02f);
                }
            }

            out[s] = hit;

            // overcast sky, twice as bright at the zenith as at the horizon,
            // scaled so open flat ground receives 1
            if(hit == NoHit) skyVisible += std::max(0.0f, (3.0f + 6.0f * dir.y) / 7.0f);
        }

        sky[index] = skyVisible / HemisphereRays;
    }
}

void LightmapBaker::gather(const std::vector<float> &light,
                           std::vector<float> &out) const
{
    // light leaving each texel, lambertian reflection
    std::vector<float> reflected(light.size());

    for(size_t i = 0; i < light.size(); i++) reflected[i] = albedo[i] * light[i];

    out.resize(light.size());
    concurrency::parallel_for(0, resolution, [&](int y)
    {
        for(int x = 0; x < resolution; x++)
        {
            int index = y * resolution + x;
            const unsigned int *texelHits = &hits[(size_t)index * HemisphereRays];
            float sum = 0.0f;

            for(int s = 0; s < HemisphereRays; s++)
            {
                if(texelHits[s] != NoHit) sum += reflected[texelHits[s]];
            }

            // cosine distributed rays, the mean is the irradiance
            out[index] = sum / HemisphereRays;
        }
    });
}

bool LightmapBaker::prepare(const std::vector<float> &heights,
                            const std::vector<float> &albedo, int resolution,
                            const std::atomic<bool> &cancel)
{
    this->resolution = resolution;
    this->heights = heights;
    this->albedo = albedo;
    const float maxHeight = *std::max_element(heights.begin(), heights.end())
                            + surfaceBias;
    computeNormals();
    // horizons for the sun visibility
    horizon.assign((size_t)resolution * resolution * HorizonDirections, 0);
    concurrency::parallel_for(0, resolution, [&](int y)
    {
        if(!cancel) traceHorizons(y, maxHeight);
    });

    if(cancel) return false;

    // first hits of the hemisphere rays and the direct sky light
    std::vector<float> sky(heights.size());
    hits.assign((size_t)resolution * resolution * HemisphereRays, NoHit);
    concurrency::parallel_for(0, resolution, [&](int y)
    {
        if(!cancel) traceHemisphere(y, maxHeight, sky);
    });

    if(cancel) return false;

    // sky light bounced by the terrain
    skyLight = sky;
    std::vector<float> bounced;

    for(int i = 0; i < Bounces; i++)
    {
        gather(skyLight, bounced);

        for(size_t t = 0; t < skyLight.size(); t++) skyLight[t] = sky[t] + bounced[t];
    }

    return true;
}

void LightmapBaker::bakeSlice(const glm::vec3 &sunDirection,
                              std::vector<float> &sunVisibility,
                              std::vector<float> &bounce) const
{
    const size_t texels = heights.size();
    const glm::vec3 sun = glm::normalize(sunDirection);
    glm::vec3 t1, t2;
    tangents(sun, t1, t2);
    // directions over the sun disk, a spiral of equal area samples, as
    // horizon azimuth bins and elevations, the same for every texel
    int bin[SunSamples];
    float binWeight[SunSamples], elevation[SunSamples];
    const float diskScale = std::tan(SunAngularRadius);

    for(int s = 0; s < SunSamples; s++)
    {
        float r = std::sqrt((s + 0.5f) / SunSamples) * diskScale;
        float angle = s * 2.39996323f;
        glm::vec3 dir = glm::normalize(sun + t1 * (r * std::cos(angle))
                                       + t2 * (r * std::sin(angle)));
        elevation[s] = std::asin(util::clamp(dir.y, -1.0f, 1.0f));
        float azimuth = std::atan2(dir.z, dir.x);

        if(azimuth < 0.0f) azimuth += glm::two_pi<float>();

        float position = azimuth / glm::two_pi<float>() * HorizonDirections;
        bin[s] = (int)position % HorizonDirections;
        binWeight[s] = position - std::floor(position);
    }

    // samples blend over about their spacing on the disk so the penumbra
    // has no steps
    const float blend = 2.0f * SunAngularRadius / std::sqrt((float)SunSamples);
    // direct sun light per texel, the start of the bounces
    std::vector<float> direct(texels);
    sunVisibility.resize(texels);
    concurrency::parallel_for(0, resolution, [&](int y)
    {
        for(int x = 0; x < resolution; x++)
        {
            int index = y * resolution + x;
            const unsigned short *texelHorizon =
                &horizon[(size_t)index * HorizonDirections];
            float visible = 0.0f;

            for(int s = 0; s < SunSamples; s++)
            {
                float h0 = unpackElevation(texelHorizon[bin[s]]);
                float h1 = unpackElevation(texelHorizon[(bin[s] + 1) % HorizonDirections]);
                float above = elevation[s] - glm::mix(h0, h1, binWeight[s]);
                visible += util::clamp(above / blend + 0.5f, 0.0f, 1.0f);
            }

            visible /= SunSamples;
            sunVisibility[index] = visible;
            direct[index] = std::max(0.0f, glm::dot(normals[index], sun)) * visible;
        }
    });
    // bounces of the sun light
    std::vector<float> light = direct;

    for(int i = 0; i < Bounces; i++)
    {
        gather(light, bounce);

        for(size_t t = 0; t < texels; t++) light[t] = direct[t] + bounce[t];
    }
}
