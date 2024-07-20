#include <DISBLE.h>

MainBLEServer::MainBLEServer()
{
}

MainBLEServer::MainBLEServer(
    BLEServerCallbacks *pBleServerCallbacks
    // BLECharacteristicCallbacks *pDeviceDataCallback
    // ---------------------------------------------
)
{
    // Criação do cliente BLE
    this->pClient = BLEDevice::createClient();
    // this->pClient->setCallbacks(pBleServerCallbacks);

    // Criação do servidor BLE
    this->pServer = BLEDevice::createServer();
    this->pServer->setCallbacks(pBleServerCallbacks);

    this->uuidGen.setRandomMode();
    this->uuidGen.setVariant4Mode();

    // Criação dos UUID do Serviço de identificação e as sua caracteristica
    // this->uuidGen.generate();
    // strncpy(this->MainServiceUUID, this->uuidGen.toCharArray(), 37);

    this->uuidGen.generate();
    strncpy(this->deviceIdCharacteristicUUID, this->uuidGen.toCharArray(), 37);

    this->uuidGen.generate();
    strncpy(this->patientIDCharacteristicUUID, this->uuidGen.toCharArray(), 37);

    this->uuidGen.generate();
    strncpy(this->deviceStateCharacteristicUUID, this->uuidGen.toCharArray(), 37);

    this->uuidGen.generate();
    strncpy(this->deviceRoutingCharacteristicUUID, this->uuidGen.toCharArray(), 37);
    // -----------------------------------------------

    // Criação da identificação do serviço BLE
    this->pIdService =
        this->pServer->createService(
            this->mainServiceUUID
            // -----------------------------------------
        );

    // Criação da caracteristica do serviço de identificação.
    uint32_t generalReadNotifyCharacteristics =
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_NOTIFY;

    this->pDeviceIdCharacteristic =
        this->pIdService->createCharacteristic(
            this->deviceIdCharacteristicUUID,
            generalReadNotifyCharacteristics
            // --------------------------------------
        );

    this->pPatientIDCharacteristic =
        this->pIdService->createCharacteristic(
            this->patientIDCharacteristicUUID,
            generalReadNotifyCharacteristics
            // --------------------------------------
        );

    this->pDeviceRoutingCharacteristic =
        this->pIdService->createCharacteristic(
            this->deviceRoutingCharacteristicUUID,
            generalReadNotifyCharacteristics
            // --------------------------------------
        );

    this->pDeviceStateCharacteristic =
        this->pIdService->createCharacteristic(
            this->deviceStateCharacteristicUUID,
            generalReadNotifyCharacteristics
            // --------------------------------------
        );

    // Descrição das caracteristicas relacionadas com o DID
    BLE2902 *deviceIdDescriptor = new BLE2902();
    deviceIdDescriptor->setValue("Device ID");
    this->pDeviceIdCharacteristic->addDescriptor(deviceIdDescriptor);

    BLE2902 *patientIdDescriptor = new BLE2902();
    patientIdDescriptor->setValue("Patient ID");
    this->pPatientIDCharacteristic->addDescriptor(patientIdDescriptor);

    BLE2902 *deviceRoutingDescriptor = new BLE2902();
    deviceRoutingDescriptor->setValue("Routing Mode");
    this->pDeviceRoutingCharacteristic->addDescriptor(deviceRoutingDescriptor);

    BLE2902 *deviceStateDescriptor = new BLE2902();
    deviceStateDescriptor->setValue("Device State");
    this->pDeviceStateCharacteristic->addDescriptor(deviceStateDescriptor);
}

int MainBLEServer::SetCharacteristicCallbacks(MainBLECharacteristics characteristic, BLECharacteristicCallbacks *callbacksClass)
{
    BLECharacteristic *serviceCharacteristic = this->GetCharacteristic(characteristic);
    if (characteristic == NULL)
    {
        Serial.printf("FAILED to set callbacks to characteristic: %s\n", characteristic);
        return -1;
    }

    Serial.printf("SETTING CALLBACKS\n");

    serviceCharacteristic->setCallbacks(callbacksClass);
    return 0;
}

int MainBLEServer::SetCharacteristicsCallbacks(BLECharacteristicCallbacks *callbacksClass, MainBLECharacteristics characteristic[], int characteristicCount)
{
    for (int i = 0; i < characteristicCount; ++i)
    {
        // this->SetCharacteristicCallbacks(characteristic[i], callbacksClass);
        BLECharacteristic *serviceCharacteristic = this->GetCharacteristic(characteristic[i]);
        if (characteristic == NULL)
        {
            Serial.printf("FAILED to set callbacks to characteristic: %s\n", characteristic);
            return -1;
        }

        // Serial.printf("SETTING CALLBACKS\n");

        serviceCharacteristic->setCallbacks(callbacksClass);
    };

    return 0;
}

void MainBLEServer::StartIdService()
{
    this->pIdService->start();
}

void MainBLEServer::StartIdentificationService()
{
    this->pIdService->start();
}

void MainBLEServer::StopIdentificationService()
{
    this->pIdService->stop();
}

void MainBLEServer::StartAdvertising(String advertName)
{
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();

    // Registar advertising para o serviço de identificação
    pAdvertising->addServiceUUID(this->mainServiceUUID);

    BLEAdvertisementData advertData = BLEAdvertisementData();
    advertData.setName(advertName.c_str());
    advertData.setAppearance(ESP_BLE_APPEARANCE_GENERIC_PULSE_OXIMETER);
    advertData.setShortName("DIS-PMD");

    pAdvertising->setScanResponseData(advertData);

    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x0);

    pAdvertising->start();
}

int MainBLEServer::SetCharacteristicValue(MainBLECharacteristics characteristic, String value, bool notify)
{
    BLECharacteristic *serviceCharacteristic = this->GetCharacteristic(characteristic);

    if (!characteristic)
    {
        Serial.printf("FAILED to set characteristic value: %s\n", characteristic);
        return -1;
    }

    serviceCharacteristic->setValue(value.c_str());
    if (notify)
        serviceCharacteristic->notify();

    return 0;
}

BLECharacteristic *MainBLEServer::MapUUIDToCharacteristic(const char *uuid)
{
    if (strcmp(uuid, this->deviceIdCharacteristicUUID))
    {
        return this->pDeviceIdCharacteristic;
    }
    else if (strcmp(uuid, this->patientIDCharacteristicUUID))
    {
        return this->pPatientIDCharacteristic;
    }
    else if (strcmp(uuid, this->deviceRoutingCharacteristicUUID))
    {
        return this->pDeviceRoutingCharacteristic;
    }
    else if (strcmp(uuid, this->deviceStateCharacteristicUUID))
    {
        return this->pDeviceRoutingCharacteristic;
    }
    else
    {
        return NULL;
    }
}

BLECharacteristic *MainBLEServer::GetCharacteristic(MainBLECharacteristics characteristic) const
{
    switch (characteristic)
    {
    case MainBLECharacteristics::DeviceID:
        return this->pDeviceIdCharacteristic;

    case MainBLECharacteristics::PatientID:
        return this->pPatientIDCharacteristic;

    case MainBLECharacteristics::RoutingMode:
        return this->pDeviceRoutingCharacteristic;

    case MainBLECharacteristics::DeviceState:
        return this->pDeviceStateCharacteristic;

    default:
        return NULL;
    }
}

BLEUUID MainBLEServer::GetCharacteristicUUID(MainBLECharacteristics characteristic) const
{
    BLEUUID uuid = this->GetCharacteristic(characteristic)->getUUID();
    return uuid;
}

void MainBLEServer::RegisterExtensionService(BLEService *)
{
    this->pServer->createService(
        this->mainServiceUUID
        // -----------------------------------------
    );
}

/*
=====================================================
   BLE identification service and server callbacks
=====================================================
*/

/*
----------------------------------
        Patient Callbacks
----------------------------------
*/

void DidCallbacks::onRead(BLECharacteristic *pCharacteristic)
{
    BLEUUID charUuid = pCharacteristic->getUUID();

    // UUID das caracteristicas que vão ser tratadas por este handler
    BLEUUID IdUUID = this->pBleServer->GetCharacteristicUUID(MainBLECharacteristics::DeviceID);
    BLEUUID patientIdUUID = this->pBleServer->GetCharacteristicUUID(MainBLECharacteristics::PatientID);
    BLEUUID deviceStateUUID = this->pBleServer->GetCharacteristicUUID(MainBLECharacteristics::DeviceState);
    BLEUUID deviceRoutingUUID = this->pBleServer->GetCharacteristicUUID(MainBLECharacteristics::RoutingMode);

    String value = "EMPTY";
    if (charUuid.equals(IdUUID))
    {
        value = this->pDeviceIdentifier->GetDeviceID();
    }
    else if (charUuid.equals(patientIdUUID))
    {
        value = this->pDeviceIdentifier->GetPatientID();
    }
    else if (charUuid.equals(deviceStateUUID))
    {
        value = this->pDeviceIdentifier->GetDeviceState();
    }
    else if (charUuid.equals(deviceRoutingUUID))
    {
        value = this->pDeviceIdentifier->GetRoutingMode();
    }
    else
    {
        Serial.printf("ERRO: Caracteristica não conhecida!!!\n");
    }

    pCharacteristic->setValue(value.c_str());
    pCharacteristic->notify();
}
void DidCallbacks::onNotify(BLECharacteristic *pCharacteristic)
{
    Serial.printf("BLE NOTIFY VALUE\n");
}

/// @brief Constructs the callbacks class for the patient characteristic
/// @param pDeviceIdentifier Class holding device information
DidCallbacks::DidCallbacks(DeviceIdentifier *pDeviceIdentifier, MainBLEServer *pBleServer)
{
    this->pBleServer = pBleServer;
    this->pDeviceIdentifier = pDeviceIdentifier;
}

/*
----------------------------------
        Server Callbacks
----------------------------------
*/

void ServerCallbacks::onConnect(BLEServer *pServer)
{
    // Pass
}
void ServerCallbacks::onDisconnect(BLEServer *pServer)
{
    pServer->startAdvertising();
}
