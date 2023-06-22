/*
 * Virtuino MQTT getting started example
 * Broker: HiveMQ (Secure connection)
 * Supported boards: ESP8266 / ESP32
 * Created by Ilias Lamprou
 * Jul 13 2021
 */

#include <Arduino.h>

#include <DHT.h>
#include <GP2YDustSensor.h>
#include <driver/ledc.h>

#include "sw_fs.h"

#pragma region definitions

#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];

int pin_led = 2;
int pin_dht11 = 23;

#define DHTPIN pin_dht11
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

int dly_loop = 580;
int dly_dht = 3000;
int dly_dust = 1000;

float sens_temperature = 0;
float sens_humidity = 0;
float sens_dust_pm25 = 0;

float sens_dust_pm10;
float sens_VOCs;
float sens_CO2;

int command1 = 0;

const uint8_t SHARP_LED_PIN = 32; // Sharp Dust/particle sensor Led Pin
const uint8_t SHARP_VO_PIN = 36;  // Sharp Dust/particle analog out pin used for reading

GP2YDustSensor dustSensor(GP2YDustSensorType::GP2Y1010AU0F, SHARP_LED_PIN, SHARP_VO_PIN);

char logBuffer[255];

#pragma endregion

#pragma region wifi stuff

#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

#pragma region mqtt_cert

static const char *root_ca PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)EOF";

#pragma endregion

// // const char *ssid = "Tenda_114328_ozan_2.4G";
// // const char *password = "f6wct00024";
// const char *ssid = "POCO X3 Pro";
// const char *password = "asdf1298";
// // const char *mqttServer = "62b784c5ce924ac796a727f4f9c84987.s2.eu.hivemq.cloud";
// const char *mqttServer = "x01bd66e.ala.us-east-1.emqxsl.com";
// const int mqttPort = 8883;
// const char *mqttUser = "ozntyr";
// const char *mqttPassword = "asdf1298";

//---- WiFi settings
String ssid = "";
String password = "";

int selected_ssid = 0;

//---- MQTT Broker settings
const char *mqtt_server = "62b784c5ce924ac796a727f4f9c84987.s2.eu.hivemq.cloud"; // replace with your broker url
const char *mqtt_username = "ozntyr";
const char *mqtt_password = "asdf1298";
const int mqtt_port = 8883;

WiFiClientSecure espClient; // for no secure connection use WiFiClient instead of WiFiClientSecure
// WiFiClient espClient;
PubSubClient client(espClient);

const char *sens_temperature_topic = "encyclopedia/sens_temperature";
const char *sens_humidity_topic = "encyclopedia/sens_humidity";
const char *sens_dust_pm25_topic = "encyclopedia/sens_dust_pm25";

const char *command1_topic = "encyclopedia/command1";
// const char* command1_topic="command2";

String indexHtml;

AsyncWebServer webServer(80);

void handle_Root(AsyncWebServerRequest *request)
{
  request->send(200, "text/html", indexHtml);
}

void handle_Select(AsyncWebServerRequest *request)
{
  String option = request->getParam("option")->value();
  // Do something with the selected option
  request->send(200, "text/plain", "Selected option: " + option);
}

void setup_WebServer()
{
  // Set up web server
  webServer.on("/", HTTP_GET, handle_Root);
  webServer.on("/select", HTTP_GET, handle_Select);

  webServer.begin();
}

void scan_Wifi()
{
  Serial.println("scan start");

  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0)
  {
    Serial.println("no networks found");
  }
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "y" : "n");

      for (int j = 0; j < known_len; j++)
      {
        if (WiFi.SSID(i) == known_ssids[j])
        {
          ssid = known_ssids[j];
          password = known_passs[j];
          Serial.printf("known ssid found and selected as target (%s) \n", ssid.c_str());
        }
      }
      delay(100);
    }
  }
  Serial.println("");
}

void setup_Wifi()
{
  scan_Wifi();

  delay(10);
  Serial.print("\nConnecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(WiFi.status());
    Serial.print(".");
  }
  randomSeed(micros());
  Serial.println("\nWiFi connected\nIP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-"; // Create a random client ID
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), mqtt_username, mqtt_password))
    {
      Serial.println("connected");

      client.subscribe(command1_topic); // subscribe the topics here
                                        // client.subscribe(command2_topic);   // subscribe the topics here
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds"); // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void callback(char *topic, byte *payload, unsigned int length)
{
  String incommingMessage = "";
  for (int i = 0; i < length; i++)
    incommingMessage += (char)payload[i];

  Serial.println("Message arrived [" + String(topic) + "]" + incommingMessage);

  //--- check the incomming message
  if (strcmp(topic, command1_topic) == 0)
  {
    if (incommingMessage.equals("1"))
    {
      // digitalWrite(pin_led, LOW); // Turn the LED on
    }
    else
    {
      // digitalWrite(pin_led, HIGH); // Turn the LED off
    }
  }

  //  check for other commands
  /*  else  if( strcmp(topic,command2_topic) == 0){
      if (incommingMessage.equals("1")) {  } // do something else
   }
   */
}

void publish_Message(const char *topic, String payload, boolean retained)
{
  if (client.publish(topic, payload.c_str(), true))
    Serial.println("Message publised [" + String(topic) + "]: " + payload);
}

#pragma endregion

#pragma region BLE stuff

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLECharacteristic *pCharacteristic;
BLEServer *pServer;

void setup_Ble()
{
  BLEDevice::init("ESP32");
  pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(BLEUUID((uint16_t)0x180F));

  pCharacteristic = pService->createCharacteristic(
      BLEUUID((uint16_t)0x2A19),
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_NOTIFY);

  pCharacteristic->addDescriptor(
      new BLE2902());

  pService->start();
  pServer->getAdvertising()->start();
}

void Send_BleMessage(String msg)
{
  if (pServer->getConnectedCount() > 0)
  {
    pCharacteristic->setValue(msg.c_str());
    pCharacteristic->notify();
  }
}

#pragma endregion

#pragma region AQI

float get_VOCs(); // TODO
float get_CO2();  // TODO

// Air Quality Index(AQI)
// thresholds are used to measure and communicate the level of air pollution in a specific area.The thresholds categorize air quality into different levels, indicating the potential health risks associated with different pollutant concentrations.The AQI typically considers pollutants such as particulate matter(PM2 .5 and PM10), ozone(O3), nitrogen dioxide(NO2), sulfur dioxide(SO2), and carbon monoxide(CO).

// AQI Tresholds
// Good (0-50): There is little to no health risk.
// Moderate (51-100): Sensitive individuals and those with respiratory or heart conditions may experience minor symptoms.
// Unhealthy for Sensitive Groups (101-150): Sensitive groups may experience more severe symptoms.
// Unhealthy (151-200): More individuals may begin to experience significant health effects.
// Very Unhealthy (201-300): Everyone may experience more serious health effects.
// Hazardous (301-500): Health warnings of emergency conditions. The entire population is more likely to be affected.

int AQI_StandartValues[] = {50, 100, 150, 200, 300, 500};
String AQI_StandartNames[] = {"Good", "Moderate", "Sensitive", "Unhealty", "Very Unhealty", "Hazardous"};
int AQI_PPMValues[] = {12, 35.5, 55.5, 150.5, 250.5, 500.5};
int AQI_StandartLen = 6;

int AQI_idx = 0;

float AQI_PM25;

// Air quality measurement
void measure_AirQuality()
{
}

// Pollutant identification
void identify_Pollutants();

// Air quality index (AQI) calculation
void calculate_AQI(int ppm)
{
  float aqi_breakpoint_low = 0;
  float aqi_breakpoint_high = 0;
  float ppm_breakpoint_low = 0;
  float ppm_breakpoint_high = 0;

  int idx = 0;
  while (idx <= AQI_StandartLen)
  {
    aqi_breakpoint_low = aqi_breakpoint_high;
    aqi_breakpoint_high = AQI_StandartValues[idx];
    ppm_breakpoint_low = ppm_breakpoint_high;
    ppm_breakpoint_high = AQI_PPMValues[idx];
    if (ppm < AQI_PPMValues[idx])
      break;
    else
      idx++;
  }

  AQI_idx = idx;

  AQI_PM25 = ((aqi_breakpoint_high - aqi_breakpoint_low) / (ppm_breakpoint_high - ppm_breakpoint_low)) * (ppm - ppm_breakpoint_low) + aqi_breakpoint_low;
}

// Automatic air purifier control
void control_AirPurifier();

// Real-time feedback and alerts
void update_AQI()
{
  calculate_AQI(10);
}
void check_AirQuality_Threshold()
{
  update_AQI();
  identify_Pollutants();

  control_AirPurifier();
}

// Settings and customization
void set_AirQuality_Thresholds();
void set_Notification_Preferences();
void set_Purifier_Control_Settings();

// Data logging and analysis
void log_AirQuality_Data();
void analyze_AirQuality_Trends();

#pragma endregion

#pragma region PWM Outputs

// Constants for fan and humidifier channels and GPIO pins
const int FAN_CHANNEL = 0;
const int FAN_PIN = 12;
const int HUMIDIFIER_CHANNEL = 1;
const int HUMIDIFIER_PIN = 14;

// Variables for fan and humidifier control
int fanSpeed = 0;            // Range: 0-255
int humidifierIntensity = 0; // Range: 0-255

// LEDC configuration variables
int ledcFrequency = 5000; // Frequency in Hz
int ledcResolution = 8;   // Bit resolution (0-15)

void setup_ledc()
{
  // Initialize LEDC library
  ledcSetup(FAN_CHANNEL, ledcFrequency, ledcResolution);        // Fan channel
  ledcSetup(HUMIDIFIER_CHANNEL, ledcFrequency, ledcResolution); // Humidifier channel

  // Attach GPIO pins to LEDC channels
  ledcAttachPin(FAN_PIN, FAN_CHANNEL);
  ledcAttachPin(HUMIDIFIER_PIN, HUMIDIFIER_CHANNEL);

  // Other setup code...
}

// Function to start the fan
void startFan()
{
  fanSpeed = 255; // Set fan speed to maximum
}

// Function to stop the fan
void stopFan()
{
  fanSpeed = 0; // Set fan speed to 0 (off)
}

// Function to start the humidifier
void startHumidifier()
{
  humidifierIntensity = 128; // Set humidifier intensity to half
}

// Function to stop the humidifier
void stopHumidifier()
{
  humidifierIntensity = 0; // Set humidifier intensity to 0 (off)
}

#pragma endregion

void setup_Serial()
{
  Serial.begin(115200);

  while (!Serial)
    delay(1);
}

void Task_PushAll(void *parameter)
{
  (void)parameter;

  for (;;)
  {

    if (!client.connected())
      reconnect();
    client.loop();

    if (isnan(sens_temperature) || isnan(sens_humidity))
    {
      Serial.println(F("Failed to read from DHT sensor!"));
    }
    else
    {
      Serial.printf("sen1: %f | sen2: %f \n", sens_temperature, sens_humidity);
      publish_Message(sens_temperature_topic, String(sens_temperature), true);
      publish_Message(sens_humidity_topic, String(sens_humidity), true);
    }
    if (isnan(sens_dust_pm25))
    {
      Serial.println(F("Failed to read from Dust sensor!"));
    }
    else
    {
      Serial.printf("sen3: %f \n", sens_dust_pm25);
      publish_Message(sens_dust_pm25_topic, String(sens_dust_pm25), true);
    }

    // Control the fan and humidifier based on the variables
    ledcWrite(FAN_CHANNEL, fanSpeed);
    ledcWrite(HUMIDIFIER_CHANNEL, humidifierIntensity);

    vTaskDelay(pdMS_TO_TICKS(dly_loop));
  }
}

void Task_DustSensor(void *parameter)
{
  (void)parameter;

  dustSensor.begin();

  for (;;)
  {
    for (int i = 1; i <= 100; i++)
    {
      sens_dust_pm25 = dustSensor.getDustDensity();
      sprintf(logBuffer, "Density: %d ug/m3", sens_dust_pm25);
      Serial.println(logBuffer);
      vTaskDelay(pdMS_TO_TICKS(dly_dust));
    }

    int dustAverage = dustSensor.getRunningAverage();
    float newBaseline = dustSensor.getBaselineCandidate();

    sprintf(logBuffer, "1m Avg Dust Density: %d ug/m3; New baseline: %.4f", dustAverage, newBaseline);
    Serial.println(logBuffer);

    // compensates sensor drift after 1m
    dustSensor.setBaseline(newBaseline);

    vTaskDelay(pdMS_TO_TICKS(dly_loop));
  }
}

void Task_Dht11Sensor(void *parameter)
{
  (void)parameter;

  dht.begin();

  for (;;)
  {

    sens_temperature = dht.readTemperature(); // replace the random value with your sensor value
    sens_humidity = dht.readHumidity();       // replace the random value  with your sensor value

    vTaskDelay(pdMS_TO_TICKS(dly_dht));
  }
}

void setup()
{
  setup_Serial();

  Serial.println("");
  Serial.println("setup_spiffs");
  setup_Spiffs();

  Serial.println("setup_wifi");
  setup_Wifi();

  Serial.println("setup_webserver");
  setup_WebServer();

  pinMode(pin_led, OUTPUT); // Initialize the BUILTIN_LED pin as an output
                            // pinMode(pin_dht11, INPUT); // Initialize the BUILTIN_LED pin as an output

#ifdef ESP8266
  espClient.setInsecure();
#else // for the ESP32
  espClient.setCACert(root_ca); // enable this line and the the "certificate" code for secure connection
#endif

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  xTaskCreate(Task_DustSensor, "Task_dustSensor", 4096, NULL, 10, NULL);
  xTaskCreate(Task_Dht11Sensor, "Task_dht11Sensor", 4096, NULL, 15, NULL);
  xTaskCreate(Task_PushAll, "Task_PushAll", 4096, NULL, 20, NULL);

  Serial.println("Setup Done!");
  delay(3000);
}

void loop()
{
}
