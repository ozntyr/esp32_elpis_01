#pragma once

#include <Arduino.h>

class AirPurificationFans
{
public:
    AirPurificationFans();

    // Activation/Deactivation
    void enable_APF();
    void disable_APF();
    bool is_APF_Enabled();

    // Fan speed control
    void set_FanSpeed(int speed);
    int get_FanSpeed();

    // Airflow direction and oscillation
    void set_AirflowDirection(const String &direction);
    String get_AirflowDirection();
    void enable_Oscillation();
    void disable_Oscillation();
    bool is_Oscillation_Enabled();

    // Real-time feedback and display
    void update_FanSpeedDisplay();
    void update_AirflowDirectionDisplay();
    void update_OscillationStatusDisplay();

    // Automatic fan control
    void control_Fan();

    // Energy-efficient operation
    void set_EnergySavingMode(const String &mode);
    String get_EnergySavingMode();

    // Settings and customization
    void set_FanPreferences();
    void set_AirflowDirectionPreferences();
    void set_OscillationPreferences();
    void set_EnergySavingPreferences();

    // Maintenance and malfunction warning
    void check_Maintenance();
    void check_Malfunction();

    // Timer and scheduling
    void set_Timer();
    void set_Schedule();

    // Data logging and analysis
    void log_FanOperationData();
    void analyze_FanOperationTrends();

private:
    // Fan speed control
    int fanSpeed_;

    // Airflow direction and oscillation
    String airflowDirection_;
    bool oscillationEnabled_;

    // Energy-efficient operation
    String energySavingMode_;

    // Add any private member variables or functions here
};
