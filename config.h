#pragma once

// --- Capteur analogique ---
#define SENSOR_PIN       A0
#define ADC_RESOLUTION   1023.0f  // ESP8266 : 10-bit (0-1023)
#define ADC_VREF         1.0f     // ESP8266 A0 max = 1V absolus
// Formule tension -> température. Adapter selon le capteur et le câblage.
// LM35 via diviseur /3.3 : TEMP_FORMULA(raw) = (raw/1023.0 * 3.3) / 0.01
// TMP36 via diviseur /3.3 : TEMP_FORMULA(raw) = ((raw/1023.0 * 3.3) - 0.5) / 0.01
// Par défaut : tension directe * 100 (pour capteur 10mV/°C déjà divisé)
#define TEMP_FORMULA(raw) ((float)(raw) / ADC_RESOLUTION * ADC_VREF * 100.0f)

// --- Réseau (valeurs par défaut si EEPROM vide) ---
#define DEFAULT_WIFI_SSID      "WIFIX"
#define DEFAULT_WIFI_PASSWORD  "PASSWORD"
#define DEFAULT_POST_URL       "https://europe-west1-meteo-des-loggias.cloudfunctions.net"
#define DEFAULT_DEVICE_NAME    "ESP8266-UNKNOWN"
#define DEFAULT_CF_FINGERPRINT "F7:5C:51:D6:8E:E9:CE:0B:48:6A:85:52:EC:A2:CA:4C:12:3D:CD:F2:17:7C:AB:07:AF:87:4F:A5:24:CC:FF:00"

// --- Timing ---
#define POST_INTERVAL_MS    120000UL  // intervalle entre deux POST (ms)
#define WIFI_RETRY_COUNT    2
#define WIFI_RETRY_DELAY_MS 1000

// --- Serial ---
#define SERIAL_BAUD 9600
