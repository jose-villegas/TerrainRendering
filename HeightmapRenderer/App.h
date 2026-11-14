#pragma once
#include "MainWindow.h"
#include "Terrain.h"
#include "AppInterface.h"
#include "Camera.h"
#include "LightShafts.h"

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
        LightShafts lightShafts;

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
        // light shafts pass over the terrain drawn since beginScene
        void renderLightShafts(float time);
        // opt-in capture mode (--capture <dir>), orbits the camera around the
        // terrain, switches to wireframe partway and saves every frame as png
        std::string captureDir;
        void setupCapture();
        bool updateCapture(int frame, float &time);
        void saveFrame(int frame);
    public:
        static App * Instance();
        void Run();
        void CaptureTo(const std::string &dir) { captureDir = dir; }
        // starts with the light shafts on, the capture then sweeps the sunset
        void EnableLightShafts() { gui.lightShafts = true; }
        Terrain &getTerrain() { return terrain; }
        Camera &getCamera() { return camera; }
        AppInterface &Gui() { return gui; }

        ~App();
};

