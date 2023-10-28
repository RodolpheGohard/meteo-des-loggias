#pragma once
#include <Arduino.h>

// Layout EEPROM (512 octets) :
//   [0-1]    : magic 0xAB 0xCD (EEPROM initialisée)
//   [2-3]    : longueur URL (uint16_t little-endian)
//   [4-203]  : URL null-terminated (max 200 chars)
//   [204]    : longueur SSID (uint8_t)
//   [205-236]: SSID null-terminated (max 32 chars)
//   [237]    : longueur password (uint8_t)
//   [238-301]: password null-terminated (max 64 chars)
//   [302]    : longueur device name (uint8_t)
//   [303-334]: device name null-terminated (max 32 chars)
//   [335]    : longueur fingerprint (uint8_t)
//   [336-430]: fingerprint null-terminated (max 95 chars)

#define EEPROM_SIZE              512
#define EEPROM_MAGIC_0           0xAB
#define EEPROM_MAGIC_1           0xCD
#define EEPROM_OFF_URL_LEN       2
#define EEPROM_OFF_URL           4
#define EEPROM_MAX_URL           200
#define EEPROM_OFF_SSID_LEN      204
#define EEPROM_OFF_SSID          205
#define EEPROM_MAX_SSID          32
#define EEPROM_OFF_PASS_LEN      237
#define EEPROM_OFF_PASS          238
#define EEPROM_MAX_PASS          64
#define EEPROM_OFF_DEVICE_LEN    302
#define EEPROM_OFF_DEVICE        303
#define EEPROM_MAX_DEVICE        32
#define EEPROM_OFF_FP_LEN        335
#define EEPROM_OFF_FP            336
#define EEPROM_MAX_FP            96

struct AppConfig {
    String wifiSsid;
    String wifiPassword;
    String postUrl;
    String deviceName;
    String fingerprint;
};

void initEEPROM();
AppConfig loadConfig();
void savePostUrl(const String& url);
void resetPostUrl();
void saveWifiCredentials(const String& ssid, const String& pass);
void saveDeviceName(const String& name);
void saveFingerprint(const String& fp);
