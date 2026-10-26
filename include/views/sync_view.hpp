#pragma once

#include <borealis.hpp>
#include "sync/sync_engine.hpp"

using namespace std;

class SyncView : public brls::Box {
public:
    SyncView();
    ~SyncView() override = default;

    void refreshStatus();

private:
    brls::Button* syncBtn = nullptr;
    brls::Label* statusLabel = nullptr;
    brls::Label* statsLabel = nullptr;
    brls::List* logList = nullptr;

    bool isSyncing = false;

    void onStartSync();
    void addLogLine(const string& line);
};
