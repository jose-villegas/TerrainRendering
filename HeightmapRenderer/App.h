#pragma once
#include "MainWindow.h"

class App
{
    private:
        MainWindow *appWindow;
        static App *instance;

        App(const App &rhs);
        App(const std::string &title, const unsigned int width,
            const unsigned int height);
        // app callbacks
        static void onError(int code, const char * description);
        static void onKeyPress(GLFWwindow *window, int key, int scancode, int action,
                               int mods);
        // app libraries configuration
        void Configure();
        // app render loop
        void Start();
    public:
        static App * Instance();
        void Run();

        ~App();
};

