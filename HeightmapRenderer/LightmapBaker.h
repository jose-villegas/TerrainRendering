#pragma once

// Monte Carlo light baker for a heightfield, one lightmap texel per height
// sample. The geometry never changes during a bake, only the sun moves, so
// everything that depends on the geometry alone is traced once in prepare():
// - a horizon map, the horizon elevation around every texel in a fixed set of
//   azimuths, which gives the sun disk visibility for any sun direction
// - the first hit of a set of cosine distributed hemisphere rays per texel,
//   which gives the sky visibility and, per sun direction, the light bounced
//   by the terrain, gathered from the hit texels
// bakeSlice() then lights one sun direction with lookups only.
class LightmapBaker
{
    public:
        // heights holds resolution * resolution samples in texel units, so a
        // height of 1 rises as much as one texel spans horizontally. albedo
        // holds the diffuse reflectance per texel. returns false if cancelled
        bool prepare(const std::vector<float> &heights,
                     const std::vector<float> &albedo, int resolution,
                     const std::atomic<bool> &cancel);
        // sky light per texel, including its bounces, flat open ground is 1
        const std::vector<float> &SkyLight() const { return skyLight; }
        // lights one sun direction (towards the sun, x along the texel
        // columns, z along the rows, y up). writes the sun disk visibility
        // [0, 1] and the bounced sun light, in units of the sun irradiance on
        // a surface facing it, per texel
        void bakeSlice(const glm::vec3 &sunDirection,
                       std::vector<float> &sunVisibility,
                       std::vector<float> &bounce) const;

        // angular radius of the sun disk, larger than the real 0.27 degrees
        // so the penumbras show at the lightmap resolution
        static const float SunAngularRadius;
        // hemisphere rays per texel, a square number, stratified
        static const int HemisphereRays = 64;
        // horizon azimuths per texel
        static const int HorizonDirections = 128;
        // diffuse bounces after the first hit
        static const int Bounces = 2;
        // sun disk samples per texel
        static const int SunSamples = 32;

    private:
        int resolution = 0;
        std::vector<float> heights;
        std::vector<float> albedo;
        std::vector<glm::vec3> normals;
        // horizon elevation, [-pi/2, pi/2] quantized to 16 bits, per texel
        // and azimuth, azimuth fastest
        std::vector<unsigned short> horizon;
        // texel index of the first hit per texel and hemisphere ray, NoHit
        // when the ray leaves the terrain
        std::vector<unsigned int> hits;
        std::vector<float> skyLight;
        static const unsigned int NoHit = 0xFFFFFFFFu;

        float heightAt(float x, float y) const;
        void computeNormals();
        void traceHorizons(int row, float maxHeight);
        void traceHemisphere(int row, float maxHeight, std::vector<float> &sky);
        // out = albedo * mean of light over each texel's hits
        void gather(const std::vector<float> &light, std::vector<float> &out) const;
};
