/*
 * Virtuino MQTT getting started example
 * Broker: HiveMQ (Secure connection)
 * Supported boards: ESP8266 / ESP32
 * Created by Ilias Lamprou
 * Jul 13 2021
 */

#include <DHT.h>
#include <GP2YDustSensor.h>
#include <driver/ledc.h>

#include "sw_fs.h"
#include "sw_wifi.h"
#include "sw_ble.h"

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

char logBuffer[255];

void setup_Serial()
{
  Serial.begin(115200);

  while (!Serial)
    delay(1);
}

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
int AQI_StandartLen = 6;

float AQI_PM25;

// Air quality measurement
void measure_AirQuality()
{
}

// Pollutant identification
void identify_Pollutants();

// Air quality index (AQI) calculation
void calculate_AQI();

// Automatic air purifier control
void control_AirPurifier();

// Real-time feedback and alerts
void update_AQI()
{
  calculate_AQI();
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

#pragma endregion AQI

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
