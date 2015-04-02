#include "Commons.h"
#include "App.h"
#include "TerrainPatch.h"
#include "TransformationMatrices.h"
#include "Terrain.h"

App * App::instance = nullptr;
bool App::wireframeMode = false;

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
    // ui library
    ImGui_ImplGlfwGL3_Init(this->appWindow->getWindow(), true);
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
    BOOST_LOG_TRIVIAL(info) << "Ocornut's IMGUI " << ImGui::GetVersion();
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
        (mode->width -  0.75 * mode->width) / 2,
        (mode->height - 0.75 * mode->height) / 2
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
    terrain.createTerrain(10);
    terrain.createMesh();
    // set view / camera matrix
    TransformationMatrices::View(
        glm::lookAt(
            glm::vec3(1.0, 2.0, 1.0),
            glm::vec3(0.0, 1.0, 0.0),
            glm::vec3(0, 1, 0)
        )
    );

    while(!glfwWindowShouldClose(this->appWindow->getWindow()))
    {
        // clean color and depth buff
        gl.Clear().ColorBuffer().DepthBuffer(); glClear(GL_COLOR_BUFFER_BIT);
        // poll input events
        glfwPollEvents();
        // inmediate user interface new frame
        ImGui_ImplGlfwGL3_NewFrame();
        {
            ImGui::SetNextWindowPos(ImVec2(10, 10));
            ImGui::Begin("Performance Window", nullptr,
                         ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
            ImGui::Text("Performance");
            ImGui::Separator();
            ImGui::Text("FPS: (%.1f)", ImGui::GetIO().Framerate);
            ImGui::End();
        }
        TransformationMatrices::View(
            glm::lookAt(
                glm::vec3(std::sin(glfwGetTime() * 0.25), 2.0, std::cos(glfwGetTime() * 0.25)),
                glm::vec3(0.0, 1.5, 0.0),
                glm::vec3(0, 1, 0)
            )
        );
        // render terrain
        terrain.display();
        // render user interface
        ImGui::Render();
        // swap double buffer
        glfwSwapBuffers(this->appWindow->getWindow());
    }

    ImGui_ImplGlfwGL3_Shutdown();
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