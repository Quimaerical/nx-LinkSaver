#include "views/sync_view.hpp"
#include "config/config_manager.hpp"
#include "api/api_client.hpp"

#include <iostream>

using namespace std;

SyncView::SyncView() {
    this->setOrientation(brls::Axis::COLUMN);
    this->setPadding(20, 20, 20, 20);

    brls::Header* header = new brls::Header();
    header->setTitle("Sincronización de Partidas");
    header->setSubtitle("Mantén tus guardados actualizados entre Nintendo Switch y PC/Android");
    this->addView(header);

    // Sync Button
    syncBtn = new brls::Button();
    syncBtn->setText("Sincronizar Todas las Carpetas");
    syncBtn->getClickEvent()->subscribe([this](brls::View* v) {
        if (!isSyncing) onStartSync();
    });
    this->addView(syncBtn);

    // Status Label
    statusLabel = new brls::Label();
    statusLabel->setText("Estado: Listo para sincronizar.");
    statusLabel->setMarginTop(10);
    this->addView(statusLabel);

    // Stats Label
    statsLabel = new brls::Label();
    statsLabel->setText("Última sincronización: Ninguna en esta sesión");
    statsLabel->setTextColor(brls::Application::getTheme()["brls/text_disabled"]);
    statsLabel->setMarginTop(5);
    statsLabel->setMarginBottom(15);
    this->addView(statsLabel);

    // Live Logs Header & List
    brls::Header* logHeader = new brls::Header();
    logHeader->setTitle("Registro de Sincronización en Vivo");
    this->addView(logHeader);

    logList = new brls::List();
    logList->setGrow(1.0f);
    this->addView(logList);

    refreshStatus();
}

void SyncView::refreshStatus() {
    auto& cfg = ConfigManager::getInstance().getConfig();
    if (cfg.serverUrl.empty()) {
        statusLabel->setText("Aviso: No has configurado la URL del servidor en Ajustes.");
        statusLabel->setTextColor(nvgRGB(234, 179, 8)); // Amber
    } else if (cfg.directories.empty()) {
        statusLabel->setText("Aviso: No tienes carpetas de guardado configuradas.");
        statusLabel->setTextColor(nvgRGB(234, 179, 8)); // Amber
    } else {
        statusLabel->setText("Listo para sincronizar (" + to_string(cfg.directories.size()) + " carpetas configuradas).");
        statusLabel->setTextColor(nvgRGB(34, 197, 94)); // Green
    }
}

void SyncView::addLogLine(const string& line) {
    if (!logList) return;
    brls::ListItem* item = new brls::ListItem(line);
    item->setTextColor(brls::Application::getTheme()["brls/text"]);
    logList->addView(item);
}

void SyncView::onStartSync() {
    auto& cfgMgr = ConfigManager::getInstance();
    const auto& cfg = cfgMgr.getConfig();

    if (cfg.serverUrl.empty()) {
        brls::Dialog* d = new brls::Dialog("Por favor configura la URL del servidor en la pestaña Ajustes.");
        d->addButton("Aceptar", []() {});
        d->open();
        return;
    }

    if (cfg.directories.empty()) {
        brls::Dialog* d = new brls::Dialog("No tienes ninguna carpeta configurada. Añade una en 'Carpetas de Saves'.");
        d->addButton("Aceptar", []() {});
        d->open();
        return;
    }

    isSyncing = true;
    syncBtn->setEnabled(false);
    syncBtn->setText("Sincronizando...");
    statusLabel->setText("Sincronización en curso...");
    statusLabel->setTextColor(nvgRGB(56, 189, 248)); // Sky Blue

    if (logList) logList->clear();
    addLogLine("Iniciando sincronización con " + cfg.serverUrl);

    // Run synchronization in background thread via brls::async
    brls::async([this, cfg]() {
        ApiClient client(cfg.serverUrl, cfg.authToken, cfg.timeoutSeconds);
        auto& mgr = ConfigManager::getInstance();
        SyncEngine engine(client, mgr);

        SyncStats stats = engine.syncAll(
            // Status Callback
            [this](const string& st) {
                brls::sync([this, st]() {
                    if (statusLabel) statusLabel->setText(st);
                });
            },
            // Progress Callback
            [this](int cur, int total) {
                brls::sync([this, cur, total]() {
                    if (statusLabel) {
                        statusLabel->setText("Procesando carpeta " + to_string(cur) + " de " + to_string(total) + "...");
                    }
                });
            },
            // Log Callback
            [this](const string& logLine) {
                brls::sync([this, logLine]() {
                    addLogLine(logLine);
                });
            }
        );

        // Update UI when sync finishes
        brls::sync([this, stats]() {
            isSyncing = false;
            syncBtn->setEnabled(true);
            syncBtn->setText("Sincronizar Todas las Carpetas");

            string summary = "Resumen: " + to_string(stats.uploaded) + " subidas, " +
                             to_string(stats.downloaded) + " descargadas, " +
                             to_string(stats.identical) + " al día, " +
                             to_string(stats.conflicts) + " conflictos resueltos.";
            if (statsLabel) statsLabel->setText(summary);

            if (stats.hasErrors()) {
                if (statusLabel) {
                    statusLabel->setText("Sincronización finalizada con errores.");
                    statusLabel->setTextColor(nvgRGB(239, 68, 68)); // Red
                }
            } else {
                if (statusLabel) {
                    statusLabel->setText("¡Sincronización completada con éxito!");
                    statusLabel->setTextColor(nvgRGB(34, 197, 94)); // Green
                }
            }

            brls::Dialog* d = new brls::Dialog(
                "Sincronización Finalizada\n\n" + summary + "\n" +
                (stats.hasErrors() ? ("Errores: " + to_string(stats.errors.size())) : "Todo en orden.")
            );
            d->addButton("Aceptar", []() {});
            d->open();
        });
    });
}
