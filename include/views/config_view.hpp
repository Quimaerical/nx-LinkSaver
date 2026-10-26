#pragma once

#include <borealis.hpp>
#include <string>

using namespace std;

class ConfigView : public brls::Box {
public:
    ConfigView();
    ~ConfigView() override = default;

    void refreshValues();

private:
    brls::ListItem* urlItem = nullptr;
    brls::ListItem* tokenItem = nullptr;
    brls::ListItem* timeoutItem = nullptr;
    brls::Button* pingBtn = nullptr;
    brls::Button* saveBtn = nullptr;
    brls::Label* statusLabel = nullptr;

    void onEditUrl();
    void onEditToken();
    void onTestPing();
    void onSave();
};
