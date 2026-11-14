#include "Commons.h"
#include "LightShafts.h"
#include <fstream>

namespace
{
    // texture units of the passes, away from the ones the terrain uses
    const GLuint depthUnit = 8;    // nearest filtering
    const GLuint firstLinearUnit = 9;
    // fog top over the terrain's highest possible point
    const float fogTopScale = 1.25f;
    // fog density at the terrain top height relative to the ground: exp(-x)
    const float heightFalloff = 2.0f;

    std::string readFile(const char *path)
    {
        std::ifstream file(path, std::ios::in | std::ios::binary);

        if(!file) throw std::runtime_error(std::string("Cannot open ") + path);

        std::stringstream content;
        content << file.rdbuf();
        return content.str();
    }

    GLuint compileShader(GLenum type, const char *path)
    {
        std::string source = readFile(path);
        const char *text = source.c_str();
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &text, nullptr);
        glCompileShader(shader);
        GLint status = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

        if(status != GL_TRUE)
        {
            char log[4096] = {};
            glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
            glDeleteShader(shader);
            throw std::runtime_error(std::string(path) + ": " + log);
        }

        return shader;
    }

    GLuint createTexture(GLenum internalFormat, GLenum format, GLenum type,
                         int width, int height)
    {
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format,
                     type, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        return texture;
    }

    GLuint createFramebuffer(GLuint color, GLuint depth)
    {
        GLuint framebuffer;
        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                               color, 0);

        if(depth)
        {
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                                   depth, 0);
        }

        if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            throw std::runtime_error("Light shafts framebuffer incomplete");
        }

        return framebuffer;
    }

    void bindTexture(GLuint unit, GLuint texture)
    {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, texture);
    }

    GLint location(GLuint program, const char *name)
    {
        return glGetUniformLocation(program, name);
    }

    void setSampler(GLuint program, const char *name, GLuint unit)
    {
        glUseProgram(program);
        glUniform1i(location(program, name), unit);
    }

    // samplers bound to the pass units, so the textures keep their own
    // parameters
    GLuint nearestSampler = 0;
    GLuint linearSampler = 0;

    void drawFullscreen()
    {
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
}

GLuint LightShafts::buildProgram(const char *fragmentPath)
{
    GLuint vertex = compileShader(GL_VERTEX_SHADER,
                                  "Resources/Shaders/fullscreen.vert");
    GLuint fragment;

    try
    {
        fragment = compileShader(GL_FRAGMENT_SHADER, fragmentPath);
    }
    catch(...)
    {
        glDeleteShader(vertex);
        throw;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    GLint status = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &status);

    if(status != GL_TRUE)
    {
        char log[4096] = {};
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        glDeleteProgram(program);
        throw std::runtime_error(std::string(fragmentPath) + ": " + log);
    }

    return program;
}

void LightShafts::initialize()
{
    initialized = true;

    try
    {
        shadowProgram = buildProgram("Resources/Shaders/shafts_shadow_height.frag");
        marchProgram = buildProgram("Resources/Shaders/shafts_march.frag");
        blurProgram = buildProgram("Resources/Shaders/shafts_blur.frag");
        compositeProgram = buildProgram("Resources/Shaders/shafts_composite.frag");
    }
    catch(std::exception &e)
    {
        APP_LOG(error) << "Light shafts disabled, " << e.what();
        failed = true;
        return;
    }

    setSampler(shadowProgram, "meshHeights", firstLinearUnit);
    setSampler(marchProgram, "sceneDepth", depthUnit);
    setSampler(marchProgram, "shadowHeight", firstLinearUnit);
    setSampler(blurProgram, "source", firstLinearUnit + 1);
    setSampler(compositeProgram, "sceneDepth", depthUnit);
    setSampler(compositeProgram, "sceneColor", firstLinearUnit);
    setSampler(compositeProgram, "shafts", firstLinearUnit + 1);
    glUseProgram(0);
    glGenVertexArrays(1, &emptyVertexArray);
    glGenSamplers(1, &nearestSampler);
    glGenSamplers(1, &linearSampler);
    GLuint samplers[2] = { nearestSampler, linearSampler };
    GLint filters[2] = { GL_NEAREST, GL_LINEAR };

    for(int i = 0; i < 2; i++)
    {
        glSamplerParameteri(samplers[i], GL_TEXTURE_MIN_FILTER, filters[i]);
        glSamplerParameteri(samplers[i], GL_TEXTURE_MAG_FILTER, filters[i]);
        glSamplerParameteri(samplers[i], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glSamplerParameteri(samplers[i], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    APP_LOG(info) << "Light shafts initialized";
}

void LightShafts::releaseTargets()
{
    if(sceneFramebuffer)
    {
        glDeleteFramebuffers(1, &sceneFramebuffer);
        glDeleteFramebuffers(2, shaftFramebuffer);
        glDeleteTextures(1, &sceneColor);
        glDeleteTextures(1, &sceneDepth);
        glDeleteTextures(2, shaftTexture);
    }

    sceneFramebuffer = sceneColor = sceneDepth = 0;
    width = height = 0;
}

void LightShafts::resize(int width, int height)
{
    releaseTargets();
    // keep the active unit's binding, unit 0 holds the terrain textures
    glActiveTexture(GL_TEXTURE0 + depthUnit);
    sceneColor = createTexture(GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, width, height);
    sceneDepth = createTexture(GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT,
                               GL_UNSIGNED_INT, width, height);
    sceneFramebuffer = createFramebuffer(sceneColor, sceneDepth);
    int halfWidth = std::max(1, (width + 1) / 2);
    int halfHeight = std::max(1, (height + 1) / 2);

    for(int i = 0; i < 2; i++)
    {
        shaftTexture[i] = createTexture(GL_RGBA16F, GL_RGBA, GL_HALF_FLOAT,
                                        halfWidth, halfHeight);
        shaftFramebuffer[i] = createFramebuffer(shaftTexture[i], 0);
    }

    this->width = width;
    this->height = height;
}

void LightShafts::beginScene(int width, int height)
{
    if(failed || width <= 0 || height <= 0) return;

    if(!initialized) initialize();

    if(failed) return;

    if(width != this->width || height != this->height)
    {
        try
        {
            resize(width, height);
        }
        catch(std::exception &e)
        {
            APP_LOG(error) << "Light shafts disabled, " << e.what();
            releaseTargets();
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            failed = true;
            return;
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, sceneFramebuffer);
    glViewport(0, 0, width, height);
    // same clear color as the default framebuffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

bool LightShafts::prepareShadowHeight(GLuint heights)
{
    bindTexture(firstLinearUnit, heights);
    GLint size = 0;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &size);

    if(size <= 0) return false;

    if(size == shadowSize) return true;

    if(shadowHeight)
    {
        glDeleteFramebuffers(1, &shadowFramebuffer);
        glDeleteTextures(1, &shadowHeight);
        shadowFramebuffer = shadowHeight = 0;
        shadowSize = 0;
    }

    glActiveTexture(GL_TEXTURE0 + depthUnit);
    shadowHeight = createTexture(GL_R16F, GL_RED, GL_HALF_FLOAT, size, size);

    try
    {
        shadowFramebuffer = createFramebuffer(shadowHeight, 0);
    }
    catch(std::exception &e)
    {
        APP_LOG(error) << "Light shafts disabled, " << e.what();
        failed = true;
        return false;
    }

    shadowSize = size;
    return true;
}

void LightShafts::drawPasses(const Sun &sun, const TerrainInfo &terrain,
                             const View &view)
{
    // 1. shadow heights, one texel per mesh vertex, heights on the linear unit
    glm::vec3 terrainSun = sun.direction / glm::vec3(terrain.horizontalScale,
                           terrain.heightScale, terrain.horizontalScale);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFramebuffer);
    glViewport(0, 0, shadowSize, shadowSize);
    glUseProgram(shadowProgram);
    glUniform3fv(location(shadowProgram, "sunDirection"), 1,
                 glm::value_ptr(terrainSun));
    drawFullscreen();
    // 2. half resolution march through the fog over the terrain
    glm::mat4 inverseViewProjection = glm::inverse(view.projection * view.view);
    float halfSize = terrain.horizontalScale * 0.5f;
    glm::vec3 boxMin(-halfSize, 0.0f, -halfSize);
    glm::vec3 boxMax(halfSize, terrain.heightScale * fogTopScale, halfSize);
    int halfWidth = std::max(1, (width + 1) / 2);
    int halfHeight = std::max(1, (height + 1) / 2);
    bindTexture(depthUnit, sceneDepth);
    bindTexture(firstLinearUnit, shadowHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, shaftFramebuffer[0]);
    glViewport(0, 0, halfWidth, halfHeight);
    glUseProgram(marchProgram);
    glUniformMatrix4fv(location(marchProgram, "inverseViewProjection"), 1,
                       GL_FALSE, glm::value_ptr(inverseViewProjection));
    glUniform3fv(location(marchProgram, "cameraPosition"), 1,
                 glm::value_ptr(view.position));
    glUniform3fv(location(marchProgram, "boxMin"), 1, glm::value_ptr(boxMin));
    glUniform3fv(location(marchProgram, "boxMax"), 1, glm::value_ptr(boxMax));
    glUniform2f(location(marchProgram, "terrainScale"), terrain.horizontalScale,
                terrain.heightScale);
    glUniform1f(location(marchProgram, "density"), density);
    glUniform1f(location(marchProgram, "heightFalloff"), heightFalloff);
    drawFullscreen();
    // 3. blur, horizontal then vertical, ends in the first texture
    glUseProgram(blurProgram);
    GLint directionLocation = location(blurProgram, "direction");

    for(int pass = 0; pass < 2; pass++)
    {
        bindTexture(firstLinearUnit + 1, shaftTexture[pass]);
        glBindFramebuffer(GL_FRAMEBUFFER, shaftFramebuffer[1 - pass]);
        glUniform2f(directionLocation, pass == 0 ? 1.0f / halfWidth : 0.0f,
                    pass == 0 ? 0.0f : 1.0f / halfHeight);
        drawFullscreen();
    }

    // 4. composite into the default framebuffer
    glm::vec3 sunDirection = glm::normalize(sun.direction);
    bindTexture(firstLinearUnit, sceneColor);
    bindTexture(firstLinearUnit + 1, shaftTexture[0]);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);
    glUseProgram(compositeProgram);
    glUniformMatrix4fv(location(compositeProgram, "inverseViewProjection"), 1,
                       GL_FALSE, glm::value_ptr(inverseViewProjection));
    glUniform3fv(location(compositeProgram, "cameraPosition"), 1,
                 glm::value_ptr(view.position));
    glUniform3fv(location(compositeProgram, "sunDirection"), 1,
                 glm::value_ptr(sunDirection));
    glUniform3fv(location(compositeProgram, "sunColor"), 1,
                 glm::value_ptr(sun.color));
    glUniform1f(location(compositeProgram, "intensity"), intensity);
    drawFullscreen();
}

void LightShafts::endScene(const Sun &sun, const TerrainInfo &terrain,
                           const View &view)
{
    if(failed || !sceneFramebuffer) return;

    // state the terrain and the interface expect afterwards
    GLint previousProgram, previousVertexArray, previousActiveTexture;
    GLint previousViewport[4], previousPolygonMode[2];
    glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previousVertexArray);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActiveTexture);
    glGetIntegerv(GL_VIEWPORT, previousViewport);
    glGetIntegerv(GL_POLYGON_MODE, previousPolygonMode);
    const GLenum capabilities[] = { GL_DEPTH_TEST, GL_CULL_FACE, GL_BLEND, GL_SCISSOR_TEST };
    GLboolean previouslyEnabled[4];

    for(int i = 0; i < 4; i++)
    {
        previouslyEnabled[i] = glIsEnabled(capabilities[i]);
        glDisable(capabilities[i]);
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glBindVertexArray(emptyVertexArray);
    glBindSampler(depthUnit, nearestSampler);
    glBindSampler(firstLinearUnit, linearSampler);
    glBindSampler(firstLinearUnit + 1, linearSampler);

    if(prepareShadowHeight(terrain.heights))
    {
        drawPasses(sun, terrain, view);
    }
    else
    {
        // no shafts this frame, the scene as it is
        glBindFramebuffer(GL_READ_FRAMEBUFFER, sceneFramebuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, width, height, 0, 0, width, height,
                          GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // restore
    for(GLuint unit = depthUnit; unit <= firstLinearUnit + 1; unit++)
    {
        glBindSampler(unit, 0);
    }

    for(int i = 0; i < 4; i++)
    {
        if(previouslyEnabled[i]) glEnable(capabilities[i]);
    }

    glPolygonMode(GL_FRONT, previousPolygonMode[0]);
    glPolygonMode(GL_BACK, previousPolygonMode[1]);
    glViewport(previousViewport[0], previousViewport[1], previousViewport[2],
               previousViewport[3]);
    glActiveTexture(previousActiveTexture);
    glBindVertexArray(previousVertexArray);
    glUseProgram(previousProgram);
}

LightShafts::LightShafts()
{
    shaftFramebuffer[0] = shaftFramebuffer[1] = 0;
    shaftTexture[0] = shaftTexture[1] = 0;
}

LightShafts::~LightShafts()
{
    // the context is gone by the time the app destroys its members
}
