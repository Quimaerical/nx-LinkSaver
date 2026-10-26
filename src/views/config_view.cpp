#include "views/config_view.hpp"
#include "config/config_manager.hpp"
#include "api/api_client.hpp"
#include "api/url_normalizer.hpp"
#include "platform/software_keyboard.hpp"

#include <iostream>

using namespace std;

ConfigView::ConfigView() {
    this->setOrientation(brls::Axis::COLUMN);
    this->setPadding(20, 20, 20, 20);

    brls::Header* header = new brls::Header();
    header->setTitle("Configuración de Servidor y Red");
    header->setSubtitle("Ajusta la conexión local (LAN) o remota (Cloudflare Tunnel)");
    this->addView(header);

    brls::List* list = new brls::List();

    // Server URL
    urlItem = new brls::ListItem("URL del Servidor");
    urlItem->getClickEvent()->subscribe([this](brls::View* v) { onEditUrl(); });
    list->addView(urlItem);

    // Auth Token
    tokenItem = new brls::ListItem("Token de Autenticación (Opcional)");
    tokenItem->getClickEvent()->subscribe([this](brls::View* v) { onEditToken(); });
    list->addView(tokenItem);

    // Timeout
    timeoutItem = new brls::ListItem("Tiempo de Espera de Red");
    timeoutItem->getClickEvent()->subscribe([this](brls::View* v) {
        auto& mgr = ConfigManager::getInstance();
        int cur = mgr.getTimeoutSeconds();
        int next = (cur >= 30) ? 5 : (cur + 5);
        mgr.setTimeoutSeconds(next);
        refreshValues();
    });
    list->addView(timeoutItem);

    this->addView(list);

    // Status Label for Ping feedback
    statusLabel = new brls::Label();
    statusLabel->setText("Estado: Listo para probar conexión");
    statusLabel->setTextColor(brls::Application::getTheme()["brls/text"]);
    statusLabel->setMarginTop(10);
    statusLabel->setMarginBottom(10);
    this->addView(statusLabel);

    // Actions Box
    brls::Box* actionsBox = new brls::Box();
    actionsBox->setOrientation(brls::Axis::ROW);
    actionsBox->setMarginTop(15);

    pingBtn = new brls::Button();
    pingBtn->setText("Comprobar Conexión (Ping)");
    pingBtn->getClickEvent()->subscribe([this](brls::View* v) { onTestPing(); });
    actionsBox->addView(pingBtn);

    saveBtn = new brls::Button();
    saveBtn->setText("Guardar Ajustes");
    saveBtn->getClickEvent()->subscribe([this](brls::View* v) { onSave(); });
    actionsBox->addView(saveBtn);

    this->addView(actionsBox);

    refreshValues();
}

void ConfigView::refreshValues() {
    const auto& cfg = ConfigManager::getInstance().getConfig();

    if (urlItem) {
        urlItem->setValue(cfg.serverUrl.empty() ? "(Sin configurar)" : cfg.serverUrl);
    }
    if (tokenItem) {
        tokenItem->setValue(cfg.authToken.empty() ? "(Ninguno)" : "********");
    }
    if (timeoutItem) {
        timeoutItem->setValue(to_string(cfg.timeoutSeconds) + " segundos");
    }
}

void ConfigView::onEditUrl() {
    auto& mgr = ConfigManager::getInstance();
    string current = mgr.getServerUrl();
    string entered = SoftwareKeyboard::prompt(
        "Ingresa la IP local (ej. 192.168.1.50:8080) o dominio HTTPS:",
        current,
        256
    );

    if (!entered.empty()) {
        mgr.setServerUrl(entered);
        refreshValues();
    }
}

void ConfigView::onEditToken() {
    auto& mgr = ConfigManager::getInstance();
    string current = mgr.getAuthToken();
    string entered = SoftwareKeyboard::prompt(
        "Ingresa el token Bearer (dejar vacío si el servidor no tiene auth):",
        current,
        256
    );

    mgr.setAuthToken(entered);
    refreshValues();
}

void ConfigView::onTestPing() {
    auto& mgr = ConfigManager::getInstance();
    string url = mgr.getServerUrl();

    if (url.empty()) {
        brls::Dialog* d = new brls::Dialog("Por favor ingresa primero la URL del servidor antes de probar ping.");
        d->addButton("Aceptar", []() {});
        d->open();
        return;
    }

    if (statusLabel) {
        statusLabel->setText("Probando conexión con " + url + "...");
        statusLabel->setTextColor(brls::Application::getTheme()["brls/text"]);
    }

    string token = mgr.getAuthToken();
    int timeout = mgr.getTimeoutSeconds();

    // Run ping in background thread
    brls::async([this, url, token, timeout]() {
        ApiClient client(url, token, timeout);
        PingResult res = client.ping();

        // Dispatch UI update back to main thread
        brls::sync([this, res]() {
            if (res.success) {
                if (statusLabel) {
                    statusLabel->setText("¡Conectado! LinkSaverBack v" + res.version + 
                                         " (" + to_string(res.latencyMs) + " ms)");
                    statusLabel->setTextColor(nvgRGB(34, 197, 94)); // Emerald Green
                }
                brls::Dialog* dialog = new brls::Dialog(
                    "Conexión exitosa con el backend.\n"
                    "• Versión: " + res.version + "\n"
                    "• Latencia: " + to_string(res.latencyMs) + " ms\n"
                    "• Tiempo Servidor: " + to_string(res.serverTime)
                );
                dialog->addButton("Aceptar", []() {});
                dialog->open();
            } else {
                if (statusLabel) {
                    statusLabel->setText("Error de conexión: " + res.error);
                    statusLabel->setTextColor(nvgRGB(239, 68, 68)); // Red
                }
                brls::Dialog* dialog = new brls::Dialog("Error al conectar:\n" + res.error);
                dialog->addButton("Aceptar", []() {});
                dialog->open();
            }
        });
    });
}

void ConfigView::onSave() {
    auto& mgr = ConfigManager::getInstance();
    if (mgr.save()) {
        brls::Dialog* d = new brls::Dialog("Ajustes guardados correctamente en " + mgr.getConfigFilePath());
        d->addButton("Aceptar", []() {});
        d->open();
    } else {
        brls::Dialog* d = new brls::Dialog("Error al escribir el archivo de configuración.");
        d->addButton("Aceptar", []() {});
        d->open();
    }
}
