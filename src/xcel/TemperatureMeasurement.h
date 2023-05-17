#pragma once

#include <Arduino.h>

class TemperatureMeasurement
{
public:
    TemperatureMeasurement();

    // Activation/Deactivation
    void enable_TMS();
    void disable_TMS();
    bool is_TMS_Enabled();

    // Temperature measurement
    void measure_Temperature();
    float get_Temperature();

    // Desired temperature level setting
    void set_DesiredTemperatureLevel(float temperatureLevel);
    float get_DesiredTemperatureLevel();

    // Real-time feedback and display
    void update_TemperatureDisplay();

    // Automatic temperature control
    void control_HVAC();

    // Temperature threshold alerts
    void check_TemperatureThresholds();

    // Settings and customization
    void set_TemperatureLevels();
    void set_ThresholdLimits();
    void set_NotificationPreferences();

    // Maintenance and malfunction warning
    void check_Maintenance();
    void check_Malfunction();

    // Timer and scheduling
    void set_Timer();
    void set_Schedule();

    // Data logging and analysis
    void log_TemperatureData();
    void analyze_TemperatureTrends();

private:
    // Member variables
    float temperature_;
    float desiredTemperatureLevel_;
    bool tmsEnabled_;

    // Add any private member variables or functions here
};
