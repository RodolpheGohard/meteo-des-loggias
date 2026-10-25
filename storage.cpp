#include "storage.h"
#include "config.h"
#include <EEPROM.h>

void initEEPROM() {
    EEPROM.begin(EEPROM_SIZE);
}

static bool isInitialized() {
    return (EEPROM.read(0) == EEPROM_MAGIC_0 && EEPROM.read(1) == EEPROM_MAGIC_1);
}

static void writeString(int offset, int maxLen, const String& s) {
    int len = min((int)s.length(), maxLen - 1);
    for (int i = 0; i < len; i++) {
        EEPROM.write(offset + i, (uint8_t)s[i]);
    }
    EEPROM.write(offset + len, 0);
}

static String readString(int offset, int maxLen) {
    String result;
    result.reserve(maxLen);
    for (int i = 0; i < maxLen; i++) {
        char c = (char)EEPROM.read(offset + i);
        if (c == 0) break;
        result += c;
    }
    return result;
}

AppConfig loadConfig() {
    AppConfig cfg;
    cfg.postUrl      = DEFAULT_POST_URL;
    cfg.wifiSsid     = DEFAULT_WIFI_SSID;
    cfg.wifiPassword = DEFAULT_WIFI_PASSWORD;
    cfg.deviceName   = DEFAULT_DEVICE_NAME;
    cfg.fingerprint  = DEFAULT_CF_FINGERPRINT;

    if (!isInitialized()) return cfg;

    uint16_t urlLen = (uint16_t)EEPROM.read(EEPROM_OFF_URL_LEN) |
                      ((uint16_t)EEPROM.read(EEPROM_OFF_URL_LEN + 1) << 8);
    if (urlLen > 0 && urlLen < EEPROM_MAX_URL) {
        cfg.postUrl = readString(EEPROM_OFF_URL, EEPROM_MAX_URL);
    }

    uint8_t ssidLen = EEPROM.read(EEPROM_OFF_SSID_LEN);
    if (ssidLen > 0 && ssidLen < EEPROM_MAX_SSID) {
        cfg.wifiSsid = readString(EEPROM_OFF_SSID, EEPROM_MAX_SSID);
    }

    uint8_t passLen = EEPROM.read(EEPROM_OFF_PASS_LEN);
    if (passLen > 0 && passLen < EEPROM_MAX_PASS) {
        cfg.wifiPassword = readString(EEPROM_OFF_PASS, EEPROM_MAX_PASS);
    }

    uint8_t deviceLen = EEPROM.read(EEPROM_OFF_DEVICE_LEN);
    if (deviceLen > 0 && deviceLen < EEPROM_MAX_DEVICE) {
        cfg.deviceName = readString(EEPROM_OFF_DEVICE, EEPROM_MAX_DEVICE);
    }

    uint8_t fpLen = EEPROM.read(EEPROM_OFF_FP_LEN);
    if (fpLen > 0 && fpLen < EEPROM_MAX_FP) {
        cfg.fingerprint = readString(EEPROM_OFF_FP, EEPROM_MAX_FP);
    }

    cfg.ecoMode = (EEPROM.read(EEPROM_OFF_ECO) == 1);

    return cfg;
}

void saveEcoMode(bool eco) {
    EEPROM.write(0, EEPROM_MAGIC_0);
    EEPROM.write(1, EEPROM_MAGIC_1);
    EEPROM.write(EEPROM_OFF_ECO, eco ? 1 : 0);
    EEPROM.commit();
}

void saveFingerprint(const String& fp) {
    EEPROM.write(0, EEPROM_MAGIC_0);
    EEPROM.write(1, EEPROM_MAGIC_1);

    uint8_t len = (uint8_t)min((int)fp.length(), EEPROM_MAX_FP - 1);
    EEPROM.write(EEPROM_OFF_FP_LEN, len);
    writeString(EEPROM_OFF_FP, EEPROM_MAX_FP, fp);

    EEPROM.commit();
}

void saveDeviceName(const String& name) {
    EEPROM.write(0, EEPROM_MAGIC_0);
    EEPROM.write(1, EEPROM_MAGIC_1);

    uint8_t len = (uint8_t)min((int)name.length(), EEPROM_MAX_DEVICE - 1);
    EEPROM.write(EEPROM_OFF_DEVICE_LEN, len);
    writeString(EEPROM_OFF_DEVICE, EEPROM_MAX_DEVICE, name);

    EEPROM.commit();
}

void savePostUrl(const String& url) {
    EEPROM.write(0, EEPROM_MAGIC_0);
    EEPROM.write(1, EEPROM_MAGIC_1);

    uint16_t len = (uint16_t)min((int)url.length(), EEPROM_MAX_URL - 1);
    EEPROM.write(EEPROM_OFF_URL_LEN,     (uint8_t)(len & 0xFF));
    EEPROM.write(EEPROM_OFF_URL_LEN + 1, (uint8_t)(len >> 8));
    writeString(EEPROM_OFF_URL, EEPROM_MAX_URL, url);

    EEPROM.commit();
}

void resetPostUrl() {
    EEPROM.write(EEPROM_OFF_URL_LEN, 0);
    EEPROM.write(EEPROM_OFF_URL_LEN + 1, 0);
    for (int i = 0; i < EEPROM_MAX_URL; i++) {
        EEPROM.write(EEPROM_OFF_URL + i, 0);
    }
    EEPROM.commit();
}

void saveWifiCredentials(const String& ssid, const String& pass) {
    EEPROM.write(0, EEPROM_MAGIC_0);
    EEPROM.write(1, EEPROM_MAGIC_1);

    uint8_t ssidLen = (uint8_t)min((int)ssid.length(), EEPROM_MAX_SSID - 1);
    EEPROM.write(EEPROM_OFF_SSID_LEN, ssidLen);
    writeString(EEPROM_OFF_SSID, EEPROM_MAX_SSID, ssid);

    uint8_t passLen = (uint8_t)min((int)pass.length(), EEPROM_MAX_PASS - 1);
    EEPROM.write(EEPROM_OFF_PASS_LEN, passLen);
    writeString(EEPROM_OFF_PASS, EEPROM_MAX_PASS, pass);

    EEPROM.commit();
}
