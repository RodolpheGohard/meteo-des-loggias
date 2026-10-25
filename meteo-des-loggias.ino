#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_AHTX0.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <string>
#include "config.h"
#include "storage.h"
#include "serial_cmd.h"

Adafruit_AHTX0 aht;
// GPIO where the DS18B20 is connected to
const int oneWireBus = D6;
// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);
// Pass our oneWire reference to Dallas Temperature sensor
DallasTemperature sensors(&oneWire);

AppConfig gConfig;
unsigned long gLastPostMs = 0;
unsigned long gLastSerialMs = 0; // dernier octet recu sur le Serial


void setup()
{
  Serial.begin(SERIAL_BAUD);
  Serial.println(F("\n[Boot] Meteo des Loggias"));

  pinMode(LED_BUILTIN, OUTPUT);  // Initialize the LED_BUILTIN pin as an output
  digitalWrite(LED_BUILTIN, LOW);

  Serial.println(F("\n[sensor] Starting the DS18B20"));
  sensors.begin();
  Serial.println(F("\n[sensor] Starting AHT"));
  if (!aht.begin())
  {
    Serial.println("[sensor] Could not find AHT? Check wiring");
    while (1)
      delay(10);
  }
  Serial.println("[sensor] AHT10 or AHT20 found");

  Serial.println("[config] loading config from EEPROM");
  initEEPROM();
  gConfig = loadConfig();
  Serial.println("[config] loaded with ssid:" + gConfig.wifiSsid);
  connectWifi(gConfig);

  digitalWrite(LED_BUILTIN, HIGH);

  measureAndSend();
  gLastPostMs = millis();
  if (gConfig.ecoMode) {
    enterConfigWindowOrSleep();
  }
}

static void connectWifi(const AppConfig& cfg) {
    if (WiFi.status() == WL_CONNECTED) return;

    Serial.printf("[WiFi] Connexion a %s", cfg.wifiSsid.c_str());
    WiFi.mode(WIFI_STA); // Désactiver scan inutile
    WiFi.begin(cfg.wifiSsid.c_str(), cfg.wifiPassword.c_str());

    for (int i = 0; i < WIFI_RETRY_COUNT && WiFi.status() != WL_CONNECTED; i++) {
        delay(WIFI_RETRY_DELAY_MS);
        Serial.print('.');
    }

    Serial.printf("\n[WiFi] Connected, IP=%s\n", WiFi.localIP().toString().c_str());

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\n[WiFi] Connecte, IP=%s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.println(F("non connecté"));
    }
}

const char *CF_HOST = "europe-west1-meteo-des-loggias.cloudfunctions.net";
const int CF_PORT = 443;
const char *CF_PATH = "/register-sensors-data";

void sendData(const String& deviceName, float temperature1, float temperature2, float humidity, int luminosity)
{
  WiFiClientSecure httpsClient; // Declare object of class WiFiClient
  httpsClient.setInsecure(); // pas de validation du certificat (pas de CA ni de fingerprint)
  httpsClient.setTimeout(15000); // 15 Seconds
  delay(1000);

  Serial.println("HTTPS Connecting ...");
  int code = 0;
  code = httpsClient.connect(CF_HOST, CF_PORT);
  Serial.print("Connected with code");
  Serial.println(code);
  if (!code)
  {
    Serial.print("Connection failed with exit code");
    Serial.println(code);

    digitalWrite(LED_BUILTIN, LOW);
    delay(300);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(300);
    digitalWrite(LED_BUILTIN, LOW);
    delay(300);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(300);
    digitalWrite(LED_BUILTIN, LOW);
    delay(300);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(300);
    digitalWrite(LED_BUILTIN, LOW);
    delay(300);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(300);
    digitalWrite(LED_BUILTIN, LOW);
    delay(300);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(300);
    return;
  }
  else
  {
    Serial.println("Connected to server");
    digitalWrite(LED_BUILTIN, LOW);
    delay(2000);
    digitalWrite(LED_BUILTIN, HIGH);
  }

  Serial.print("requesting URL: ");
  Serial.println(String(CF_HOST) + ":" + String(CF_PORT) + String(CF_PATH));

  String body = "{\"sensorName\": \""+ deviceName +"\", \"temperature1\": " + String(temperature1) + ", \"temperature2\": " + String(temperature2) + ", \"humidity\": " + String(humidity) + ", \"luminosity\": " + String(luminosity) + "}";
  Serial.print("sending to Cloud Function: ");
  Serial.println(body);

  httpsClient.println("POST " + String(CF_PATH) + " HTTP/1.1");
  httpsClient.println("Host: " + String(CF_HOST));
  httpsClient.println("Connection: close");
  httpsClient.println("Content-Type: application/json");
  httpsClient.print("Content-Length: ");
  httpsClient.println(body.length());
  httpsClient.println();
  httpsClient.println(body);

  Serial.println("request sent");

  while (httpsClient.connected())
  {
    String line = httpsClient.readStringUntil('\n');
    if (line == "\r")
    {
      Serial.println("headers received");
      break;
    }
  }

  Serial.println("reply was:");
  Serial.println("==========");
  String line;
  while (httpsClient.available())
  {
    line = httpsClient.readStringUntil('\n'); // Read Line by Line
    Serial.println(line);                     // Print response
  }
  Serial.println("==========");
  Serial.println("closing connection");
}

// Lit les capteurs et envoie les donnees.
void measureAndSend()
{
  sensors.requestTemperatures();
  float temperatureC = sensors.getTempCByIndex(0);

  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp); // populate temp and humidity objects with fresh data

  int lux = analogRead(A0);

  sendData(gConfig.deviceName, temperatureC, temp.temperature, humidity.relative_humidity, lux);
}

// Dort le temps qu'il reste avant le prochain envoi : si la carte est restee
// eveillee (session Serial), le deep sleep est raccourci pour garder la cadence.
void goToDeepSleep()
{
  uint64_t elapsedUs = (uint64_t)(millis() - gLastPostMs) * 1000ULL;
  uint64_t sleepUs = elapsedUs < DEEP_SLEEP_INTERVAL_US ? DEEP_SLEEP_INTERVAL_US - elapsedUs : 1000000ULL;
  Serial.printf("[Sleep] Extinction WiFi, deep sleep %lu ms\n", (unsigned long)(sleepUs / 1000));
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  Serial.flush();
  ESP.deepSleep(sleepUs);
}

// Mode ECO : ecoute le Serial quelques secondes apres l'envoi. Un octet recu
// (commande, ou simple retour a la ligne envoye par la page de config) signifie
// que quelqu'un est connecte : pas de deep sleep, on continue dans loop() qui
// s'endormira quand le Serial redeviendra silencieux. Sinon : deep sleep.
void enterConfigWindowOrSleep()
{
  Serial.printf("[Serial] Fenetre d'ecoute (%lu ms) - envoyez un octet pour rester eveille\n", SERIAL_WINDOW_MS);
  unsigned long windowStart = millis();
  while (millis() - windowStart < SERIAL_WINDOW_MS) {
    if (Serial.available()) {
      gLastSerialMs = millis();
      Serial.println(F("[Sleep] Serial actif : mode ECO suspendu"));
      return;
    }
    delay(20);
  }

  goToDeepSleep();
}

void loop()
{
  if (Serial.available()) {
    gLastSerialMs = millis();
  }

  if (WiFi.status() != WL_CONNECTED) {
      connectWifi(gConfig);
  }

  handleSerialCommands(gConfig);

  if (millis() - gLastPostMs >= POST_INTERVAL_MS) {
    gLastPostMs = millis();
    measureAndSend();
  }

  // ECO + plus personne sur le Serial : on se rendort.
  if (gConfig.ecoMode && millis() - gLastSerialMs >= SERIAL_IDLE_TIMEOUT_MS) {
    Serial.println(F("[Sleep] Serial inactif : retour au deep sleep"));
    goToDeepSleep();
    return;
  }

  delay(50);
}
