#include "Commons.h"
#include "App.h"
#include "TerrainPatch.h"
#include "TransformationMatrices.h"
#include "Terrain.h"

Terrain * App::terrain = nullptr;
App * App::instance = nullptr;

App::App(const std::string &title, const unsigned int width,
         const unsigned int height)
{
}

void App::onError(int code, const char * description)
{
    throw std::runtime_error(description);
}

int counter;
float stopRot = 0.0;

void App::onKeyPress(GLFWwindow *window, int key, int scancode, int action,
                     int mods)
{
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

    if(key == GLFW_KEY_W && action == GLFW_PRESS)
    {
        terrain->test(++counter);
    }

    if(key == GLFW_KEY_S && action == GLFW_PRESS)
    {
        terrain->test(--counter);
    }

    if(key == GLFW_KEY_P && action == GLFW_PRESS)
    {
        stopRot ? stopRot = 0 : stopRot = glfwGetTime();
    }
}

void App::onWindowResize(GLFWwindow *window, int width, int height)
{
    gl.Viewport(width, height);
    TransformationMatrices::Projection(
        glm::perspective(
            glm::radians(60.0f),
            (float)width / height,
            0.01f, 30.0f
        )
    );
}

void App::Configure()
{
    // glfw error catching
    glfwSetErrorCallback(App::onError);
    // initialize glfw for context creation
    glfwInit();
    // setup rendering window
    this->appWindow = new MainWindow("Terrain Rendering", 800, 600);
    // make window as current rendering context
    this->appWindow->makeCurrentContext();
    // initialize glew library for opengl api access
    GLenum err = glewInit();

    if(err != GLEW_OK)
    {
        throw std::runtime_error((const char*)glewGetErrorString(err));
    }

    terrain = new Terrain();
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

    while(true)
    {
        if(glfwWindowShouldClose(instance->appWindow->getWindow())) { break; }

        // clean color and depth buff
        gl.Clear().ColorBuffer().DepthBuffer(); glClear(GL_COLOR_BUFFER_BIT);
        double time = glfwGetTime();
        time = stopRot ? stopRot : time;
        // set scene matrixes
        TransformationMatrices::View(
            glm::lookAt(
                glm::vec3(std::cos(time * 0.33) * 2, 1, std::sin(time * 0.33) * 2),
                glm::vec3(0.5, 0.5, 0),
                glm::vec3(0, 1, 0)
            )
        );
        terrain->display();
        glfwSwapBuffers(this->appWindow->getWindow());
        glfwPollEvents();
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