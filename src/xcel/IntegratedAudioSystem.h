#pragma once

#include <Arduino.h>

class IntegratedAudioSystem
{
public:
    IntegratedAudioSystem();

    // Activation/Deactivation
    void enable_IAS();
    void disable_IAS();
    bool is_IAS_Active();

    // Built-in speaker system
    void play_Audio(const String &audioFile);
    void stop_Audio();

    // Sound profiles and customization
    void set_SoundProfile(const String &profile);
    String get_SoundProfile();

    // Adjustable volume and audio settings
    void set_Volume(int volume);
    int get_Volume();
    void adjust_AudioSettings();

    // Bluetooth connectivity
    void enable_Bluetooth();
    void disable_Bluetooth();
    bool is_Bluetooth_Enabled();
    void connect_To_BluetoothDevice(const String &deviceName);
    void disconnect_From_BluetoothDevice();

    // Timer and scheduling
    void set_Timer();
    void set_Schedule();

    // Compatibility with voice assistants
    void enable_VoiceAssistantSupport();
    void disable_VoiceAssistantSupport();
    bool is_VoiceAssistantSupported();
    void connect_To_VoiceAssistant(const String &assistantName);
    void disconnect_From_VoiceAssistant();

private:
    // Adjustable volume and audio settings
    int volume_;

    // Sound profiles and customization
    String soundProfile_;

    // Bluetooth connectivity
    bool bluetoothEnabled_;
    String connectedBluetoothDevice_;

    // Compatibility with voice assistants
    bool voiceAssistantSupported_;
    String connectedVoiceAssistant_;

    // Add any private member variables or functions here
};
