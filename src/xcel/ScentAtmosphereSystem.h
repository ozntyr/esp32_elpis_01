#pragma once

#include <Arduino.h>

class ScentAtmosphereSystem
{
public:
    ScentAtmosphereSystem();

    // Activation/Deactivation
    void enable_SAS();
    void disable_SAS();
    bool is_SAS_Enabled();

    // Aromatherapy and scent diffusion
    void start_ScentDiffusion();
    void stop_ScentDiffusion();

    // Adjustable scent intensity and duration
    void set_ScentIntensity(int intensity);
    int get_ScentIntensity();
    void set_ScentDuration(int duration);
    int get_ScentDuration();

    // Customizable scent profiles
    void set_ScentProfile(const String &profile);
    String get_ScentProfile();

    // Scent cartridges or refillable reservoir
    void replace_ScentCartridge();
    void refill_ScentReservoir();

    // Automatic scent adjustments
    void adjust_Scent();

    // Timer and scheduling
    void set_Timer();
    void set_Schedule();

    // Mobile app connectivity
    void connect_To_MobileApp();
    void update_ScentSettingsFromApp();

    // Maintenance and malfunction warning
    void check_Maintenance();
    void check_Malfunction();

    // Eco-friendly and safe materials
    void use_EcoFriendlyMaterials();
    void ensure_SafetyStandards();

    // Water level monitoring and alerts
    void monitor_WaterLevel();
    void notify_RefillNeeded();

private:
    // Adjustable scent intensity and duration
    int scentIntensity_;
    int scentDuration_;

    // Customizable scent profiles
    String scentProfile_;

    // Add any private member variables or functions here
};
