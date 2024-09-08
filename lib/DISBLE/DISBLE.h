#pragma once

/* ---- BLE Libraries ---- */
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLEUUID.h>
#include <BLE2902.h>
/* ---- BLE Libraries ---- */

#include <map>

#include <UUID.h>
#include <DIS.h>

enum MainBLECharacteristics
{
    DeviceID = 0,
    PatientID,
    DeviceState,
    RoutingMode,
};

// Main BLE service - passive identification and data transmission
class MainBLEServer
{
private:
    BLEAdvertising *pAdvertising = nullptr;
    BLEServer *pServer = nullptr;
    BLEClient *pClient = nullptr;

    // BLE device identification service
    char mainServiceUUID[36 + 1] = "3b98eff8-b21d-4958-88bc-aca8688f1bb9";

    char deviceIdCharacteristicUUID[36 + 1] = "a3e5f815-f164-47a8-9798-a6f1522ee643";
    char patientIDCharacteristicUUID[36 + 1] = "05b1912c-5b45-449b-b094-111251e47be5";
    char deviceStateCharacteristicUUID[36 + 1] = "72844701-8953-4f0e-989f-bc79b340523f";
    char deviceRoutingCharacteristicUUID[36 + 1] = "435090a0-391e-435b-8d8e-92336936adc9";

    BLECharacteristic *pDeviceIdCharacteristic = nullptr;
    BLECharacteristic *pPatientIDCharacteristic = nullptr;
    BLECharacteristic *pDeviceStateCharacteristic = nullptr;
    BLECharacteristic *pDeviceRoutingCharacteristic = nullptr;

    BLEService *pIdService = nullptr;
    // --------------------------------------------

public:
    /* ----------------------
    Constructors
    ------------------------ */
    MainBLEServer();
    MainBLEServer(
        BLEServerCallbacks *pServerCallbacks
        // ---------------------------------
    );

    /* ----------------------
    Getters
    ------------------------ */
    BLEServer *GetBleServerRef()
    {
        return this->pServer;
    }
    BLEClient *GetBleClientRef()
    {
        return this->pClient;
    }

    /* ----------------------
    Setters
    ------------------------ */
    int SetCharacteristicCallbacks(MainBLECharacteristics characteristic, BLECharacteristicCallbacks *callbacksClass);
    int SetCharacteristicsCallbacks(BLECharacteristicCallbacks *callbacksClass, MainBLECharacteristics characteristic[], int charCount);

    /* ----------------------
    Behavior
    ------------------------ */
    void StartIdService();

    void StartIdentificationService();
    void StopIdentificationService();

    void StartAdvertising(String advertName, u8_t otherServicesCount, BLEUUID serviceUUIDs[]);

    int SetCharacteristicValue(MainBLECharacteristics characteristic, String value, bool notify);

    BLECharacteristic *MapUUIDToCharacteristic(const char *uuid);
    BLECharacteristic *GetCharacteristic(MainBLECharacteristics characteristic) const;
    BLEUUID GetCharacteristicUUID(MainBLECharacteristics characteristic) const;

    void RegisterExtensionService(BLEService *);
};

/*
----------------------------------
    DeviceIdentifier Callbacks
----------------------------------
*/

class DidCallbacks : public BLECharacteristicCallbacks
{
    // Class inheritance method implementations
    void onRead(BLECharacteristic *pCharacteristic);
    void onNotify(BLECharacteristic *pCharacteristic);

public:
    MainBLEServer *pBleServer;
    DeviceIdentifier *pDeviceIdentifier;

    DidCallbacks(DeviceIdentifier *pDeviceIdentifier, MainBLEServer *pBleServer);
};

/*
----------------------------------
        Server Callbacks
----------------------------------
*/

class ServerCallbacks : public BLEServerCallbacks
{
    void onConnect(BLEServer *pServer);
    void onDisconnect(BLEServer *pServer);
};
