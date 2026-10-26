#include "views/main_tab_view.hpp"
#include "views/sync_view.hpp"
#include "views/directory_manager_view.hpp"
#include "views/config_view.hpp"

MainTabView::MainTabView() {
    this->setTitle("LinkSaver");
    this->setIcon("romfs:/icon.png");

    // Tab 1: Sincronizar
    this->addTab("Sincronizar", new SyncView());

    // Tab 2: Carpetas de Saves
    this->addTab("Carpetas de Saves", new DirectoryManagerView());

    // Tab 3: Ajustes
    this->addTab("Ajustes", new ConfigView());
}
