#pragma once

#include <Arduino.h>

class AirPurificationFilters
{
public:
    AirPurificationFilters();

    // Filter types and layers
    void set_FilterTypes(const String &hepaType, const String &activatedCarbonType, const String &preFilterType, const String &xFilterType);
    String get_HepaFilterType();
    String get_ActivatedCarbonFilterType();
    String get_PreFilterType();
    String get_XFilterType();

    // Filter efficiency measurement
    void measure_FilterEfficiency();
    float get_HepaFilterEfficiency();
    float get_ActivatedCarbonFilterEfficiency();
    float get_PreFilterEfficiency();
    float get_XFilterEfficiency();

    // Real-time feedback and display
    void update_FilterEfficiencyDisplay();
    void update_FilterStatusDisplay();

    // Replacement reminders
    void check_FilterReplacement();

    // Filter life tracking
    void track_FilterLifespan();

    // Data logging and analysis
    void log_FilterData();
    void analyze_FilterTrends();

private:
    // Filter types and layers
    String hepaFilterType_;
    String activatedCarbonFilterType_;
    String preFilterType_;
    String xFilterType_;

    // Filter efficiency measurement
    float hepaFilterEfficiency_;
    float activatedCarbonFilterEfficiency_;
    float preFilterEfficiency_;
    float xFilterEfficiency_;

    // Filter life tracking
    unsigned long hepaFilterLifespan_;
    unsigned long activatedCarbonFilterLifespan_;
    unsigned long preFilterLifespan_;
    unsigned long xFilterLifespan_;

    // Add any private member variables or functions here
};
