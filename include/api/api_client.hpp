#pragma once

#include <string>
#include <vector>
#include <functional>
#include <ctime>
#include <cstdint>

using namespace std;

struct PingResult {
    bool success = false;
    string version;
    long serverTime = 0;
    long latencyMs = 0;
    string error;
};

struct ManifestFile {
    string relativePath;
    string fileName;
    string sha256;
    int64_t sizeBytes = 0;
    time_t remoteModifiedAt = 0;
    int version = 1;
};

struct ManifestResult {
    bool success = false;
    string tag;
    string displayName;
    string storagePath;
    long serverTime = 0;
    vector<ManifestFile> files;
    string error;
};

struct UploadResult {
    bool success = false;
    string collisionAction; // "none", "displaced_previous", "saved_as_conflict"
    string message;
    string error;
};

struct DownloadResult {
    bool success = false;
    int64_t bytesDownloaded = 0;
    bool resumed = false;
    string sha256;
    string error;
};

using DownloadProgressCallback = function<void(int64_t downloadedBytes, int64_t totalBytes)>;

class ApiClient {
public:
    ApiClient(const string& baseUrl, const string& authToken = "", int timeoutSeconds = 10);
    ~ApiClient();

    // Configuration
    void setBaseUrl(const string& baseUrl);
    string getBaseUrl() const;

    void setAuthToken(const string& token);
    string getAuthToken() const;

    void setTimeout(int seconds);
    int getTimeout() const;

    // Core REST Endpoints
    PingResult ping();

    bool registerDirectory(const string& tag, const string& displayName, const string& description = "");

    ManifestResult getManifest(const string& tag);

    UploadResult uploadSave(const string& tag, const string& localFilePath, 
                            const string& relativePath, time_t remoteModifiedAt);

    DownloadResult downloadSave(const string& tag, const string& relativePath, 
                                const string& localDestPath, bool allowResume = true,
                                DownloadProgressCallback progressCb = nullptr);

    // Garbage Collection: Purgers of orphaned temporary/partial files
    static void cleanupOrphanedTempFiles(const string& folderPath);

private:
    string baseUrl;
    string authToken;
    int timeoutSeconds;

    string buildUrl(const string& endpoint) const;
    void configureHeaders(void* curlHandle, void** headerList) const;
};
