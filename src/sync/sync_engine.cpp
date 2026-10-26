#include "sync/sync_engine.hpp"
#include "crypto/sha256.hpp"

#include <filesystem>
#include <map>
#include <iostream>

using namespace std;
namespace fs = std::filesystem;

SyncEngine::SyncEngine(ApiClient& apiClient, ConfigManager& configManager)
    : client(apiClient), configMgr(configManager) {
}

string SyncEngine::resolveLocalFullPath(const string& sdmcRelativePath) const {
    string p = sdmcRelativePath;
    if (p.find("sdmc:/") == 0) {
        return p;
    }
#ifdef __SWITCH__
    if (!p.empty() && p.front() == '/') {
        return "sdmc:" + p;
    }
    return "sdmc:/" + p;
#else
    // Desktop development fallback: if sdmc/ folder exists, use it, else relative
    if (p.find('/') == 0) {
        p = p.substr(1);
    }
    return "./" + p;
#endif
}

SyncStats SyncEngine::syncAll(StatusCallback statusCb, ProgressCallback progressCb, LogCallback logCb) {
    SyncStats totalStats;
    const auto& directories = configMgr.getConfig().directories;

    if (directories.empty()) {
        if (logCb) logCb("No hay carpetas configuradas para sincronizar.");
        if (statusCb) statusCb("Sin carpetas configuradas.");
        return totalStats;
    }

    if (statusCb) statusCb("Verificando conexión con el servidor...");
    PingResult pingRes = client.ping();
    if (!pingRes.success) {
        string err = "Error de conexión: " + pingRes.error;
        if (logCb) logCb(err);
        if (statusCb) statusCb("Servidor inalcanzable.");
        totalStats.errors.push_back(err);
        return totalStats;
    }

    int totalDirs = static_cast<int>(directories.size());
    for (int i = 0; i < totalDirs; ++i) {
        const auto& dir = directories[i];
        if (progressCb) progressCb(i + 1, totalDirs);

        SyncStats tagStats = syncTag(dir, statusCb, logCb);
        totalStats.uploaded += tagStats.uploaded;
        totalStats.downloaded += tagStats.downloaded;
        totalStats.identical += tagStats.identical;
        totalStats.conflicts += tagStats.conflicts;
        for (const auto& err : tagStats.errors) {
            totalStats.errors.push_back(err);
        }
    }

    if (statusCb) {
        if (totalStats.hasErrors()) {
            statusCb("Sincronización completada con algunos errores.");
        } else {
            statusCb("¡Sincronización completada con éxito!");
        }
    }
    return totalStats;
}

SyncStats SyncEngine::syncTag(const DirectoryTag& dir, StatusCallback statusCb, LogCallback logCb) {
    SyncStats stats;
    string tag = dir.tag;
    string localRoot = resolveLocalFullPath(dir.localPath);

    if (statusCb) statusCb("Sincronizando tag: " + tag);
    if (logCb) logCb("[" + tag + "] Iniciando sincronización en " + localRoot);

    // 1. Purge any leftover or orphaned temporary files
    ApiClient::cleanupOrphanedTempFiles(localRoot);

    // 2. Ensure directory exists on the Go backend
    client.registerDirectory(dir.tag, dir.displayName);

    // 3. Ensure local folder exists
    try {
        if (!fs::exists(localRoot)) {
            fs::create_directories(localRoot);
        }
    } catch (const exception& e) {
        string err = "[" + tag + "] Error al crear carpeta local: " + e.what();
        if (logCb) logCb(err);
        stats.errors.push_back(err);
        return stats;
    }

    // 4. Fetch Remote Manifest from Go backend
    ManifestResult manifest = client.getManifest(tag);
    if (!manifest.success) {
        string err = "[" + tag + "] Error al obtener manifiesto: " + manifest.error;
        if (logCb) logCb(err);
        stats.errors.push_back(err);
        return stats;
    }

    map<string, ManifestFile> remoteFiles;
    for (const auto& rf : manifest.files) {
        remoteFiles[rf.relativePath] = rf;
    }

    // 5. Scan local files matching allowed extensions
    map<string, string> localFiles; // relativePath -> fullPath
    try {
        for (const auto& entry : fs::recursive_directory_iterator(localRoot, fs::directory_options::skip_permission_denied)) {
            if (entry.is_regular_file()) {
                string fullPath = entry.path().string();
                if (dir.matchesExtension(fullPath)) {
                    // Compute path relative to localRoot
                    string relPath = fs::relative(entry.path(), localRoot).string();
                    localFiles[relPath] = fullPath;
                }
            }
        }
    } catch (const exception& e) {
        string err = "[" + tag + "] Error al escanear carpeta local: " + e.what();
        if (logCb) logCb(err);
        stats.errors.push_back(err);
    }

    // 6. Compare and Upload local files
    for (const auto& [relPath, fullPath] : localFiles) {
        string localHash = Sha256::calculateFileHash(fullPath);
        time_t localModTime = 0;
        try {
            auto ftime = fs::last_write_time(fullPath);
            localModTime = chrono::system_clock::to_time_t(
                chrono::file_clock::to_sys(ftime)
            );
        } catch (...) {
            localModTime = time(nullptr);
        }

        auto it = remoteFiles.find(relPath);
        if (it != remoteFiles.end()) {
            // File exists both locally and remotely
            if (it->second.sha256 == localHash) {
                stats.identical++;
                continue; // Identical, up to date!
            }
        }

        // Upload to server (Server applies Save Collision Strategy!)
        if (logCb) logCb("[" + tag + "] Subiendo: " + relPath);
        UploadResult upRes = client.uploadSave(tag, fullPath, relPath, localModTime);
        if (upRes.success) {
            stats.uploaded++;
            if (upRes.collisionAction == "displaced_previous" || upRes.collisionAction == "saved_as_conflict") {
                stats.conflicts++;
                if (logCb) logCb("[" + tag + "] Colisión: " + upRes.message);
            }
        } else {
            string err = "[" + tag + "] Fallo al subir " + relPath + ": " + upRes.error;
            if (logCb) logCb(err);
            stats.errors.push_back(err);
        }
    }

    // 7. Check for remote files missing locally -> Download
    for (const auto& [relPath, remoteFile] : remoteFiles) {
        if (localFiles.find(relPath) == localFiles.end()) {
            string targetLocalPath = (fs::path(localRoot) / relPath).string();
            if (logCb) logCb("[" + tag + "] Descargando nueva partida: " + relPath);

            DownloadResult downRes = client.downloadSave(tag, relPath, targetLocalPath, true);
            if (downRes.success) {
                stats.downloaded++;
            } else {
                string err = "[" + tag + "] Fallo al descargar " + relPath + ": " + downRes.error;
                if (logCb) logCb(err);
                stats.errors.push_back(err);
            }
        }
    }

    if (logCb) {
        logCb("[" + tag + "] Resumen: " + to_string(stats.uploaded) + " subidas, " +
              to_string(stats.downloaded) + " descargadas, " +
              to_string(stats.identical) + " al día.");
    }
    return stats;
}
