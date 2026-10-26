#include <borealis.hpp>

#ifdef __SWITCH__
#include <switch.h>
#endif

#include "config/config_manager.hpp"
#include "views/main_tab_view.hpp"

#include <iostream>

using namespace std;

int main(int argc, char* argv[]) {
#ifdef __SWITCH__
    // Initialize Horizon OS network sockets and virtual ROM filesystem
    socketInitializeDefault();
    romfsInit();
#endif

    // Set Borealis logging
    brls::Logger::setLogLevel(brls::LogLevel::LOG_INFO);

    // Initialize Borealis Application
    if (!brls::Application::init("LinkSaver")) {
        brls::Logger::error("Failed to initialize Borealis application");
        return EXIT_FAILURE;
    }

    // Load persisted settings from config.json
    ConfigManager::getInstance().load();

    // Create root TabFrame
    MainTabView* mainTab = new MainTabView();
    brls::Application::pushActivity(new brls::Activity(mainTab));

    // Run main render and event loop
    while (brls::Application::mainLoop()) {
        // Borealis handles events, controller input, animations, and rendering at 60 FPS
    }

#ifdef __SWITCH__
    // Clean up Horizon OS resources
    romfsExit();
    socketExit();
#endif

    return EXIT_SUCCESS;
}
