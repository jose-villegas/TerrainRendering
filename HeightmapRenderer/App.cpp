#include "Commons.h"
#include "App.h"
#include "TerrainPatch.h"
#include "TransformationMatrices.h"
#include "Terrain.h"

App * App::instance = nullptr;
bool App::wireframeMode = false;

App::App(const std::string &title, const unsigned int width,
         const unsigned int height) : appWindow(CreateContext())
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

    if(key == GLFW_KEY_1 && action == GLFW_PRESS)
    {
        if(!wireframeMode)
        {
            glPolygonMode(GL_FRONT, GL_LINE);
            glPolygonMode(GL_BACK, GL_LINE);
            wireframeMode = true;
        }
        else
        {
            glPolygonMode(GL_FRONT, GL_FILL);
            glPolygonMode(GL_BACK, GL_FILL);
            wireframeMode = false;
        }
    }
}

void App::onWindowResize(GLFWwindow *window, int width, int height)
{
    gl.Viewport(width, height);
    TransformationMatrices::Projection(
        glm::perspective(
            glm::radians(60.0f),
            (float)width / height,
            0.1f, 100.0f
        )
    );
}

void App::Configure()
{
    // set app callbacks
    glfwSetKeyCallback(this->appWindow->getWindow(), App::onKeyPress);
    glfwSetWindowSizeCallback(this->appWindow->getWindow(), App::onWindowResize);
    App::onWindowResize(this->appWindow->getWindow(),
                        this->appWindow->windowWidth(),
                        this->appWindow->windowHeight());
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
}

MainWindow * App::CreateContext()
{
    // glfw error catching
    glfwSetErrorCallback(App::onError);
    // initialize glfw for context creation
    glfwInit();
    // setup rendering window
    MainWindow *newWindow = new MainWindow("Terrain Rendering", 800, 600);
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
    delete this->appWindow;
    glfwTerminate();
}

App * App::Instance()
{
    if(!instance)
    {
        return instance = new App("Terrain Rendering", 800, 600);
    }

    return instance;
}

void App::Start()
{
    if(!instance) return;

    terrain.initialize();
    terrain.createTerrain(9);
    terrain.createMesh();
    float lastTime = 0.0f;

    while(true)
    {
        double currentTime = glfwGetTime();
        float deltaTime = float(currentTime - lastTime);

        if(glfwWindowShouldClose(instance->appWindow->getWindow())) { break; }

        // clean color and depth buff
        gl.Clear().ColorBuffer().DepthBuffer(); glClear(GL_COLOR_BUFFER_BIT);
        double time = glfwGetTime() * 0.0;

        if(glfwGetKey(this->appWindow->getWindow(),
                      GLFW_KEY_P) == GLFW_PRESS) time = 0.0;

        // set scene matrixes
        TransformationMatrices::View(
            glm::lookAt(
                glm::vec3(std::cos(time * 0.33) * 2, 2, std::sin(time * 0.33) * 2),
                glm::vec3(0.0, 1.0, 0.0),
                glm::vec3(0, 1, 0)
            )
        );
        terrain.display();
        glfwSwapBuffers(this->appWindow->getWindow());
        glfwPollEvents();
        lastTime = currentTime;
    }
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