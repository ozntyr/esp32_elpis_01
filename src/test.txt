/*
 * Virtuino MQTT getting started example
 * Broker: HiveMQ (Secure connection)
 * Supported boards: ESP8266 / ESP32
 * Created by Ilias Lamprou
 * Jul 13 2021
 */

#include <DHT.h>
#include <GP2YDustSensor.h>

#include "sw_fs.h"
#include "sw_wifi.h"

unsigned long lastMsg = 0;

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
int command1 = 0;

const uint8_t SHARP_LED_PIN = 32; // Sharp Dust/particle sensor Led Pin
const uint8_t SHARP_VO_PIN = 36;  // Sharp Dust/particle analog out pin used for reading

GP2YDustSensor dustSensor(GP2YDustSensorType::GP2Y1010AU0F, SHARP_LED_PIN, SHARP_VO_PIN);
char logBuffer[255];

void setup_Serial()
{
  Serial.begin(115200);

  while (!Serial)
    delay(1);
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

  Serial.println("Setup Done!");
  delay(3000);
}

void loop()
{

  if (!client.connected())
    reconnect();
  client.loop();

  //---- example: how to publish sensor values every 5 sec
  unsigned long now = millis();
  if (now - lastMsg > dly_loop)
  {
    lastMsg = now;
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
  }
}
