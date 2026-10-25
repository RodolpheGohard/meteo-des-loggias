#include "serial_cmd.h"
#include "config.h"
#include "storage.h"
#include <ESP8266WiFi.h>

static const char* const VALID_DEVICE_NAMES[] = {
    "ESP8266-UNKNOWN", // valeur par défaut
    "ESP8266", // loggia du salon - sic
    "ESP8266-Salon", // intérieur du salon
    "ESP8266-Loggia-Chambre",
    "ESP8266-Test",
    nullptr
};

static bool isValidDeviceName(const String& name) {
    for (int i = 0; VALID_DEVICE_NAMES[i] != nullptr; i++) {
        if (name == VALID_DEVICE_NAMES[i]) return true;
    }
    return false;
}

static void printHelp() {
    Serial.println(F("Commandes disponibles :"));
    Serial.println(F("  GET_URL                  - affiche l'URL courante"));
    Serial.println(F("  SET_URL <url>            - sauvegarde l'URL en EEPROM"));
    Serial.println(F("  RESET_URL                - remet l'URL par defaut"));
    Serial.println(F("  GET_WIFI                 - affiche le SSID courant"));
    Serial.println(F("  SET_WIFI <ssid> <pass>   - sauvegarde WiFi et reconnecte"));
    Serial.println(F("  GET_DEVICE               - affiche le nom du device"));
    Serial.println(F("  SET_DEVICE <name>        - change le nom (voir LIST_VALID_DEVICES)"));
    Serial.println(F("  LIST_VALID_DEVICES       - liste les noms valides"));
    Serial.println(F("  ECO ON                   - active le deep sleep (persistant)"));
    Serial.println(F("  ECO OFF                  - desactive le deep sleep, module toujours eveille (defaut)"));
    Serial.println(F("  ECO                      - affiche l'etat du mode ECO"));
    Serial.println(F("  STATUS                   - etat complet"));
    Serial.println(F("  HELP                     - cette aide"));
}

void handleSerialCommands(AppConfig& cfg) {
    if (!Serial.available()) return;

    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) return;

    if (line == F("GET_URL")) {
        Serial.print(F("URL: "));
        Serial.println(cfg.postUrl);

    } else if (line.startsWith(F("SET_URL "))) {
        String url = line.substring(8);
        url.trim();
        if (url.length() == 0 || url.length() >= EEPROM_MAX_URL) {
            Serial.println(F("ERR: URL invalide ou trop longue (max 199 chars)"));
            return;
        }
        if (!url.startsWith(F("http://")) && !url.startsWith(F("https://"))) {
            Serial.println(F("ERR: URL doit commencer par http:// ou https://"));
            return;
        }
        savePostUrl(url);
        cfg.postUrl = url;
        Serial.println(F("OK: URL sauvegardee"));

    } else if (line == F("RESET_URL")) {
        resetPostUrl();
        cfg.postUrl = DEFAULT_POST_URL;
        Serial.println(F("OK: URL remise par defaut"));

    } else if (line == F("GET_WIFI")) {
        Serial.print(F("SSID: "));
        Serial.println(cfg.wifiSsid);

    } else if (line.startsWith(F("SET_WIFI "))) {
        String args = line.substring(9);
        args.trim();
        int sep = args.indexOf(' ');
        if (sep < 1) {
            Serial.println(F("ERR: usage SET_WIFI <ssid> <password>"));
            return;
        }
        String ssid = args.substring(0, sep);
        String pass = args.substring(sep + 1);
        pass.trim();
        if (ssid.length() >= EEPROM_MAX_SSID || pass.length() >= EEPROM_MAX_PASS) {
            Serial.println(F("ERR: SSID ou password trop long"));
            return;
        }
        saveWifiCredentials(ssid, pass);
        cfg.wifiSsid     = ssid;
        cfg.wifiPassword = pass;
        Serial.println(F("OK: credentials sauvegardes, reconnexion..."));
        WiFi.disconnect();
        WiFi.begin(cfg.wifiSsid.c_str(), cfg.wifiPassword.c_str());

    } else if (line == F("GET_DEVICE")) {
        Serial.print(F("Device: "));
        Serial.println(cfg.deviceName);

    } else if (line.startsWith(F("SET_DEVICE "))) {
        String name = line.substring(11);
        name.trim();
        if (!isValidDeviceName(name)) {
            Serial.println(F("ERR: nom invalide. Tapez LIST_VALID_DEVICES."));
            return;
        }
        saveDeviceName(name);
        cfg.deviceName = name;
        Serial.println(F("OK: nom du device sauvegarde"));

    } else if (line == F("LIST_VALID_DEVICES")) {
        Serial.println(F("Noms valides :"));
        for (int i = 0; VALID_DEVICE_NAMES[i] != nullptr; i++) {
            Serial.print(F("  ")); Serial.println(VALID_DEVICE_NAMES[i]);
        }

    } else if (line == F("ECO ON")) {
        saveEcoMode(true);
        cfg.ecoMode = true;
        Serial.println(F("OK: mode ECO active (deep sleep des que le Serial est inactif)"));

    } else if (line == F("ECO OFF")) {
        saveEcoMode(false);
        cfg.ecoMode = false;
        Serial.println(F("OK: mode ECO desactive (module toujours eveille)"));

    } else if (line == F("ECO")) {
        Serial.print(F("ECO: "));
        Serial.println(cfg.ecoMode ? F("actif") : F("inactif"));

    } else if (line == F("STATUS")) {
        Serial.println(F("--- STATUS ---"));
        Serial.print(F("SSID    : ")); Serial.println(cfg.wifiSsid);
        Serial.print(F("WiFi    : "));
        Serial.println(WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : F("deconnecte"));
        Serial.print(F("Device  : ")); Serial.println(cfg.deviceName);
        Serial.print(F("URL     : ")); Serial.println(cfg.postUrl);
        Serial.print(F("Eco     : ")); Serial.println(cfg.ecoMode ? F("actif (deep sleep hors session Serial)") : F("inactif (toujours eveille)"));
        Serial.print(F("Heap    : ")); Serial.print(ESP.getFreeHeap()); Serial.println(F(" octets"));

    } else if (line == F("HELP")) {
        printHelp();

    } else {
        Serial.print(F("ERR: commande inconnue '"));
        Serial.print(line);
        Serial.println(F("'. Tapez HELP."));
    }
}
