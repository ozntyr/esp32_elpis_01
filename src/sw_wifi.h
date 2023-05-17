
#pragma once

#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

#include "sw_fs.h"

#include "mqtt_cert.h"

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
