#pragma once

#include <Arduino.h>

class AirHumidificationSystem
{
public:
    AirHumidificationSystem();

    // Activation/Deactivation
    void enable_AHS();
    void disable_AHS();
    bool is_AHS_enabled();

    // Humidity measurement
    void measure_humidity();
    float get_RelativeHumidity();
    float get_temperature();

    // Desired humidity level setting
    void set_DesiredHumidityLevel(float humidityLevel);
    float get_DesiredHumidityLevel();

    // Real-time feedback and display
    void update_HumidityDisplay();
    void update_TemperatureDisplay();

    // Automatic humidifier control
    void control_humidifier();

    // Water level monitoring and alerts
    void monitor_WaterLevel();
    void check_WaterLevel();

    // Settings and customization
    void set_HumidityLevels();
    void set_TemperatureThresholds();
    void set_NotificationPreferences();

    // Maintenance and malfunction warning
    void check_Maintenance();
    void check_Malfunction();

    // Timer and scheduling
    void set_Timer();
    void set_Schedule();

    // Data logging and analysis
    void log_HumidityData();
    void log_TemperatureData();
    void analyze_HumidityTrends();

private:
    // Add any private member variables or functions here
};
