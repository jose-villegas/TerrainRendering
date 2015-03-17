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