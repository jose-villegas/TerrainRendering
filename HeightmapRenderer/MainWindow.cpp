#include "Commons.h"
#include "MainWindow.h"

GLFWwindow* MainWindow::currentContext = nullptr;

MainWindow::MainWindow(const std::string &windowTitle, const unsigned int width,
                       const unsigned int height, GLFWmonitor* monitor /* = nullptr */,
                       GLFWwindow * share /* = nullptr */) : window(glfwCreateWindow(width, height,
                                   windowTitle.c_str(), monitor, share), glfwDestroyWindow)
{
    if(!window)
    {
        throw std::runtime_error("GLFW3 failed to create a window");
    }

    // set internal class values
    this->width = width;
    this->height = height;
    this->title = windowTitle;
    this->monitor = monitor;
    this->share = share;
}

void MainWindow::makeCurrentContext()
{
    glfwMakeContextCurrent(this->getWindow());
    currentContext = this->getWindow();
}