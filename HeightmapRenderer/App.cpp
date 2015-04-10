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
    //setup logging to file
    logging::add_file_log
    (
        keywords::file_name = "app_%N.log",
        keywords::format =
            (
                expr::stream
                << expr::format_date_time< boost::posix_time::ptime >("TimeStamp",
                        "%Y-%m-%d %H:%M:%S") << ":"
                << expr::attr< unsigned int >("LineID")
                << " <" << logging::trivial::severity << "> "
                << expr::smessage
            )
    );
    logging::add_console_log
    (
        std::cout,
        keywords::format =
            (
                expr::stream
                << expr::format_date_time< boost::posix_time::ptime >("TimeStamp",
                        "%Y-%m-%d %H:%M:%S") << ":"
                << expr::attr< unsigned int >("LineID")
                << " <" << logging::trivial::severity << "> "
                << expr::smessage
            )
    );
    // filter priority
    logging::core::get()->set_filter
    (
        logging::trivial::severity >= logging::trivial::info
    );
    logging::add_common_attributes();
    // log libraries version
    BOOST_LOG_TRIVIAL(info) << "GLFW " << glfwGetVersionString();
    BOOST_LOG_TRIVIAL(info) << "OpenGL " << glGetString(GL_VERSION) << "s, GLSL " <<
                            glGetString(GL_SHADING_LANGUAGE_VERSION);
    BOOST_LOG_TRIVIAL(info) << "Boost "
                            << BOOST_VERSION / 100000 << "."        // major version
                            << BOOST_VERSION / 100 % 1000 << "."    // minior version
                            << BOOST_VERSION % 100;                 // patch level
    BOOST_LOG_TRIVIAL(info) << "Ocornut's IMGUI " << ImGui::GetVersion();
    BOOST_LOG_TRIVIAL(info) << "FreeImage "
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

    while(!glfwWindowShouldClose(this->appWindow->getWindow()))
    {
        if(!gui.pauseTime) time += ImGui::GetIO().DeltaTime;

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
        // render terrain
        terrain.render(time);
        // render user interface
        gui.render();
        // swap double buffer
        glfwSwapBuffers(this->appWindow->getWindow());
    }
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
        BOOST_LOG_TRIVIAL(error) << e.Log();
        throw std::runtime_error(e.what());
    }
    catch(std::exception &e)
    {
        glfwTerminate();
        BOOST_LOG_TRIVIAL(error) << e.what();
        throw std::runtime_error(e.what());
    }
}