
#include <Arduino.h>

#include <DHT.h>
#include <GP2YDustSensor.h>
#include <driver/ledc.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <FastLED.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>

#include "sw_fs.h"

#pragma region Variable and Constant Definitions

#define pin_dht11 23
#define pin_dust_led 32
#define pin_dust_v0 36
#define pin_ws2812 26
#define pin_fan 12
#define pin_mist 14

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

#define SHARP_LED_PIN pin_dust_led // Sharp Dust/particle sensor Led Pin
#define SHARP_VO_PIN pin_dust_v0   // Sharp Dust/particle analog out pin used for reading

GP2YDustSensor dustSensor(GP2YDustSensorType::GP2Y1010AU0F, SHARP_LED_PIN, SHARP_VO_PIN);

char logBuffer[255];

// Topics for sending sensor readings
const char *tr_temperature = "Readings/Temperature";
const char *tr_humidity = "Readings/Humidity";
const char *tr_dust_pm25 = "Readings/PM25";

// Topics for receiving commands
const char *tc_stop_all = "Commands/StopAll";
const char *tc_stop_fan = "Commands/StopFan";
const char *tc_stop_humumidifier = "Commands/StopHumidifier";
const char *tc_start_all = "Commands/StartAll";
const char *tc_start_fan = "Commands/StartFan";
const char *tc_start_humidifier = "Commands/StartHumidifier";
const char *tc_set_mode = "Commands/SetMode";
const char *tc_set_timer_date = "Commands/SetTimerDate";
const char *tc_set_timer_duration = "Commands/SetTimerDuration";
const char *tc_set_is_timer_repeating = "Commands/SetIsTimerRepeating";
const char *tc_set_ledEffect = "Commands/SetLedEffect";
const char *tc_set_ledColor = "Commands/SetLedColor";
const char *tc_set_ledBrightness = "Commands/SetLedBrightness";

enum sw_modes
{
  MODE_MANUAL = 0,
  MODE_AUTO,
  MODE_SLEEP,
  MODE_TURBO,
  MODE_ENERYSAVE
};

sw_modes mode_current;

const char *ntpServer = "pool.ntp.org";
const long gmtOffset = 10800;
const int daylightOffset = 0;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer, gmtOffset, daylightOffset);

struct tm timer_currentDate_tm;
struct tm timer_startDate_tm;
struct tm timer_stopDate_tm;
int timer_durationinSeconds_int = 0;

bool is_timerRepeating = false;
bool is_waiting4Schedule = false;
bool is_onSchedule = false;

const int BUFFER_SIZE = 50;
char buffer[BUFFER_SIZE];
int bufferIndex = 0;
int counter = 0;

#define LED_PIN pin_ws2812 // TBD
#define LED_COUNT 30       // TBD

CRGB leds[LED_COUNT];

enum led_effects
{
  EFF_OFF = 0,
  EFF_SOLID,
  EFF_WIPE,
  EFF_CHASE,
  EFF_BREATHING,
  EFF_FIRE,
  EFF_RAINBOW
};

led_effects ledEff_current = led_effects::EFF_OFF;
CRGB ledColor_current = CRGB::Black;

uint8_t ledBrightnes_current = 128; // Initial brightness value (0-255)

#define BMP280_1_I2C_ADDRESS 0x76 // Change to the address of the first BMP280
#define BMP280_2_I2C_ADDRESS 0x77 // Change to the address of the second BMP280

Adafruit_BMP280 bmpSensor1; // Initialize the first BMP280 sensor object
Adafruit_BMP280 bmpSensor2; // Initialize the second BMP280 sensor object

#pragma endregion

#pragma region PWM Outputs

// Constants for fan and humidifier channels and GPIO pins
const int FAN_CHANNEL = 0;
#define FAN_PIN pin_fan
const int HUMIDIFIER_CHANNEL = 1;
#define HUMIDIFIER_PIN pin_mist

// Variables for fan and humidifier control
int fanSpeed = 0;            // Range: 0-255
int humidifierIntensity = 0; // Range: 0-255

int fanSpeedManual = 0;
int humidifierIntensityManual = 0;

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
void startFan(int speed)
{
  fanSpeed = speed; // Set fan speed to maximum
}

// Function to stop the fan
void stopFan()
{
  fanSpeed = 0; // Set fan speed to 0 (off)
}

// Function to start the humidifier
void startHumidifier(int intensity)
{
  humidifierIntensity = intensity; // Set humidifier intensity to half
}

// Function to stop the humidifier
void stopHumidifier()
{
  humidifierIntensity = 0; // Set humidifier intensity to 0 (off)
}

#pragma endregion

#pragma region WS2812

void setup_ws2812()
{
  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, LED_COUNT);
  FastLED.show(); // Initialize all pixels to off
}

bool isValidHtmlColor(const char *htmlCode)
{
  if (htmlCode[0] == '#' && strlen(htmlCode) == 7)
  {
    for (int i = 1; i < 7; i++)
    {
      char c = htmlCode[i];
      if (!isxdigit(c))
      {
        return false;
      }
    }
    return true;
  }
  return false;
}

CRGB htmlToColor(const char *htmlCode)
{
  long number = strtol(htmlCode + 1, NULL, 16);
  uint8_t r = number >> 16;
  uint8_t g = number >> 8;
  uint8_t b = number;
  return CRGB(r, g, b);
}

void led_changeBrightness(uint8_t newBrightness)
{
  // Ensure the new brightness is within valid range
  if (newBrightness >= 0 && newBrightness <= 255)
  {
    ledBrightnes_current = newBrightness;
    FastLED.setBrightness(ledBrightnes_current);
    FastLED.show();
  }
}

void led_turnOff()
{
  fill_solid(leds, LED_COUNT, CRGB::Black);
  FastLED.show();
}

void led_solidColor(CRGB color)
{
  fill_solid(leds, LED_COUNT, color);
  FastLED.show();
}

void led_colorWipe(CRGB color, int delayTime)
{
  for (int i = 0; i < LED_COUNT; i++)
  {
    leds[i] = color;
    FastLED.show();
    delay(delayTime);
  }
}

void led_colorChase(CRGB color, int delayTime)
{
  for (int i = 0; i < LED_COUNT; i++)
  {
    leds[i] = color;
    FastLED.show();
    delay(delayTime);
    leds[i] = CRGB::Black;
  }
}

void led_breathingEffect(CRGB color)
{
  int brightness;
  for (int i = 0; i < 256; i++)
  {
    brightness = sin8(i);
    fill_solid(leds, LED_COUNT, color);
    FastLED.setBrightness(brightness);
    FastLED.show();
    delay(20);
  }
}

void led_fireEffect()
{
  // Implement the fire effect here
  // (You can use the Fire2012 example from the FastLED library, for example)
}

void led_rainbowEffect()
{
  uint8_t deltaHue = 255 / LED_COUNT;
  uint8_t hue = 0;
  for (int i = 0; i < LED_COUNT; i++)
  {
    leds[i] = CHSV(hue, 255, 255);
    hue += deltaHue;
  }
  FastLED.show();
  delay(10);
}

#pragma endregion

#pragma region NTP

void printDateTime(const struct tm &timeStruct)
{
  Serial.print("Date: ");
  Serial.print(timeStruct.tm_year + 1900);
  Serial.print("-");
  Serial.print(timeStruct.tm_mon + 1);
  Serial.print("-");
  Serial.print(timeStruct.tm_mday);
  Serial.print(" ");
  Serial.print(timeStruct.tm_hour);
  Serial.print(":");
  Serial.print(timeStruct.tm_min);
  Serial.print(":");
  Serial.println(timeStruct.tm_sec);
}

void getTimeFromNTP(struct tm &timeStruct)
{
  time_t currentTimestamp = timeClient.getEpochTime();
  localtime_r(&currentTimestamp, &timeStruct);
}

void updateTimerStopDate()
{
  /* this part is just for practice
  int startTime = static_cast<int>(mktime(&timer_startDate_tm)); // Convert tm to time_t then time_t to integer
  int stopTime = startTime + timer_durationinSeconds_int;        // Add 5 mins (300 seconds)
  time_t endTime = static_cast<time_t>(stopTime);                // Convert back to time_t

  localtime_r(&endTime, &timer_stopDate_tm);
  */

  timer_stopDate_tm = timer_startDate_tm;
  timer_stopDate_tm.tm_sec += timer_durationinSeconds_int;
}

void setTimerStartDate(int seconds, bool isrepeat)
{
  time_t startTime = static_cast<time_t>(seconds);
  localtime_r(&startTime, &timer_startDate_tm);

  if (isrepeat)
    timer_startDate_tm.tm_hour += 24;

  is_waiting4Schedule = true;

  updateTimerStopDate();
}

void setTimerDuration(int seconds)
{
  timer_durationinSeconds_int = seconds;
  updateTimerStopDate();
}

void setTimerRepeat(bool isit)
{
  is_timerRepeating = isit;
  is_waiting4Schedule = true;
}

#pragma endregion

#pragma region process messages

void setup_Serial()
{
  Serial.begin(115200);

  while (!Serial)
    delay(1);
}

void processMessages(char *topic, byte *payload, unsigned int length)
{
  String incommingMessage = "";
  for (int i = 0; i < length; i++)
    incommingMessage += (char)payload[i];

  Serial.println("Message arrived [" + String(topic) + "]" + incommingMessage);

  //--- check the incomming message
  if (strcmp(topic, tc_stop_all) == 0)
  {
    stopFan();
    stopHumidifier();
  }
  else if (strcmp(topic, tc_stop_fan) == 0)
  {
    stopFan();
  }
  else if (strcmp(topic, tc_stop_humumidifier) == 0)
  {
    stopHumidifier();
  }
  else if (strcmp(topic, tc_start_all) == 0)
  {
    startFan(fanSpeedManual);
    startHumidifier(humidifierIntensityManual);

    mode_current = sw_modes::MODE_MANUAL;
  }
  else if (strcmp(topic, tc_start_fan) == 0)
  {
    int speed = std::stoi(incommingMessage.c_str());
    if (speed < 0 || speed > 255)
      speed = 0;
    fanSpeedManual = fanSpeedManual;
    startFan(fanSpeedManual);
    mode_current = sw_modes::MODE_MANUAL;
  }
  else if (strcmp(topic, tc_start_humidifier) == 0)
  {
    int intensity = std::stoi(incommingMessage.c_str());
    if (intensity < 0 || intensity > 255)
      intensity = 0;
    humidifierIntensityManual = intensity;
    startHumidifier(humidifierIntensityManual);
    mode_current = sw_modes::MODE_MANUAL;
  }
  else if (strcmp(topic, tc_set_mode) == 0)
  {
    sw_modes mode = static_cast<sw_modes>(std::stoi(incommingMessage.c_str()));

    if (mode >= sw_modes::MODE_MANUAL && mode <= sw_modes::MODE_ENERYSAVE && mode_current != mode)
    {
      mode_current = mode;
    }
  }
  else if (strcmp(topic, tc_set_timer_date) == 0)
  {
    setTimerStartDate(std::stoi(incommingMessage.c_str()), false);
  }
  else if (strcmp(topic, tc_set_timer_duration) == 0)
  {
    setTimerDuration(std::stoi(incommingMessage.c_str()));
  }
  else if (strcmp(topic, tc_set_is_timer_repeating) == 0)
  {
    setTimerRepeat((incommingMessage == "true" || incommingMessage == "True" || incommingMessage == "TRUE" || incommingMessage == "1"));
  }
  else if (strcmp(topic, tc_set_ledEffect) == 0)
  {
    led_effects eff = static_cast<led_effects>(std::stoi(incommingMessage.c_str()));

    if (eff >= led_effects::EFF_OFF && eff <= led_effects::EFF_RAINBOW && ledEff_current != eff)
    {
      ledEff_current = eff;
    }
  }
  else if (strcmp(topic, tc_set_ledColor) == 0)
  {
    if (isValidHtmlColor(incommingMessage.c_str()))
      ledColor_current = htmlToColor(incommingMessage.c_str());
  }
  else if (strcmp(topic, tc_set_ledBrightness) == 0)
  {
    int brightness = std::stoi(incommingMessage.c_str());
    led_changeBrightness(brightness);
  }
  else
  {
    // TODO?
  }
}

void processStreamBuffer(const char *buffer)
{
  // Process the received message from the buffer
  // For example, you can extract topic and payload like before
  String message = buffer;
  int idx_separator = message.indexOf('=');
  if (message.startsWith("$") && idx_separator != -1)
  {
    String topic = message.substring(1, idx_separator);
    String payload = message.substring(idx_separator + 1);

    // Convert the topic and payload to char arrays
    char topicArray[50];   // Adjust the size based on your requirements
    char payloadArray[50]; // Adjust the size based on your requirements
    topic.toCharArray(topicArray, sizeof(topicArray));
    payload.toCharArray(payloadArray, sizeof(payloadArray));

    // Call the processMessages function with the extracted data
    processMessages(topicArray, (byte *)payloadArray, payload.length());
  }
}

void loop_Serial()
{
  // Check for incoming data on Serial port
  if (Serial.available() > 0)
  {
    // Read the data from Serial
    String serialData = Serial.readString();

    int idx_separator = serialData.indexOf('=');

    // Check if the message format is correct
    if (serialData.startsWith("$") && idx_separator != -1)
    {
      // Remove any spaces from the message
      serialData.replace(" ", "");

      // Extract the topic and payload from the message
      String topic = serialData.substring(1, idx_separator);
      String payload = serialData.substring(idx_separator + 1);

      // Convert the topic and payload to char arrays
      char topicArray[50];   // Adjust the size based on your requirements
      char payloadArray[50]; // Adjust the size based on your requirements
      topic.toCharArray(topicArray, sizeof(topicArray));
      payload.toCharArray(payloadArray, sizeof(payloadArray));

      // Call the processMessages function with the extracted data
      processMessages(topicArray, (byte *)payloadArray, payload.length());
    }
  }
}

#pragma endregion

#pragma region WiFi

#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

#pragma region MQTT Certificate

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
String ssid = "Misafir 2.4";
String password = "Filtrekahve24";

int selected_ssid = 0;

//---- MQTT Broker settings
const char *mqtt_server = "62b784c5ce924ac796a727f4f9c84987.s2.eu.hivemq.cloud"; // replace with your broker url
const char *mqtt_username = "ozntyr";
const char *mqtt_password = "asdf1298";
const int mqtt_port = 8883;

WiFiClientSecure espClient; // for no secure connection use WiFiClient instead of WiFiClientSecure
// WiFiClient espClient;
PubSubClient client(espClient);

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

      client.subscribe(tc_stop_all);
      client.subscribe(tc_stop_fan);
      client.subscribe(tc_stop_humumidifier);
      client.subscribe(tc_start_all);
      client.subscribe(tc_start_fan);
      client.subscribe(tc_start_humidifier);
      client.subscribe(tc_set_mode);
      client.subscribe(tc_set_timer_date);
      client.subscribe(tc_set_timer_duration);
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

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
  processMessages(topic, payload, length);
}

void publish_Message(const char *topic, String payload, boolean retained)
{
  if (client.publish(topic, payload.c_str(), true))
    Serial.println("Message publised [" + String(topic) + "]: " + payload);
}

#pragma endregion

#pragma region BLE

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define MSG_BUFFER_SIZE (50)
char msg_ble[MSG_BUFFER_SIZE];

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

void Send_BleMessage(String msg_ble)
{
  if (pServer->getConnectedCount() > 0)
  {
    pCharacteristic->setValue(msg_ble.c_str());
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
float AQI_PPMValues[] = {12, 35.5, 55.5, 150.5, 250.5, 500.5};
int AQI_StandartLen = 6;

int AQI_idx = 0;

float AQI_PM25;

// Air quality measurement
void measure_AirQuality()
{
}

// Pollutant identification
void identify_Pollutants()
{
  // TO DO?
}

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
void control_AirPurifier()
{
  // TO DO?
}

// Real-time feedback and alerts
void update_AQI()
{
  calculate_AQI(sens_dust_pm25);
}
void check_AirQuality_Threshold()
{
  update_AQI();
  identify_Pollutants();

  control_AirPurifier();
}

// Settings and customization
void set_AirQuality_Thresholds()
{
  // TO DO?
}
void set_Notification_Preferences()
{
  // TO DO?
}
void set_Purifier_Control_Settings()
{
  // TO DO?
}

// Data logging and analysis
void log_AirQuality_Data()
{
  // TO DO?
}
void analyze_AirQuality_Trends()
{
  // TO DO?
}

#pragma endregion

#pragma region Tasks

void updateModeState()
{
  switch (mode_current)
  {
  case sw_modes::MODE_AUTO:
    if (AQI_PM25 <= AQI_StandartValues[0]) // 50
    {
      startFan(64);
      startHumidifier(64);
    }
    else if (AQI_PM25 <= AQI_StandartValues[1]) // 100
    {
      startFan(96);
      startHumidifier(96);
    }
    else if (AQI_PM25 <= AQI_StandartValues[2]) // 150
    {
      startFan(128);
      startHumidifier(128);
    }
    else // 150+
    {
      startFan(230);
      startHumidifier(230);
    }
    break;
  case sw_modes::MODE_SLEEP:
    if (AQI_PM25 >= AQI_StandartValues[2]) // 150+
    {
      startFan(102);
      startHumidifier(102);
    }
    else
    {
      startFan(51);
      startHumidifier(51);
    }
    break;
  case sw_modes::MODE_TURBO:
    if (AQI_PM25 <= AQI_StandartValues[0]) // 50
    {
      startFan(128);
      startHumidifier(128);
    }
    else if (AQI_PM25 <= AQI_StandartValues[1]) // 100
    {
      startFan(160);
      startHumidifier(160);
    }
    else if (AQI_PM25 <= AQI_StandartValues[2]) // 150
    {
      startFan(204);
      startHumidifier(204);
    }
    else // 150+
    {
      startFan(244);
      startHumidifier(244);
    }
    break;
  case sw_modes::MODE_ENERYSAVE:
    if (AQI_PM25 <= AQI_StandartValues[0]) // 50
    {
      startFan(51);
      startHumidifier(51);
    }
    else if (AQI_PM25 <= AQI_StandartValues[1]) // 100
    {
      startFan(75);
      startHumidifier(75);
    }
    else if (AQI_PM25 <= AQI_StandartValues[2]) // 150
    {
      startFan(90);
      startHumidifier(90);
    }
    else // 150+
    {
      startFan(128);
      startHumidifier(128);
    }
    break;
  case sw_modes::MODE_MANUAL:
  default:
    // there should be no need for update on this part
    break;
  }
}

TaskHandle_t hndl_PushAll;
TaskHandle_t hndl_UpdateLedC;
TaskHandle_t hndl_DustSens;
TaskHandle_t hndl_Dht11Sens;
TaskHandle_t hndl_NTP;
TaskHandle_t hndl_StreamRead;
TaskHandle_t hndl_WS2812;
TaskHandle_t hndl_BMP280;

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
      publish_Message(tr_temperature, String(sens_temperature), true);
      publish_Message(tr_humidity, String(sens_humidity), true);
    }
    if (isnan(sens_dust_pm25))
    {
      Serial.println(F("Failed to read from Dust sensor!"));
    }
    else
    {
      check_AirQuality_Threshold();
      Serial.printf("sen3: %f \n", AQI_PM25);
      publish_Message(tr_dust_pm25, String(AQI_PM25), true);
    }

    // Update mode state to calculate fan speed and humidifier intensity
    updateModeState();

    // Control the fan and humidifier based on the variables
    // ledcWrite(FAN_CHANNEL, fanSpeed);
    // ledcWrite(HUMIDIFIER_CHANNEL, humidifierIntensity);

    vTaskDelay(pdMS_TO_TICKS(dly_loop));
  }
}

void Task_UpdateLedC(void *parameter)
{
  (void)parameter;

  setup_ledc();

  for (;;)
  {

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

void Task_NTP(void *parameter)
{
  (void)parameter;
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("NTP requires wifi connection!");
    vTaskDelay(pdMS_TO_TICKS(dly_loop));
  }

  timeClient.begin();

  Serial.println("Connected to NTP");

  timeClient.update();

  for (;;)
  {

    timeClient.update();

    Serial.print("Current ");
    getTimeFromNTP(timer_currentDate_tm);
    printDateTime(timer_currentDate_tm);

    if (mode_current == sw_modes::MODE_MANUAL)
    {
      if (is_waiting4Schedule && mktime(&timer_currentDate_tm) >= mktime(&timer_startDate_tm))
      {
        startFan(fanSpeedManual);
        startHumidifier(humidifierIntensityManual);
        is_onSchedule = true;
        is_waiting4Schedule = false;
      }
    }
    if (is_onSchedule && mktime(&timer_currentDate_tm) >= mktime(&timer_stopDate_tm))
    {
      stopFan();
      stopHumidifier();
      is_onSchedule = false;
      if (is_timerRepeating)
        setTimerStartDate(static_cast<int>(mktime(&timer_startDate_tm)), true);
    }

    vTaskDelay(pdMS_TO_TICKS(dly_loop));
  }
}

void Task_ReadSerialStream(void *parameter)
{
  while (true)
  {
    if (Serial.available() > 0)
    {
      char character = Serial.read();
      if (character != '\n')
      {
        // Add character to buffer if it's not a newline
        if (bufferIndex < BUFFER_SIZE - 1)
        {
          buffer[bufferIndex] = character;
          bufferIndex++;
        }
      }
      else
      {
        // Process the buffer if a newline character is received
        buffer[bufferIndex] = '\0'; // Null-terminate the buffer for string manipulation
        processStreamBuffer(buffer);
        bufferIndex = 0; // Reset the buffer index for the next message
      }
      counter = 0; // Reset the counter as there is data available
    }
    else
    {
      counter++;
      if (counter > 5)
      {
        counter = 0;
        bufferIndex = 0; // Clear the buffer as no data is available for a certain period
      }
    }
    vTaskDelay(pdMS_TO_TICKS(100)); // Adjust the delay time as needed
  }
}

void Task_WS2812(void *parameter)
{

  setup_ws2812();

  for (;;)
  {

    switch (ledEff_current)
    {
    case EFF_SOLID:
      led_solidColor(ledColor_current);
      break;
    case EFF_WIPE:
      led_colorWipe(ledColor_current, 50);
      break;
    case EFF_CHASE:
      led_colorChase(ledColor_current, 50);
      break;
    case EFF_BREATHING:
      led_breathingEffect(ledColor_current);
      break;
    case EFF_FIRE:
      led_fireEffect();
      break;
    case EFF_RAINBOW:
      led_rainbowEffect();
      break;
    case EFF_OFF:
    default:
      led_turnOff();
      break;
    }

    vTaskDelay(dly_loop);
  }
}

void Task_BMP280(void *parameter)
{

  bool is_bmp1OK = false;
  bool is_bmp2OK = false;

  // Initialize sensor 1
  if (bmpSensor1.begin(BMP280_1_I2C_ADDRESS))
  {
    is_bmp1OK = true;
    Serial.println("BMP280 Sensor 1 initialized successfully!");
  }

  // Initialize sensor 2
  if (bmpSensor2.begin(BMP280_2_I2C_ADDRESS))
  {
    is_bmp2OK = true;
    Serial.println("BMP280 Sensor 2 initialized successfully!");
  }

  while (true)
  {
    if (!is_bmp1OK)
    {
      Serial.println("Sensor data retrieval error:");
      Serial.println("Could not find a valid BMP280 sensor at address 0x76, check wiring!");
    }
    else
    {
      float bmp1_altitude = bmpSensor1.readAltitude();
      float bmp1_pressure = bmpSensor1.readPressure() / 100000.0; // Convert Pa to bars
      float bmp1_temperature = bmpSensor1.readTemperature();

      Serial.printf("1st bmp280: Altitude: %.2f meters, Pressure: %.5f bars, Temperature: %.2f °C\n",
                    bmp1_altitude, bmp1_pressure, bmp1_temperature);
    }

    if (!is_bmp2OK)
    {
      Serial.println("Sensor data retrieval error:");
      Serial.println("Could not find a valid BMP280 sensor at address 0x77, check wiring!");
    }
    else
    {
      float bmp2_altitude = bmpSensor2.readAltitude();
      float bmp2_pressure = bmpSensor2.readPressure() / 100000.0; // Convert Pa to bars
      float bmp2_temperature = bmpSensor2.readTemperature();

      Serial.printf("2nd bmp280: Altitude: %.2f meters, Pressure: %.5f bars, Temperature: %.2f °C\n",
                    bmp2_altitude, bmp2_pressure, bmp2_temperature);
    }

    vTaskDelay(dly_loop); // Delay for 1000 ms
  }
}

#pragma endregion

#pragma region Arduino

void check_stackUsage()
{
  UBaseType_t stackUsage = uxTaskGetStackHighWaterMark(hndl_PushAll);
  Serial.print("hndl_PushAll usage: ");
  Serial.print(stackUsage);
  Serial.println(" words");
  stackUsage = uxTaskGetStackHighWaterMark(hndl_UpdateLedC);
  Serial.print("hndl_UpdateLedC usage: ");
  Serial.print(stackUsage);
  Serial.println(" words");
  stackUsage = uxTaskGetStackHighWaterMark(hndl_Dht11Sens);
  Serial.print("hndl_Dht11Sens usage: ");
  Serial.print(stackUsage);
  Serial.println(" words");
  stackUsage = uxTaskGetStackHighWaterMark(hndl_DustSens);
  Serial.print("hndl_DustSens usage: ");
  Serial.print(stackUsage);
  Serial.println(" words");
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

#ifdef ESP8266
  espClient.setInsecure();
#else // for the ESP32
  espClient.setCACert(root_ca); // enable this line and the the "certificate" code for secure connection
#endif

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqttCallback);

  xTaskCreate(Task_DustSensor, "Task_dustSensor", 4096, NULL, 10, &hndl_DustSens);
  xTaskCreate(Task_Dht11Sensor, "Task_dht11Sensor", 4096, NULL, 15, &hndl_Dht11Sens);
  xTaskCreate(Task_PushAll, "Task_PushAll", 4096, NULL, 20, &hndl_PushAll);
  xTaskCreate(Task_UpdateLedC, "Task_UpdateLedC", 4096, NULL, 20, &hndl_UpdateLedC);
  xTaskCreate(Task_NTP, "Task_NTP", 4096, NULL, 20, &hndl_NTP);
  xTaskCreate(Task_ReadSerialStream, "Task_ReadSerialStream", 4096, NULL, 20, &hndl_StreamRead);
  xTaskCreate(Task_WS2812, "Task_WS2812", 4096, NULL, 100, &hndl_WS2812);
  xTaskCreate(Task_BMP280, "Task_BMP280", 4096, NULL, 20, &hndl_BMP280);

  check_stackUsage();

  Serial.println("Setup Done!");
  delay(3000);
}

unsigned long tim_last;

void loop()
{
  unsigned long now = millis();
  if (now - tim_last >= dly_loop)
  {

    // check_stackUsage();

    tim_last = now;
  }
}

#pragma endregion
