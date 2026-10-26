#include "config/config_manager.hpp"
#include "api/url_normalizer.hpp"
#include "nlohmann/json.hpp"

#include <fstream>
#include <filesystem>
#include <sstream>
#include <algorithm>
#include <cctype>

using namespace std;
using json = nlohmann::json;
namespace fs = std::filesystem;

bool DirectoryTag::matchesExtension(const string& filePath) const {
    if (extensions.empty()) return true;

    string ext;
    size_t dotPos = filePath.rfind('.');
    if (dotPos != string::npos) {
        ext = filePath.substr(dotPos);
        transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    }

    for (const auto& allowed : extensions) {
        string allowedLower = allowed;
        transform(allowedLower.begin(), allowedLower.end(), allowedLower.begin(), ::tolower);
        if (allowedLower == ext) {
            return true;
        }
    }
    return false;
}

string DirectoryTag::getExtensionsCommaSeparated() const {
    stringstream ss;
    for (size_t i = 0; i < extensions.size(); ++i) {
        ss << extensions[i];
        if (i + 1 < extensions.size()) ss << ", ";
    }
    return ss.str();
}

void DirectoryTag::setExtensionsFromCommaSeparated(const string& csv) {
    extensions.clear();
    stringstream ss(csv);
    string item;
    while (getline(ss, item, ',')) {
        // trim item
        while (!item.empty() && isspace(static_cast<unsigned char>(item.front()))) item.erase(0, 1);
        while (!item.empty() && isspace(static_cast<unsigned char>(item.back()))) item.pop_back();
        if (!item.empty()) {
            if (item.front() != '.') item = "." + item;
            extensions.push_back(item);
        }
    }
}

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

ConfigManager::ConfigManager() {
    activeConfigPath = resolveDefaultPath();
}

string ConfigManager::resolveDefaultPath() const {
#ifdef __SWITCH__
    return "sdmc:/switch/LinkSaver/config.json";
#else
    // Desktop development / testing fallback
    if (fs::exists("sdmc:/switch/LinkSaver")) {
        return "sdmc:/switch/LinkSaver/config.json";
    }
    return "./config.json";
#endif
}

string ConfigManager::getConfigFilePath() const {
    return activeConfigPath;
}

void ConfigManager::setCustomConfigPath(const string& path) {
    if (!path.empty()) {
        activeConfigPath = path;
    }
}

AppConfig& ConfigManager::getConfig() {
    return config;
}

const AppConfig& ConfigManager::getConfig() const {
    return config;
}

void ConfigManager::setServerUrl(const string& url) {
    config.serverUrl = UrlNormalizer::normalize(url);
}

string ConfigManager::getServerUrl() const {
    return config.serverUrl;
}

void ConfigManager::setAuthToken(const string& token) {
    config.authToken = token;
}

string ConfigManager::getAuthToken() const {
    return config.authToken;
}

void ConfigManager::setTimeoutSeconds(int seconds) {
    config.timeoutSeconds = (seconds > 0) ? seconds : 10;
}

int ConfigManager::getTimeoutSeconds() const {
    return config.timeoutSeconds;
}

void ConfigManager::addDirectory(const DirectoryTag& dir) {
    // If tag already exists, replace it; otherwise append
    for (auto& existing : config.directories) {
        if (existing.tag == dir.tag) {
            existing = dir;
            return;
        }
    }
    config.directories.push_back(dir);
}

bool ConfigManager::updateDirectory(const string& tagId, const DirectoryTag& dir) {
    for (auto& existing : config.directories) {
        if (existing.tag == tagId) {
            existing = dir;
            return true;
        }
    }
    return false;
}

bool ConfigManager::removeDirectory(const string& tagId) {
    auto it = remove_if(config.directories.begin(), config.directories.end(),
                        [&tagId](const DirectoryTag& item) { return item.tag == tagId; });
    if (it != config.directories.end()) {
        config.directories.erase(it, config.directories.end());
        return true;
    }
    return false;
}

optional<DirectoryTag> ConfigManager::getDirectory(const string& tagId) const {
    for (const auto& item : config.directories) {
        if (item.tag == tagId) {
            return item;
        }
    }
    return nullopt;
}

bool ConfigManager::load(const string& customPath) {
    string path = customPath.empty() ? activeConfigPath : customPath;
    if (!fs::exists(path)) {
        return false;
    }

    ifstream inFile(path);
    if (!inFile.is_open()) {
        return false;
    }

    try {
        json j;
        inFile >> j;

        config.serverUrl = j.value("server_url", "");
        config.authToken = j.value("auth_token", "");
        config.timeoutSeconds = j.value("timeout_seconds", 10);
        config.directories.clear();

        if (j.contains("directories") && j["directories"].is_array()) {
            for (const auto& item : j["directories"]) {
                DirectoryTag dt;
                dt.tag = item.value("tag", "");
                dt.displayName = item.value("display_name", "");
                dt.localPath = item.value("local_path", "");
                if (item.contains("extensions") && item["extensions"].is_array()) {
                    for (const auto& ext : item["extensions"]) {
                        dt.extensions.push_back(ext.get<string>());
                    }
                }
                config.directories.push_back(dt);
            }
        }
        return true;
    } catch (...) {
        return false;
    }
}

bool ConfigManager::save(const string& customPath) {
    string path = customPath.empty() ? activeConfigPath : customPath;

    try {
        fs::path p(path);
        if (p.has_parent_path()) {
            fs::create_directories(p.parent_path());
        }

        json j;
        j["server_url"] = config.serverUrl;
        j["auth_token"] = config.authToken;
        j["timeout_seconds"] = config.timeoutSeconds;

        json dirs = json::array();
        for (const auto& dt : config.directories) {
            json item;
            item["tag"] = dt.tag;
            item["display_name"] = dt.displayName;
            item["local_path"] = dt.localPath;
            item["extensions"] = dt.extensions;
            dirs.push_back(item);
        }
        j["directories"] = dirs;

        ofstream outFile(path);
        if (!outFile.is_open()) return false;

        outFile << j.dump(4);
        return true;
    } catch (...) {
        return false;
    }
}
