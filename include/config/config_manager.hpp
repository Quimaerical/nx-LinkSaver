#pragma once

#include <string>
#include <vector>
#include <optional>

using namespace std;

struct DirectoryTag {
    string tag;
    string displayName;
    string localPath;
    vector<string> extensions; // e.g. {".sav", ".srm", ".state"}

    // Helper: checks if a given file path matches any allowed extension
    bool matchesExtension(const string& filePath) const;
    string getExtensionsCommaSeparated() const;
    void setExtensionsFromCommaSeparated(const string& csv);
};

struct AppConfig {
    string serverUrl;
    string authToken;
    int timeoutSeconds = 10;
    vector<DirectoryTag> directories;
};

class ConfigManager {
public:
    static ConfigManager& getInstance();

    bool load(const string& customPath = "");
    bool save(const string& customPath = "");

    AppConfig& getConfig();
    const AppConfig& getConfig() const;

    void setServerUrl(const string& url);
    string getServerUrl() const;

    void setAuthToken(const string& token);
    string getAuthToken() const;

    void setTimeoutSeconds(int seconds);
    int getTimeoutSeconds() const;

    // Directory Tag CRUD operations
    void addDirectory(const DirectoryTag& dir);
    bool updateDirectory(const string& tagId, const DirectoryTag& dir);
    bool removeDirectory(const string& tagId);
    optional<DirectoryTag> getDirectory(const string& tagId) const;

    string getConfigFilePath() const;
    void setCustomConfigPath(const string& path);

private:
    ConfigManager();
    ~ConfigManager() = default;

    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    AppConfig config;
    string activeConfigPath;

    string resolveDefaultPath() const;
};
