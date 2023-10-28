#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_AHTX0.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <string>

Adafruit_AHTX0 aht;
// GPIO where the DS18B20 is connected to
const int oneWireBus = D6;
// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);
// Pass our oneWire reference to Dallas Temperature sensor
DallasTemperature sensors(&oneWire);

const int BAUDS = 9600;
const char *WIFI_SSID = "SFR_D228";
const char *WIFI_PASSWORD = "8cjt87qkyc7jme26sceu";
const int WIFI_POLL_DELAY = 500;

void setup()
{
  Serial.println("Starting ...");
  digitalWrite(LED_BUILTIN, LOW);

  Serial.begin(BAUDS);

  // Start the DS18B20 sensor
  sensors.begin();
  if (!aht.begin())
  {
    Serial.println("Could not find AHT? Check wiring");
    while (1)
      delay(10);
  }
  Serial.println("AHT10 or AHT20 found");

  Serial.print("Connection en cours");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) // tant que l'esp n'est pas connecté au réseau on attends
  {
    delay(WIFI_POLL_DELAY);
    Serial.print(".");
  }
  Serial.print("Connecté, adresse IP : ");
  Serial.println(WiFi.localIP());

  digitalWrite(LED_BUILTIN, HIGH);
}

// const char CF_FINGERPRINT[] PROGMEM = "AB 24 A8 96 AB 9F E1 A0 D2 6D 56 08 87 AA 8D 5C 17 08 A6 C4";
const char CF_FINGERPRINT[] PROGMEM = "93 7C D5 0E C8 8E F7 CC 1F 1E 60 06 C7 F4 D5 FA 13 86 25 91";
const char *CF_HOST = "europe-west1-meteo-des-loggias.cloudfunctions.net";
//const char *CF_HOST = "www.iana.org";
const int CF_PORT = 443;
const char *CF_PATH = "/register-sensors-data";

//const int HTTPS_POLL_DELAY = 100;
//const int HTTPS_POLL_MAX_RETRIES = 1;

const int MAIN_LOOP_DELAY = 60000;

void sendData(float temperature1, float temperature2, float humidity, int luminosity)
{
  WiFiClientSecure httpsClient; // Declare object of class WiFiClient
  // httpsClient.setInsecure();

  Serial.printf("Using fingerprint '%s'\n", CF_FINGERPRINT);
  httpsClient.setFingerprint(CF_FINGERPRINT);
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
    return;
  }
  else
  {
    Serial.println("Connected to server");
  }

  Serial.print("requesting URL: ");
  Serial.println(String(CF_HOST) + ":" + String(CF_PORT) + String(CF_PATH));

  String body = "{\"sensorName\": \"ESP8266\", \"temperature1\": " + String(temperature1) + ", \"temperature2\": " + String(temperature2) + ", \"humidity\": " + String(humidity) + ", \"luminosity\": " + String(luminosity) + "}";
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

void loop()
{
  // Temperature
  sensors.requestTemperatures();
  float temperatureC = sensors.getTempCByIndex(0);
  //Serial.print(temperatureC);
  //Serial.println("ºC");

  // Temperature & humidity
  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp); // populate temp and humidity objects with fresh data
  //Serial.print("Temperature: ");
  //Serial.print(temp.temperature);
  //Serial.println(" degrees C");
  //Serial.print("Humidity: ");
  //Serial.print(humidity.relative_humidity);
  //Serial.println("% rH");

  int lux = analogRead(A0);
  //Serial.print("Luminosity");
  //Serial.println(lux);

  sendData(temperatureC, temp.temperature, humidity.relative_humidity, lux);
  Serial.print("Waiting ");
  Serial.print(MAIN_LOOP_DELAY);
  Serial.println(" Seconds");
  delay(MAIN_LOOP_DELAY);
}
