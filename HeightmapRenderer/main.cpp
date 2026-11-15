#include "Commons.h"
#include "App.h"

// laptops with two GPUs run this on the discrete one instead of the
// integrated one; the drivers look these exports up in the executable
extern "C"
{
    __declspec(dllexport) unsigned long NvOptimusEnablement = 1;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

int main(int argc, char **argv)
{
    std::unique_ptr<App> app(App::Instance());

    // opt-in: --capture <dir> records an orbit of the terrain as png frames,
    // --light-shafts starts with the light shafts on (a sunset capture)
    for(int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];

        if(arg == "--capture" && i + 1 < argc) app->CaptureTo(argv[i + 1]);
        else if(arg == "--light-shafts") app->EnableLightShafts();
    }

    // start app
    app->Run();
}
