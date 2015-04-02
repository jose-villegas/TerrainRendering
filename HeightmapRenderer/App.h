#pragma once
#include "MainWindow.h"
#include "Terrain.h"

class App
{
    private:
        static oglplus::Context gl;
        static App *instance;
        static bool wireframeMode;

        MainWindow * appWindow;
        Terrain terrain;

        App(const App &rhs);
        App();
        // app callbacks
        static void onError(int code, const char * description);
        static void onKeyPress(GLFWwindow *window, int key, int scancode, int action,
                               int mods);
        static void onWindowResize(GLFWwindow *window, int width, int height);
        // app libraries configuration
        void Configure();
        MainWindow * CreateContext();
        // app render loop
        void Start();
    public:
        static App * Instance();
        void Run();

        ~App();
};

