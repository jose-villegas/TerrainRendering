#pragma once
using namespace oglplus;

class Camera
{
    private:
        enum Plane
        {
            Right = 0,
            Left,
            Bottom,
            Top,
            Far,
            Near
        };
        glm::vec4 planes[6];

        inline int vectorToIndex(const glm::vec3 &v) const
        {
            int idx = 0;

            if(v.z >= 0) idx |= 1;

            if(v.y >= 0) idx |= 2;

            if(v.x >= 0) idx |= 4;

            return idx;
        }

    public:
        glm::vec4 getPlane(Plane p) const;
        void calcPlanes(const glm::mat4 &matrix);
        int halfPlaneTest(const glm::vec3 &p, const glm::vec3 &normal, float offset);
        int isBoxInFrustum(const glm::vec3 &origin, const glm::vec3 &halfDim);

    private:
        // view matrix
        glm::vec3 eye;
        glm::vec3 center;
        glm::vec3 upVector;
        // projection matrix
        float fovy;
        float aspect;
        float nearClip;
        float farClip;
        // frustum plane, left, right, bottom, up
        glm::vec4 frustumP;
        // screen size
        int width;
        int height;
        // do not call this one
        void perspective(float fovy, float aspect, float nearClip, float farClip);
    public:
        Camera();
        ~Camera();
        // sets the camera at position looking at lookAt
        void lookAt(const glm::vec3 &eye, const glm::vec3 &center,
                    const glm::vec3 &up = glm::vec3(0.0f, 1.0f, 0.0));

        void perspective(float fovy, int width, int height, float nearClip,
                         float farClip);

        const glm::vec3 &Position() const { return eye; }
        float FarClip() const { return farClip; }
        float NearClip() const { return nearClip; }
        // frustum plane, left, right, bottom, up
        const glm::vec4 &Frustum() const { return frustumP; }
        glm::vec2 ScreenSize() { return glm::vec2(width, height); }
};

