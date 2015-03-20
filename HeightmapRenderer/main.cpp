#include "Commons.h"
#include "App.h"

int main(void)
{
    std::unique_ptr<App> app(App::Instance());
    // start app
    app->Run();
}