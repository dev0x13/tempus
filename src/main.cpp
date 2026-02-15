#include "app/Application.hpp"
#include <iostream>

#ifdef _WIN32
#pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
#endif

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
