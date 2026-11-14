#pragma once

// Light shafts (crepuscular rays) over the terrain as a post process.
// The terrain renders into an offscreen color and depth target, then:
// 1. shadow height: per mesh vertex, the height under which the air is
//    in the terrain's shadow for the current sun (marched over the heights)
// 2. march: per half resolution pixel, a view ray through a height fog,
//    lit only above the shadow height
// 3. blur: separable, view distance aware
// 4. composite: scene * transmittance + sun color * phase * scattering
// The passes use raw OpenGL and texture units 8 to 10, and restore the state
// the rest of the renderer relies on.
class LightShafts
{
    public:
        struct Sun
        {
            // world direction towards the sun, any length
            glm::vec3 direction;
            glm::vec3 color;
        };
        struct TerrainInfo
        {
            // mesh vertex heights in [0, 1], one texel per vertex
            GLuint heights;
            float horizontalScale;
            float heightScale;
        };
        struct View
        {
            glm::mat4 view;
            glm::mat4 projection;
            glm::vec3 position;
        };

        // extinction per world unit at the ground
        float density = 0.08f;
        // scales the scattered sunlight
        float intensity = 5.0f;

        // binds the offscreen scene target, size of the default framebuffer
        void beginScene(int width, int height);
        // draws the shafts and the scene into the default framebuffer
        void endScene(const Sun &sun, const TerrainInfo &terrain,
                      const View &view);
        // false after a shader failed to build, the caller turns it off
        bool Available() const { return !failed; }

        LightShafts();
        ~LightShafts();
    private:
        bool initialized = false;
        bool failed = false;
        int width = 0;
        int height = 0;
        GLuint emptyVertexArray = 0;
        // full resolution scene
        GLuint sceneFramebuffer = 0;
        GLuint sceneColor = 0;
        GLuint sceneDepth = 0;
        // terrain shadow heights
        GLuint shadowFramebuffer = 0;
        GLuint shadowHeight = 0;
        int shadowSize = 0;
        // half resolution ping pong
        GLuint shaftFramebuffer[2];
        GLuint shaftTexture[2];
        GLuint shadowProgram = 0;
        GLuint marchProgram = 0;
        GLuint blurProgram = 0;
        GLuint compositeProgram = 0;

        void initialize();
        void resize(int width, int height);
        void releaseTargets();
        // (re)creates the shadow height target at the heights size
        bool prepareShadowHeight(GLuint heights);
        void drawPasses(const Sun &sun, const TerrainInfo &terrain,
                        const View &view);
        static GLuint buildProgram(const char *fragmentPath);
};
