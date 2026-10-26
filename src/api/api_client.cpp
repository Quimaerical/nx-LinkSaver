#include "api/api_client.hpp"
#include "api/url_normalizer.hpp"
#include "nlohmann/json.hpp"

#include <curl/curl.h>
#include <chrono>
#include <fstream>
#include <filesystem>
#include <iostream>

using namespace std;
using json = nlohmann::json;

namespace fs = std::filesystem;

static size_t stringWriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    string* str = static_cast<string*>(userp);
    str->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

static size_t fileWriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    ofstream* stream = static_cast<ofstream*>(userp);
    stream->write(static_cast<char*>(contents), totalSize);
    return stream->good() ? totalSize : 0;
}

struct HeaderData {
    string sha256;
};

static size_t headerCallback(char* buffer, size_t size, size_t nitems, void* userdata) {
    size_t total = size * nitems;
    string header(buffer, total);
    HeaderData* data = static_cast<HeaderData*>(userdata);

    string prefix = "X-Sha256:";
    if (header.find(prefix) == 0) {
        string val = header.substr(prefix.length());
        while (!val.empty() && (val.front() == ' ' || val.front() == '\t')) val.erase(0, 1);
        while (!val.empty() && (val.back() == '\r' || val.back() == '\n' || val.back() == ' ')) val.pop_back();
        data->sha256 = val;
    }
    return total;
}

ApiClient::ApiClient(const string& baseUrl, const string& authToken, int timeoutSeconds)
    : authToken(authToken), timeoutSeconds(timeoutSeconds) {
    setBaseUrl(baseUrl);
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

ApiClient::~ApiClient() {
    // curl_global_cleanup will be handled by the runtime or process exit
}

void ApiClient::setBaseUrl(const string& url) {
    baseUrl = UrlNormalizer::normalize(url);
}

string ApiClient::getBaseUrl() const {
    return baseUrl;
}

void ApiClient::setAuthToken(const string& token) {
    authToken = token;
}

string ApiClient::getAuthToken() const {
    return authToken;
}

void ApiClient::setTimeout(int seconds) {
    timeoutSeconds = (seconds > 0) ? seconds : 10;
}

int ApiClient::getTimeout() const {
    return timeoutSeconds;
}

string ApiClient::buildUrl(const string& endpoint) const {
    if (baseUrl.empty()) return "";
    if (!endpoint.empty() && endpoint.front() == '/') {
        return baseUrl + endpoint;
    }
    return baseUrl + "/" + endpoint;
}

void ApiClient::configureHeaders(void* curlHandle, void** headerList) const {
    CURL* curl = static_cast<CURL*>(curlHandle);
    struct curl_slist** list = reinterpret_cast<struct curl_slist**>(headerList);

    *list = curl_slist_append(*list, "User-Agent: LinkSaver-Switch/1.0.0");
    *list = curl_slist_append(*list, "X-Client-Type: Switch");

    if (!authToken.empty()) {
        string authHeader = "Authorization: Bearer " + authToken;
        *list = curl_slist_append(*list, authHeader.c_str());
    }

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, *list);
}

PingResult ApiClient::ping() {
    PingResult result;
    if (baseUrl.empty()) {
        result.error = "Server URL is empty";
        return result;
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        result.error = "Failed to initialize cURL handle";
        return result;
    }

    string url = buildUrl("/api/v1/ping");
    string responseBody;
    struct curl_slist* headers = nullptr;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, static_cast<long>(timeoutSeconds));
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, static_cast<long>(timeoutSeconds));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, stringWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    configureHeaders(curl, reinterpret_cast<void**>(&headers));

    auto start = chrono::high_resolution_clock::now();
    CURLcode res = curl_easy_perform(curl);
    auto end = chrono::high_resolution_clock::now();

    result.latencyMs = chrono::duration_cast<chrono::milliseconds>(end - start).count();

    if (res == CURLE_OK) {
        long httpCode = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        if (httpCode == 200) {
            try {
                auto j = json::parse(responseBody);
                result.success = true;
                result.version = j.value("version", "1.0.0");
                result.serverTime = j.value("server_time", 0L);
            } catch (const exception& e) {
                result.error = string("JSON parse error: ") + e.what();
            }
        } else {
            result.error = "HTTP error " + to_string(httpCode);
        }
    } else {
        result.error = curl_easy_strerror(res);
    }

    if (headers) curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return result;
}

bool ApiClient::registerDirectory(const string& tag, const string& displayName, const string& description) {
    if (baseUrl.empty() || tag.empty()) return false;

    CURL* curl = curl_easy_init();
    if (!curl) return false;

    string url = buildUrl("/api/v1/directories/register");
    string responseBody;
    struct curl_slist* headers = nullptr;

    json payload = {
        {"tag", tag},
        {"display_name", displayName.empty() ? tag : displayName},
        {"description", description}
    };
    string jsonStr = payload.dump();

    headers = curl_slist_append(headers, "Content-Type: application/json");
    configureHeaders(curl, reinterpret_cast<void**>(&headers));

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonStr.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, static_cast<long>(timeoutSeconds));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, stringWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);

    CURLcode res = curl_easy_perform(curl);
    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return (res == CURLE_OK && httpCode == 200);
}

ManifestResult ApiClient::getManifest(const string& tag) {
    ManifestResult result;
    if (baseUrl.empty() || tag.empty()) {
        result.error = "Base URL or tag is empty";
        return result;
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        result.error = "Failed to initialize cURL handle";
        return result;
    }

    string url = buildUrl("/api/v1/sync/manifest/" + tag);
    string responseBody;
    struct curl_slist* headers = nullptr;

    configureHeaders(curl, reinterpret_cast<void**>(&headers));

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, static_cast<long>(timeoutSeconds));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, stringWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);

    CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK) {
        long httpCode = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        if (httpCode == 200) {
            try {
                auto j = json::parse(responseBody);
                result.success = true;
                result.tag = j.value("tag", tag);
                result.displayName = j.value("display_name", "");
                result.storagePath = j.value("storage_path", "");
                result.serverTime = j.value("server_time", 0L);

                if (j.contains("files") && j["files"].is_array()) {
                    for (const auto& item : j["files"]) {
                        ManifestFile mf;
                        mf.relativePath = item.value("relative_path", "");
                        mf.fileName = item.value("file_name", "");
                        mf.sha256 = item.value("sha256_hash", "");
                        mf.sizeBytes = item.value("file_size_bytes", 0L);
                        mf.version = item.value("version", 1);
                        result.files.push_back(mf);
                    }
                }
            } catch (const exception& e) {
                result.error = string("JSON parse error: ") + e.what();
            }
        } else {
            result.error = "HTTP error " + to_string(httpCode);
        }
    } else {
        result.error = curl_easy_strerror(res);
    }

    if (headers) curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return result;
}

UploadResult ApiClient::uploadSave(const string& tag, const string& localFilePath, 
                                   const string& relativePath, time_t remoteModifiedAt) {
    UploadResult result;
    if (baseUrl.empty() || tag.empty() || localFilePath.empty()) {
        result.error = "Invalid arguments for upload";
        return result;
    }

    if (!fs::exists(localFilePath)) {
        result.error = "Local file does not exist: " + localFilePath;
        return result;
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        result.error = "Failed to initialize cURL handle";
        return result;
    }

    string url = buildUrl("/api/v1/sync/upload/" + tag);
    string responseBody;
    struct curl_slist* headers = nullptr;
    curl_mime* mime = curl_mime_init(curl);

    // Form field: file
    curl_mimepart* partFile = curl_mime_addpart(mime);
    curl_mime_name(partFile, "file");
    curl_mime_filedata(partFile, localFilePath.c_str());

    // Form field: relative_path
    curl_mimepart* partRel = curl_mime_addpart(mime);
    curl_mime_name(partRel, "relative_path");
    curl_mime_data(partRel, relativePath.c_str(), CURL_ZERO_TERMINATED);

    // Form field: client_type
    curl_mimepart* partClient = curl_mime_addpart(mime);
    curl_mime_name(partClient, "client_type");
    curl_mime_data(partClient, "Switch", CURL_ZERO_TERMINATED);

    // Form field: remote_modified_at
    string timeStr = to_string(remoteModifiedAt);
    curl_mimepart* partTime = curl_mime_addpart(mime);
    curl_mime_name(partTime, "remote_modified_at");
    curl_mime_data(partTime, timeStr.c_str(), CURL_ZERO_TERMINATED);

    configureHeaders(curl, reinterpret_cast<void**>(&headers));

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, static_cast<long>(timeoutSeconds * 3)); // uploads can take longer
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, stringWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);

    CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK) {
        long httpCode = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        if (httpCode == 200) {
            try {
                auto j = json::parse(responseBody);
                result.success = true;
                result.collisionAction = j.value("collision_action", "none");
                result.message = j.value("message", "Uploaded successfully");
            } catch (const exception& e) {
                result.error = string("JSON parse error: ") + e.what();
            }
        } else {
            result.error = "HTTP error " + to_string(httpCode) + ": " + responseBody;
        }
    } else {
        result.error = curl_easy_strerror(res);
    }

    curl_mime_free(mime);
    if (headers) curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return result;
}

DownloadResult ApiClient::downloadSave(const string& tag, const string& relativePath, 
                                       const string& localDestPath, bool allowResume,
                                       DownloadProgressCallback progressCb) {
    DownloadResult result;
    if (baseUrl.empty() || tag.empty() || relativePath.empty() || localDestPath.empty()) {
        result.error = "Invalid arguments for download";
        return result;
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        result.error = "Failed to initialize cURL handle";
        return result;
    }

    // Ensure destination parent directory exists
    fs::path dest(localDestPath);
    if (dest.has_parent_path()) {
        fs::create_directories(dest.parent_path());
    }

    // Temporary partial file path
    string partPath = localDestPath + ".linksaver_part";
    int64_t existingBytes = 0;

    if (allowResume && fs::exists(partPath)) {
        try {
            existingBytes = fs::file_size(partPath);
        } catch (...) {
            existingBytes = 0;
        }
    }

    ios_base::openmode openMode = ios::binary | ios::out;
    if (existingBytes > 0 && allowResume) {
        openMode |= ios::app;
        result.resumed = true;
    } else {
        openMode |= ios::trunc;
        existingBytes = 0;
    }

    ofstream outFile(partPath, openMode);
    if (!outFile.is_open()) {
        result.error = "Failed to open temporary file: " + partPath;
        curl_easy_cleanup(curl);
        return result;
    }

    char* escapedPath = curl_easy_escape(curl, relativePath.c_str(), static_cast<int>(relativePath.length()));
    string url = buildUrl("/api/v1/sync/download/" + tag + "?path=" + (escapedPath ? escapedPath : relativePath));
    if (escapedPath) curl_free(escapedPath);

    struct curl_slist* headers = nullptr;
    HeaderData headerData;

    configureHeaders(curl, reinterpret_cast<void**>(&headers));

    if (existingBytes > 0) {
        curl_easy_setopt(curl, CURLOPT_RESUME_FROM_LARGE, static_cast<curl_off_t>(existingBytes));
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, static_cast<long>(timeoutSeconds * 3));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fileWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &outFile);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, headerCallback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &headerData);

    CURLcode res = curl_easy_perform(curl);
    outFile.close();

    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

    if (headers) curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    // 200 OK or 206 Partial Content
    if (res == CURLE_OK && (httpCode == 200 || httpCode == 206)) {
        try {
            // Atomic rename from partial to final target destination
            fs::rename(partPath, localDestPath);
            result.success = true;
            result.bytesDownloaded = fs::file_size(localDestPath);
            result.sha256 = headerData.sha256;
            return result;
        } catch (const exception& e) {
            result.error = string("Failed to rename downloaded file: ") + e.what();
        }
    } else {
        result.error = (res != CURLE_OK) ? curl_easy_strerror(res) : ("HTTP error " + to_string(httpCode));
        
        // Garbage Collection: If resume is disabled or error is fatal, clean up temporary file
        if (!allowResume || httpCode == 404 || httpCode == 400 || httpCode == 401) {
            try {
                if (fs::exists(partPath)) {
                    fs::remove(partPath);
                }
            } catch (...) {}
        }
    }

    return result;
}

void ApiClient::cleanupOrphanedTempFiles(const string& folderPath) {
    if (folderPath.empty() || !fs::exists(folderPath)) return;

    try {
        for (const auto& entry : fs::recursive_directory_iterator(folderPath, fs::directory_options::skip_permission_denied)) {
            if (entry.is_regular_file()) {
                string name = entry.path().filename().string();
                if (name.rfind(".linksaver_part", name.length() - 15) != string::npos ||
                    name.find(".linksaver_tmp_") == 0) {
                    fs::remove(entry.path());
                }
            }
        }
    } catch (...) {
        // Silently tolerate permission/read errors
    }
}
