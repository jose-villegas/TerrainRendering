#include "App.h"

App * App::instance = nullptr;

App::App(const std::string &title, const unsigned int width,
         const unsigned int height) : MainWindow(title, width, height)
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
}

void App::Configure()
{
    // already configured
    if(instance) return;

    // glfw error catching
    glfwSetErrorCallback(App::onError);
    // initialize glfw for context creation
    glfwInit();
    // instantiate app thus creating rendering window
    instance = new App("Terrain Rendering", 800, 600);

    // couldn't create app instance
    if(!instance) { exit(1); }

    // set app callbacks
    glfwSetKeyCallback(instance->getWindow(), App::onKeyPress);
    // make window as current rendering context
    instance->makeCurrentContext();
    // initialize glew library for opengl api access
    GLenum err = glewInit();

    if(err != GLEW_OK)
    {
        throw std::runtime_error((const char*)glewGetErrorString(err));
    }

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
}

App * App::Instance()
{
    if(!instance)
    {
        App::Configure();
        return instance;
    }

    return instance;
}

void App::Start()
{
    if(!instance) return;

    oglplus::Example triangle1(oglplus::Vec3f(1, 0, 0));

    while(true)
    {
        if(glfwWindowShouldClose(instance->getWindow())) { break; }

        triangle1.display(instance->width, instance->height);
        // app specific code -- here --
        glfwSwapBuffers(instance->getWindow());
        glfwPollEvents();
    }
}

void App::Run()
{
    try
    {
        // app initialization
        App::Configure();
        // start app loop
        App::Start();
        // delete window and terminate glfw context
        delete instance;
        glfwTerminate();
    }
    catch(std::exception& e)
    {
        glfwTerminate();
        throw std::runtime_error(e.what());
    }
}