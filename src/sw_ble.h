
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLECharacteristic *pCharacteristic;

void setup_Ble()
{
    BLEDevice::init("ESP32");
    BLEServer *pServer = BLEDevice::createServer();
    BLEService *pService = pServer->createService(BLEUUID((uint16_t)0x180F));

    pCharacteristic = pService->createCharacteristic(
        BLEUUID((uint16_t)0x2A19),
        BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_NOTIFY);

    pCharacteristic->addDescriptor(
        new BLE2902());

    pService->start();
    pServer->getAdvertising()->start();
}

void Send_BleMessage(String msg)
{
    msg = "Hello, World!";
    pCharacteristic->setValue(msg.c_str());
    pCharacteristic->notify();
}
