#pragma once

#include <Arduino.h>

class AirQualityTracking
{
public:
    AirQualityTracking();

    // Activation/Deactivation
    void enable_AQT();
    void disable_AQT();
    bool is_AQT_Enabled();

    // Air quality measurement
    void measure_AirQuality();
    float get_PM25();
    float get_PM10();
    float get_VOCs();
    float get_CO2();
    float get_Temperature();
    float get_Humidity();

    // Pollutant identification
    void identify_Pollutants();

    // Air quality index (AQI) calculation
    void calculate_AQI();

    // Real-time feedback and alerts
    void update_AQI();
    void check_AirQuality_Threshold();

    // Automatic air purifier control
    void control_AirPurifier();

    // Settings and customization
    void set_AirQuality_Thresholds();
    void set_Notification_Preferences();
    void set_Purifier_Control_Settings();

    // Maintenance and malfunction warning
    void check_Sensor_Maintenance();
    void check_Sensor_Malfunction();

    // Data logging and analysis
    void log_AirQuality_Data();
    void analyze_AirQuality_Trends();

private:
    // Add any private member variables or functions here
};
