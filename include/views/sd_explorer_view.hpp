#pragma once

#include <borealis.hpp>
#include <string>
#include <vector>
#include <functional>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

using FolderSelectedCallback = function<void(const string& selectedPath)>;

class SdExplorerView : public brls::Box {
public:
    SdExplorerView(const string& initialPath = "", FolderSelectedCallback onSelected = nullptr);
    ~SdExplorerView() override = default;

    void refreshList();
    void navigateTo(const string& path);
    void navigateUp();

    string getCurrentPath() const;
    string getNormalizedSdPath() const;

private:
    string currentPath;
    FolderSelectedCallback onFolderSelected;

    brls::Header* pathHeader = nullptr;
    brls::List* directoryList = nullptr;
    brls::Button* selectCurrentBtn = nullptr;

    void populateEntries();
    string resolveRootPath() const;
};
