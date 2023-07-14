#include <Arduino.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include "sw_fs.h"

const char *ssid = "Misafir 2.4";
const char *password = "Filtrekahve24";

const char *ntpServer = "pool.ntp.org";
const long gmtOffset = 10800;
const int daylightOffset = 0;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer, gmtOffset, daylightOffset);

struct tm currentDate;
struct tm startDate;
struct tm stopDate;

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

void setTimeSpan5Min()
{
    time_t currentTime = timeClient.getEpochTime(); // Obtain current time as time_t
    int startTime = static_cast<int>(currentTime);  // Convert time_t to integer
    int stopTime = startTime + 300;                 // Add 5 mins (300 seconds)
    time_t endTime = static_cast<time_t>(stopTime); // Convert back to time_t

    localtime_r(&currentTime, &startDate);
    localtime_r(&endTime, &stopDate);
}

void setup()
{
    Serial.begin(115200);

    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println();

    timeClient.begin();

    Serial.println("Connected to WiFi");

    timeClient.update();

    Serial.print("Start ");
    getTimeFromNTP(startDate);
    printDateTime(startDate);

    stopDate = startDate;
    stopDate.tm_min += 5;

    mktime(&stopDate);

    setTimeSpan5Min();

    Serial.print("Stop ");
    printDateTime(stopDate);
}

void loop()
{
    timeClient.update();

    Serial.print("Current ");
    getTimeFromNTP(currentDate);
    printDateTime(currentDate);

    delay(1000);
}
