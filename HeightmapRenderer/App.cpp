#include "Commons.h"
#include "App.h"
#include "TransformationMatrices.h"
#include "Terrain.h"

App * App::instance = nullptr;

App::App() : appWindow(CreateContext())
{
}

void App::onError(int code, const char * description)
{
    throw std::runtime_error(description);
}

void App::onKeyPress(GLFWwindow *window, int key, int scancode, int action,
                     int mods)
{
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

    // f key enables free camera mode
    if(key == GLFW_KEY_F && action == GLFW_PRESS)
    {
        instance->moveAround = !instance->moveAround;
    }
}

void App::onMouseWheel(GLFWwindow *window, double xoffset, double yoffset)
{
    static float zoom = 0.0f;
    zoom += (float)yoffset / 4.0f;
    zoom = std::max(0.0f, zoom);
    instance->camera.perspective(
        glm::radians(60.0f - 5.0f * zoom),
        instance->appWindow->windowWidth(),
        instance->appWindow->windowHeight(),
        0.1, 500.f
    );
}

void App::onWindowResize(GLFWwindow *window, int width, int height)
{
    gl.Viewport(width, height);
    App::instance->camera.perspective(
        glm::radians(60.0f),
        width, height,
        0.1f, 500.0f
    );
}

void App::Configure()
{
    // ui library
    gui.initialize(this->appWindow->getWindow());
    // initialize image loading library
    FreeImage_Initialise();
    // set app callbacks
    glfwSetKeyCallback(this->appWindow->getWindow(), App::onKeyPress);
    glfwSetWindowSizeCallback(this->appWindow->getWindow(), App::onWindowResize);
    glfwSetScrollCallback(this->appWindow->getWindow(), App::onMouseWheel);
    App::onWindowResize(
        this->appWindow->getWindow(),
        this->appWindow->windowWidth(),
        this->appWindow->windowHeight()
    );
    //setup logging to console and file, filter priority
    logging::Logger::get().addFile("app_0.log");
    logging::Logger::get().setMinimum(logging::severity::info);
    // log libraries version
    APP_LOG(info) << "GLFW " << glfwGetVersionString();
    APP_LOG(info) << "OpenGL " << glGetString(GL_VERSION) << "s, GLSL " <<
                            glGetString(GL_SHADING_LANGUAGE_VERSION);
    APP_LOG(info) << "Ocornut's IMGUI " << ImGui::GetVersion();
    APP_LOG(info) << "FreeImage "
                            << std::to_string(FREEIMAGE_MAJOR_VERSION)
                            << "." + std::to_string(FREEIMAGE_MINOR_VERSION)
                            << "." + std::to_string(FREEIMAGE_RELEASE_SERIAL);
}

MainWindow * App::CreateContext()
{
    // glfw error catching
    glfwSetErrorCallback(App::onError);
    // initialize glfw for context creation
    glfwInit();
    // get os current monitor
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    glfwWindowHint(GLFW_RED_BITS, mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
    // setup rendering window
    MainWindow *newWindow = new MainWindow(
        "Terrain Rendering",
        0.75 * mode->width,
        0.75 * mode->height
    );
    glfwSetWindowPos(
        newWindow->getWindow(),
        (int)(mode->width -  0.75 * mode->width) / 2,
        (int)(mode->height - 0.75 * mode->height) / 2
    );
    // make window as current rendering context
    newWindow->makeCurrentContext();
    // initialize glew library for opengl api access
    GLenum err = glewInit();

    if(err != GLEW_OK)
    {
        throw std::runtime_error((const char*)glewGetErrorString(err));
    }

    return newWindow;
}

App::~App()
{
    gui.terminate();
    FreeImage_DeInitialise();
    delete this->appWindow;
    glfwTerminate();
}

App * App::Instance()
{
    if(!instance)
    {
        return instance = new App();
    }

    return instance;
}

void App::Start()
{
    if(!instance) return;

    terrain.initialize();
    // set view / camera matrix
    camera.lookAt(glm::vec3(0, 5, 10), glm::vec3(0));
    float time = 0.0f;
    // new seed for rand
    std::srand(std::time(nullptr));
    App::Instance()->getTerrain().createTerrain(
        (int)std::pow(2, 8),
        glm::vec3(0, 0, 5), 0
    );
    App::Instance()->getTerrain().createMesh(8);
    const bool capturing = !captureDir.empty();
    int frame = 0;

    if(capturing) setupCapture();

    while(!glfwWindowShouldClose(this->appWindow->getWindow()))
    {
        if(capturing)
        {
            if(!updateCapture(frame, time)) break;
        }
        else if(!gui.pauseTime) time += ImGui::GetIO().DeltaTime;

        //camera.lookAt(
        //    glm::vec3(
        //        std::sin(glfwGetTime() * 0.075) * 15.0,
        //        5.0,
        //        std::cos(glfwGetTime() * 0.075) * 5.0
        //    ),
        //    glm::vec3(0.0, 0.0, 0.0),
        //    glm::vec3(0, 1, 0)
        //);
        // clean color and depth buff
        gl.Clear().ColorBuffer().DepthBuffer(); glClear(GL_COLOR_BUFFER_BIT);
        // poll input events
        glfwPollEvents();
        // app specific input handling
        handleUserInput(
            this->appWindow->getWindow(),
            ImGui::GetIO().DeltaTime
        );
        // draw the user interface
        gui.draw(time);
        // render terrain, through the light shafts pass when enabled
        bool shafts = gui.lightShafts && lightShafts.Available();
        int width = 0, height = 0;

        if(shafts)
        {
            glfwGetFramebufferSize(this->appWindow->getWindow(), &width, &height);
            lightShafts.beginScene(width, height);
            shafts = lightShafts.Available();
        }

        terrain.render(time);

        if(shafts) renderLightShafts(time);

        // a shader that failed to build turns the toggle off
        if(!lightShafts.Available()) gui.lightShafts = false;

        // render user interface
        gui.render();

        if(capturing) saveFrame(frame++);

        // swap double buffer
        glfwSwapBuffers(this->appWindow->getWindow());
    }
}

namespace
{
    // capture mode frames: one full orbit, 8 seconds when played at 30 frames
    // per second, wireframe from the middle on
    const int captureFrames = 240;
    const int captureWireframeFrom = 120;
    // the orbit starts at the camera of the readme screenshot, found by
    // matching its terrain silhouette, with the sun 7 degrees high behind the
    // far ridge so its shadow fills the basin like in the screenshot, the
    // view starts back lit and brightens as the sun rises to about 57
    // degrees and the orbit turns to the sunlit slopes, the baked lightmaps
    // interpolate between the times of the day
    const float captureStartAngle = glm::radians(34.5f);
    const float captureSunFrom = 2.05f;
    const float captureSunTo = 0.6f;
    // with the light shafts the sun sets instead, low enough for the peaks to
    // cast long shadows through the fog, and without the wireframe
    const float captureShaftsSunFrom = 1.7f;
    const float captureShaftsSunTo = 2.05f;
    // pixel error large enough to show the level of detail chunks
    const float capturePixelError = 3.0f;
}

void App::renderLightShafts(float time)
{
    LightShafts::Sun sun;
    terrain.calculateLightDir(time * gui.timeScale, sun.direction, sun.color);
    LightShafts::TerrainInfo info =
    {
        terrain.getMeshHeightsId(), terrain.TerrainHorizontalScale(), terrain.HeightScale()
    };
    LightShafts::View view =
    {
        TransformationMatrices::View(), TransformationMatrices::Projection(),
        camera.Position()
    };
    lightShafts.density = gui.shaftDensity;
    lightShafts.intensity = gui.shaftIntensity;
    lightShafts.endScene(sun, info, view);
}

void App::setupCapture()
{
    CreateDirectoryA(captureDir.c_str(), nullptr);
    // same terrain as the readme screenshot, 513 mesh on a 512 heightmap
    gui.meshResolution = gui.heightmapResolution = 9;
    terrain.createTerrain(512, glm::vec3(0, 0, 5), 0);
    terrain.createMesh(9);
    // textured terrain from low to high: soil, grass, soil, snow, on the
    // default quarter height ranges
    const char *soil =
        "Resources\\Textures\\depositphotos_30900321-Seamless-Texture-of-Rocky-Steppe-Soil..jpg";
    const char *textures[4] =
    {
        soil,
        "Resources\\Textures\\Seamless_grass_texture_by_hhh316.jpg",
        soil,
        "Resources\\Textures\\snow_scuffed_ground_1.jpg"
    };

    for(int i = 0; i < 4; i++)
    {
        terrain.loadTexture(i, textures[i]);
        terrain.setTextureRange(i, i / 4.0f, (i + 1) / 4.0f);
    }

    // the level of detail chunks show the mesh subdivision in wireframe
    gui.geomipmapping = terrain.useLoDChunks = true;
    gui.geoThreeshold = capturePixelError;
    ChunkDetailLevel::Threeshold(capturePixelError);
    gui.hidden = true;
    // the low sun of the readme shadows would turn the time of the day
    // grading into a sunset, the screenshot colors are neutral
    gui.colorGrading = false;
    terrain.EnableTimeOfTheDayColorGrading(false);
    // path trace the time of the day lighting, the first render uploads it
    terrain.bakeLightmaps(384, 512);

    while(!terrain.LightmapsBaked())
    {
        glfwPollEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    APP_LOG(info) << "Capturing " << captureFrames << " frames to " << captureDir;
}

bool App::updateCapture(int frame, float &time)
{
    if(frame >= captureFrames) return false;

    // fixed steps so the capture does not depend on the frame rate
    float progress = (float)frame / (captureFrames - 1);
    float sunFrom = gui.lightShafts ? captureShaftsSunFrom : captureSunFrom;
    float sunTo = gui.lightShafts ? captureShaftsSunTo : captureSunTo;
    time = glm::mix(sunFrom, sunTo, progress) / gui.timeScale;
    gui.wireframeMode = !gui.lightShafts && frame >= captureWireframeFrom;
    // orbit around the terrain center
    float angle = captureStartAngle + glm::two_pi<float>() * frame / captureFrames;
    camera.lookAt(
        glm::vec3(std::sin(angle) * 10.0f, 5.0f, std::cos(angle) * 10.0f),
        glm::vec3(0.0f, 0.0f, 0.0f)
    );
    return true;
}

void App::saveFrame(int frame)
{
    int width, height;
    glfwGetFramebufferSize(this->appWindow->getWindow(), &width, &height);
    std::vector<unsigned char> pixels(width * height * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, width, height, GL_BGR, GL_UNSIGNED_BYTE, pixels.data());
    // opengl rows start at the bottom, as freeimage expects without top down
    FIBITMAP *image = FreeImage_ConvertFromRawBits(
                          pixels.data(), width, height, width * 3, 24,
                          FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, FALSE);
    char name[32];
    std::snprintf(name, sizeof(name), "\\frame_%04d.png", frame);
    FreeImage_Save(FIF_PNG, image, (captureDir + name).c_str());
    FreeImage_Unload(image);
}

void App::handleUserInput(GLFWwindow * window, float deltaTime)
{
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    static glm::vec3 position = glm::vec3(0, 5, 10), direction, right, up;
    static float horizontalAngle = 3.14f;
    static float verticalAngle = 0.0f;
    static float speed = 1.5f;
    static float mouseSpeed = 0.05f;
    static double xpos, ypos;
    static float halfWidth, halfHeight;

    if(!moveAround) return;

    // get window width and height
    halfWidth = (float)this->appWindow->windowWidth() / 2.0f;
    halfHeight = (float)this->appWindow->windowHeight() / 2.0f;
    glfwGetCursorPos(window, &xpos, &ypos);
    // reset mouse position
    glfwSetCursorPos(
        window,
        halfWidth,
        halfHeight
    );
    // Compute new orientation
    horizontalAngle += mouseSpeed * deltaTime * (halfWidth - xpos);
    verticalAngle += mouseSpeed * deltaTime * (halfHeight - ypos);
    // direction, spherical coords to cartesian coords
    direction =
        glm::vec3(
            std::cos(verticalAngle) * std::sin(horizontalAngle),
            std::sin(verticalAngle),
            std::cos(verticalAngle) * std::cos(horizontalAngle)
        );
    // right vector
    right =
        glm::vec3(
            std::sin(horizontalAngle - 3.14f / 2.0f),
            0.0f,
            std::cos(horizontalAngle - 3.14f / 2.0f)
        );
    // up vector
    up = glm::cross(right, direction);

    // forward
    if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    {
        position += direction * deltaTime * speed;
    }

    // backward
    if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    {
        position -= direction * deltaTime * speed;
    }

    // right strafe
    if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    {
        position += right * deltaTime * speed;
    }

    // left strafe
    if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    {
        position -= right * deltaTime * speed;
    }

    // speed increase
    if(glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
    {
        speed += 0.25;
    }

    // speed decrease
    if(glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
    {
        speed -= 0.25;
    }

    // new camera position
    this->camera.lookAt(position, position + direction, up);
}

void App::Run()
{
    try
    {
        // app initialization
        this->Configure();
        // start app loop
        this->Start();
    }
    catch(oglplus::ProgramBuildError &e)
    {
        glfwTerminate();
        APP_LOG(error) << e.Log();
        throw std::runtime_error(e.what());
    }
    catch(std::exception &e)
    {
        glfwTerminate();
        APP_LOG(error) << e.what();
        throw std::runtime_error(e.what());
    }
}