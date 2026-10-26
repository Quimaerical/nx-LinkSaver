#include "views/sd_explorer_view.hpp"

#include <algorithm>
#include <iostream>

using namespace std;
namespace fs = std::filesystem;

SdExplorerView::SdExplorerView(const string& initialPath, FolderSelectedCallback onSelected)
    : onFolderSelected(onSelected) {
    this->setOrientation(brls::Axis::COLUMN);
    this->setPadding(20, 20, 20, 20);

    if (initialPath.empty() || !fs::exists(initialPath)) {
        currentPath = resolveRootPath();
    } else {
        currentPath = initialPath;
    }

    pathHeader = new brls::Header();
    this->addView(pathHeader);

    // Button to select current directory
    selectCurrentBtn = new brls::Button();
    selectCurrentBtn->setText("Seleccionar esta carpeta (Y)");
    selectCurrentBtn->getClickEvent()->subscribe([this](brls::View* v) {
        if (onFolderSelected) {
            onFolderSelected(getNormalizedSdPath());
        }
        brls::Application::popActivity();
    });
    this->addView(selectCurrentBtn);

    directoryList = new brls::List();
    this->addView(directoryList);

    // Register Y button action on Nintendo Switch
    this->registerAction("Seleccionar Carpeta", brls::BUTTON_Y, [this](brls::View* v) {
        if (onFolderSelected) {
            onFolderSelected(getNormalizedSdPath());
        }
        brls::Application::popActivity();
        return true;
    });

    // Register B button action to go up
    this->registerAction("Subir Nivel", brls::BUTTON_B, [this](brls::View* v) {
        navigateUp();
        return true;
    });

    populateEntries();
}

string SdExplorerView::resolveRootPath() const {
#ifdef __SWITCH__
    return "sdmc:/";
#else
    if (fs::exists("sdmc:/")) return "sdmc:/";
    return "./";
#endif
}

string SdExplorerView::getCurrentPath() const {
    return currentPath;
}

string SdExplorerView::getNormalizedSdPath() const {
    string p = currentPath;
    if (p.find("sdmc:") == 0) {
        p = p.substr(5); // strip sdmc:
    }
    if (p.empty() || p.front() != '/') {
        p = "/" + p;
    }
    while (p.length() > 1 && p.back() == '/') {
        p.pop_back();
    }
    return p;
}

void SdExplorerView::navigateTo(const string& path) {
    if (fs::exists(path) && fs::is_directory(path)) {
        currentPath = path;
        populateEntries();
    }
}

void SdExplorerView::navigateUp() {
    fs::path p(currentPath);
    if (p.has_parent_path() && p != p.parent_path() && p != "sdmc:/" && p != "sdmc:") {
        currentPath = p.parent_path().string();
        populateEntries();
    } else {
        // Can't go higher, pop activity
        brls::Application::popActivity();
    }
}

void SdExplorerView::refreshList() {
    populateEntries();
}

void SdExplorerView::populateEntries() {
    if (!pathHeader || !directoryList) return;

    pathHeader->setTitle("Explorador SD: " + getNormalizedSdPath());
    directoryList->clear();

    // 1. ".." Go Up entry if not at root
    fs::path cur(currentPath);
    if (cur != "sdmc:/" && cur != "sdmc:" && cur != "./" && cur != "/" && cur.has_parent_path()) {
        brls::ListItem* upItem = new brls::ListItem(".. (Subir un nivel)");
        upItem->getClickEvent()->subscribe([this](brls::View* v) {
            navigateUp();
        });
        directoryList->addView(upItem);
    }

    vector<fs::path> subdirs;
    vector<fs::path> saveFiles;

    try {
        for (const auto& entry : fs::directory_iterator(currentPath, fs::directory_options::skip_permission_denied)) {
            if (entry.is_directory()) {
                subdirs.push_back(entry.path());
            } else if (entry.is_regular_file()) {
                string ext = entry.path().extension().string();
                transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (ext == ".sav" || ext == ".srm" || ext == ".state" || ext == ".dsv") {
                    saveFiles.push_back(entry.path());
                }
            }
        }
    } catch (...) {
        // Handle read error gracefully
    }

    sort(subdirs.begin(), subdirs.end());
    sort(saveFiles.begin(), saveFiles.end());

    // Add subdirectories
    for (const auto& dirPath : subdirs) {
        string folderName = dirPath.filename().string();
        brls::ListItem* item = new brls::ListItem("[Dir] " + folderName);
        string fullPath = dirPath.string();
        item->getClickEvent()->subscribe([this, fullPath](brls::View* v) {
            navigateTo(fullPath);
        });
        directoryList->addView(item);
    }

    // Add save files (for visual confirmation)
    for (const auto& filePath : saveFiles) {
        string fileName = filePath.filename().string();
        brls::ListItem* fileItem = new brls::ListItem("   " + fileName);
        fileItem->setTextColor(brls::Application::getTheme()["brls/text_disabled"]);
        directoryList->addView(fileItem);
    }
}
