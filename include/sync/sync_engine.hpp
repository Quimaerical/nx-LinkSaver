#pragma once

#include "api/api_client.hpp"
#include "config/config_manager.hpp"

#include <string>
#include <vector>
#include <functional>

using namespace std;

struct SyncStats {
    int uploaded = 0;
    int downloaded = 0;
    int identical = 0;
    int conflicts = 0;
    vector<string> logMessages;
    vector<string> errors;

    bool hasErrors() const { return !errors.empty(); }
};

using StatusCallback = function<void(const string& statusText)>;
using ProgressCallback = function<void(int currentStep, int totalSteps)>;
using LogCallback = function<void(const string& logLine)>;

class SyncEngine {
public:
    SyncEngine(ApiClient& apiClient, ConfigManager& configManager);

    // Executes full bidirectional synchronization across all mapped directories
    SyncStats syncAll(StatusCallback statusCb = nullptr, 
                      ProgressCallback progressCb = nullptr,
                      LogCallback logCb = nullptr);

    // Synchronizes a single directory tag
    SyncStats syncTag(const DirectoryTag& dir,
                      StatusCallback statusCb = nullptr,
                      LogCallback logCb = nullptr);

private:
    ApiClient& client;
    ConfigManager& configMgr;

    string resolveLocalFullPath(const string& sdmcRelativePath) const;
};
