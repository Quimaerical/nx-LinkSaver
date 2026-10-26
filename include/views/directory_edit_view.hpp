#pragma once

#include <borealis.hpp>
#include "config/config_manager.hpp"

using namespace std;

using DirectorySavedCallback = function<void()>;

class DirectoryEditView : public brls::Box {
public:
    DirectoryEditView(const DirectoryTag& tag, bool isNew = false, DirectorySavedCallback onSaved = nullptr);
    ~DirectoryEditView() override = default;

    void refreshView();

private:
    DirectoryTag currentTag;
    string originalTagId;
    bool isNewMapping;
    DirectorySavedCallback onSavedCallback;

    brls::ListItem* tagItem = nullptr;
    brls::ListItem* nameItem = nullptr;
    brls::ListItem* pathItem = nullptr;
    brls::ListItem* extItem = nullptr;
    brls::Button* saveBtn = nullptr;
    brls::Button* deleteBtn = nullptr;

    void onEditTag();
    void onEditName();
    void onBrowsePath();
    void onEditExtensions();
    void onSave();
    void onDelete();
};
