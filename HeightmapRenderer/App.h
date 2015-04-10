#pragma once
#include "MainWindow.h"
#include "Terrain.h"
#include "AppInterface.h"
#include "Camera.h"

class App
{
    private:
        static oglplus::Context gl;
        static App *instance;
        bool moveAround = false;

        MainWindow * appWindow;
        Camera camera;
        Terrain terrain;
        AppInterface gui;

        void handleUserInput(GLFWwindow * window, float deltaTime);

        App(const App &rhs);
        App();
        // app callbacks
        static void onError(int code, const char * description);
        static void onKeyPress(GLFWwindow *window, int key, int scancode, int action,
                               int mods);
        static void onMouseWheel(GLFWwindow *window, double xoffset, double yoffset);
        static void onWindowResize(GLFWwindow *window, int width, int height);
        // app libraries configuration
        void Configure();
        MainWindow * CreateContext();
        // app render loop
        void Start();
    public:
        static App * Instance();
        void Run();
        Terrain &getTerrain() { return terrain; }
        Camera &getCamera() { return camera; }
        AppInterface &Gui() { return gui; }

        ~App();
};

