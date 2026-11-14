#include "Commons.h"
#include "App.h"

int main(int argc, char **argv)
{
    std::unique_ptr<App> app(App::Instance());

    // opt-in: --capture <dir> records an orbit of the terrain as png frames
    for(int i = 1; i + 1 < argc; i++)
    {
        if(std::string(argv[i]) == "--capture") app->CaptureTo(argv[i + 1]);
    }

    // start app
    app->Run();
}
