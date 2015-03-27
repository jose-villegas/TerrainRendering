#pragma once

class MainWindow
{
    private:
        static GLFWwindow* currentContext;
        std::unique_ptr<GLFWwindow, void(*)(GLFWwindow*)> window;
        // no copy constructor
        MainWindow(const MainWindow &rhs);

    public:
        MainWindow(const std::string &windowTitle, const unsigned int width,
                   const unsigned int height, GLFWmonitor* monitor = nullptr,
                   GLFWwindow * share = nullptr);
        virtual ~MainWindow() { this->window.reset(); };
        // return glfw raw window pointer
        GLFWwindow * getWindow() const { return this->window.get(); };
        // set up as current rendering context
        void makeCurrentContext();
        // window events callbacks
        void setWindowPositionCallback(void (*fptr)(void*, int, int));
        void setWindowSizeCallback(void(*fptr)(void*, int, int));
        void setWindowCloseCallback(void(*fptr)(void*, bool));
        void setWindowRefreshCallback(void(*fptr)(void*));
        void setWindowFocusCallback(void(*fptr)(void*, bool));

        const unsigned int windowWidth() { glfwGetWindowSize(this->window.get(), &this->width, &this->height); return width; }
        const unsigned int windowHeight() { glfwGetWindowSize(this->window.get(), &this->width, &this->height); return height; }

    protected:
        int width;
        int height;
        std::string title;
        GLFWmonitor *monitor;
        GLFWwindow *share;
        // window handling methods
};

