#include "Commons.h"
#include "App.h"

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
