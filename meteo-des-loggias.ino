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
// TODO: import these from ENV
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
    return;
  } else {
    Serial.println("AHT10 or AHT20 found");
  }
  
  Serial.print("Connection au WiFi en cours");
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

// TODO: this should definitely be taken from env or even through an update system 
const char CF_FINGERPRINT[] PROGMEM = "93 7C D5 0E C8 8E F7 CC 1F 1E 60 06 C7 F4 D5 FA 13 86 25 91";
const char *CF_HOST = "europe-west1-meteo-des-loggias.cloudfunctions.net";
const int CF_PORT = 443;
const char *CF_PATH = "/register-sensors-data";

const int MAIN_LOOP_DELAY = 60000;

struct SensorsReading
{
  float temperature1;
  float temperature2;
  float humidity;
  int luminosity;
};


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


void sendReadings(SensorsReading[] readings, int size) {
  if (connectToServer(&client)) {
    // Now we need to generate potentially large payload generated on the fly
    // HTTP requires to send ContentLength in advance, so it has to be generated and sized beforehand -- sucks
    // Alternative is to use chunked encoding. Not nice but eh.
    // Or maybe protobuf ?
    // https://stackoverflow.com/questions/33264731/whats-the-best-way-to-send-http-if-content-length-is-only-approximate
  }

}

const int READINGS_BUFFER_SIZE = 20;
SensorsReading readings[READINGS_BUFFER_SIZE];
const int readingsIndex = 0;

void loop()
{
  SensorsReading sensorsReading;

  // Temperature
  sensors.requestTemperatures();
  float temperatureC = sensors.getTempCByIndex(0);
  sensorsReading.temperature1 = temperatureC;

  // Temperature & humidity
  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp); // populate temp and humidity objects with fresh data
  sensorsReading.temperature1 = temp.temperature;
  sensorsReading.humidity = humidity.relative_humidity

  // Luminosity
  int lux = analogRead(A0);
  sensorsReading.luminosity = lux;
  
  readings[readingsIndex] = sensorsReading;
  readings++;
  if (readingsIndex == READINGS_BUFFER_SIZE) {
    // reset buffer and send data
    sendReadings(readings);
    readingsIndex = 0;
  }
  sendData(temperatureC, temp.temperature, humidity.relative_humidity, lux);

  // Wait for our next reading
  // TODO: some kind of deep sleep power saving stuff here ?
  Serial.print("Waiting ");
  Serial.print(MAIN_LOOP_DELAY);
  Serial.println(" Seconds");
  delay(MAIN_LOOP_DELAY);
}
