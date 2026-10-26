#include "views/directory_edit_view.hpp"
#include "views/sd_explorer_view.hpp"
#include "platform/software_keyboard.hpp"

using namespace std;

DirectoryEditView::DirectoryEditView(const DirectoryTag& tag, bool isNew, DirectorySavedCallback onSaved)
    : currentTag(tag), originalTagId(tag.tag), isNewMapping(isNew), onSavedCallback(onSaved) {
    this->setOrientation(brls::Axis::COLUMN);
    this->setPadding(20, 20, 20, 20);

    brls::Header* header = new brls::Header();
    header->setTitle(isNew ? "Añadir Nuevo Mapeo de Carpeta" : "Editar Mapeo de Carpeta");
    header->setSubtitle("Vincula una ruta física en la microSD con una etiqueta lógica del servidor");
    this->addView(header);

    brls::List* list = new brls::List();

    // 1. Tag ID
    tagItem = new brls::ListItem("Etiqueta Lógica (Tag)");
    tagItem->getClickEvent()->subscribe([this](brls::View* v) { onEditTag(); });
    list->addView(tagItem);

    // 2. Display Name
    nameItem = new brls::ListItem("Nombre Descriptivo");
    nameItem->getClickEvent()->subscribe([this](brls::View* v) { onEditName(); });
    list->addView(nameItem);

    // 3. Local SD Path (Opens SD Explorer!)
    pathItem = new brls::ListItem("Ruta Local en microSD");
    pathItem->getClickEvent()->subscribe([this](brls::View* v) { onBrowsePath(); });
    list->addView(pathItem);

    // 4. Allowed Extensions
    extItem = new brls::ListItem("Extensiones Permitidas");
    extItem->getClickEvent()->subscribe([this](brls::View* v) { onEditExtensions(); });
    list->addView(extItem);

    this->addView(list);

    // Actions Box
    brls::Box* actionsBox = new brls::Box();
    actionsBox->setOrientation(brls::Axis::ROW);
    actionsBox->setMarginTop(15);

    saveBtn = new brls::Button();
    saveBtn->setText("Guardar Mapeo");
    saveBtn->getClickEvent()->subscribe([this](brls::View* v) { onSave(); });
    actionsBox->addView(saveBtn);

    if (!isNewMapping) {
        deleteBtn = new brls::Button();
        deleteBtn->setText("Eliminar Mapeo");
        deleteBtn->getClickEvent()->subscribe([this](brls::View* v) { onDelete(); });
        actionsBox->addView(deleteBtn);
    }

    this->addView(actionsBox);

    refreshView();
}

void DirectoryEditView::refreshView() {
    if (tagItem) {
        tagItem->setValue(currentTag.tag.empty() ? "(Toca para ingresar tag)" : currentTag.tag);
    }
    if (nameItem) {
        nameItem->setValue(currentTag.displayName.empty() ? "(Toca para ingresar nombre)" : currentTag.displayName);
    }
    if (pathItem) {
        pathItem->setValue(currentTag.localPath.empty() ? "(Toca para explorar SD)" : currentTag.localPath);
    }
    if (extItem) {
        string exts = currentTag.getExtensionsCommaSeparated();
        extItem->setValue(exts.empty() ? ".sav, .srm" : exts);
    }
}

void DirectoryEditView::onEditTag() {
    string entered = SoftwareKeyboard::prompt(
        "Ingresa la etiqueta lógica única (ej. retroarch-cores, azahar):",
        currentTag.tag,
        64
    );
    if (!entered.empty()) {
        currentTag.tag = entered;
        if (currentTag.displayName.empty()) {
            currentTag.displayName = entered;
        }
        refreshView();
    }
}

void DirectoryEditView::onEditName() {
    string entered = SoftwareKeyboard::prompt(
        "Ingresa el nombre descriptivo (ej. RetroArch Cores, Azahar 3DS):",
        currentTag.displayName,
        64
    );
    if (!entered.empty()) {
        currentTag.displayName = entered;
        refreshView();
    }
}

void DirectoryEditView::onBrowsePath() {
    // Open SdExplorerView modally
    SdExplorerView* explorer = new SdExplorerView(
        currentTag.localPath,
        [this](const string& chosenPath) {
            currentTag.localPath = chosenPath;
            refreshView();
        }
    );
    brls::Application::pushActivity(new brls::Activity(explorer));
}

void DirectoryEditView::onEditExtensions() {
    string current = currentTag.getExtensionsCommaSeparated();
    string entered = SoftwareKeyboard::prompt(
        "Ingresa extensiones separadas por coma (ej. .sav, .srm, .state):",
        current,
        128
    );
    currentTag.setExtensionsFromCommaSeparated(entered);
    refreshView();
}

void DirectoryEditView::onSave() {
    if (currentTag.tag.empty()) {
        brls::Dialog* d = new brls::Dialog("El tag no puede estar vacío.");
        d->addButton("Aceptar", []() {});
        d->open();
        return;
    }

    if (currentTag.localPath.empty()) {
        brls::Dialog* d = new brls::Dialog("Debes seleccionar una ruta local en la tarjeta microSD.");
        d->addButton("Aceptar", []() {});
        d->open();
        return;
    }

    if (currentTag.extensions.empty()) {
        currentTag.extensions = {".sav", ".srm"};
    }

    auto& mgr = ConfigManager::getInstance();
    if (isNewMapping) {
        mgr.addDirectory(currentTag);
    } else {
        mgr.updateDirectory(originalTagId, currentTag);
    }
    mgr.save();

    if (onSavedCallback) {
        onSavedCallback();
    }
    brls::Application::popActivity();
}

void DirectoryEditView::onDelete() {
    brls::Dialog* dialog = new brls::Dialog("¿Estás seguro de que deseas eliminar este mapeo de carpeta?");
    dialog->addButton("Cancelar", []() {});
    dialog->addButton("Eliminar", [this]() {
        auto& mgr = ConfigManager::getInstance();
        mgr.removeDirectory(originalTagId);
        mgr.save();
        if (onSavedCallback) {
            onSavedCallback();
        }
        brls::Application::popActivity();
    });
    dialog->open();
}
