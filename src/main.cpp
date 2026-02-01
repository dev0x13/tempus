#include "app/Application.hpp"
#include <iostream>

int main() {
    try {
        timetracker::app::Application app;

        if (!app.init()) {
            std::cerr << "Failed to initialize application" << std::endl;
            return 1;
        }

        app.run();

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}

#ifdef _WIN32
int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
    return main();
}
#endif