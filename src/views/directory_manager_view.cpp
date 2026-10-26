#include "views/directory_manager_view.hpp"
#include "views/directory_edit_view.hpp"
#include "config/config_manager.hpp"

using namespace std;

DirectoryManagerView::DirectoryManagerView() {
    this->setOrientation(brls::Axis::COLUMN);
    this->setPadding(20, 20, 20, 20);

    brls::Header* header = new brls::Header();
    header->setTitle("Carpetas de Saves (Etiquetas)");
    header->setSubtitle("Mapea tus carpetas de emuladores para sincronización automática");
    this->addView(header);

    addBtn = new brls::Button();
    addBtn->setText("+ Añadir Nuevo Mapeo de Carpeta");
    addBtn->getClickEvent()->subscribe([this](brls::View* v) { onAddNew(); });
    this->addView(addBtn);

    list = new brls::List();
    list->setMarginTop(15);
    this->addView(list);

    refreshList();
}

void DirectoryManagerView::refreshList() {
    if (!list) return;
    list->clear();

    const auto& directories = ConfigManager::getInstance().getConfig().directories;

    if (directories.empty()) {
        brls::ListItem* emptyItem = new brls::ListItem("No hay carpetas configuradas aún. Pulsa el botón de arriba.");
        emptyItem->setTextColor(brls::Application::getTheme()["brls/text_disabled"]);
        list->addView(emptyItem);
        return;
    }

    for (const auto& dt : directories) {
        string title = dt.displayName.empty() ? dt.tag : dt.displayName;
        brls::ListItem* item = new brls::ListItem(title);
        item->setValue(dt.tag + "  ->  " + dt.localPath);

        string tagId = dt.tag;
        item->getClickEvent()->subscribe([this, tagId](brls::View* v) {
            onEditDirectory(tagId);
        });
        list->addView(item);
    }
}

void DirectoryManagerView::onAddNew() {
    DirectoryTag newTag;
    newTag.extensions = {".sav", ".srm"};

    DirectoryEditView* editView = new DirectoryEditView(
        newTag,
        true,
        [this]() { refreshList(); }
    );
    brls::Application::pushActivity(new brls::Activity(editView));
}

void DirectoryManagerView::onEditDirectory(const string& tagId) {
    auto tagOpt = ConfigManager::getInstance().getDirectory(tagId);
    if (!tagOpt.has_value()) return;

    DirectoryEditView* editView = new DirectoryEditView(
        tagOpt.value(),
        false,
        [this]() { refreshList(); }
    );
    brls::Application::pushActivity(new brls::Activity(editView));
}
