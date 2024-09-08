#include <Arduino.h>

// Wifi connectivity
// #include <WiFi.h>

// Timing
#include <elapsedMillis.h>

// Random 32 bit numbers and UUIDs
// #include <ESPRandom.h>

/* ---- Private Libraries ---- */
#include <Utilities.h>
#include <DIS.h>
#include <Scanning.h>
#include <DISBLE.h>
#include <DISNet.h>
/* ---- Private Libraries ---- */

#include <AES.h>
#include <Crypto.h>

// ---------------------------------------------------

void handleBLeRoutines();

DeviceRoutingMode permissions = DeviceRoutingModes::receives + DeviceRoutingModes::transmits;

elapsedMillis timerOne;

AES128_ESP aes128;
MainBLEServer *ble_server;
BLENetworkNode *thisBLENetworkNode;

const byte key[16] = {
    0x2b, 0x7e, 0x15, 0x16,
    0x28, 0xae, 0xd2, 0xa6,
    0xab, 0xf7, 0x15, 0x88,
    0x09, 0xcf, 0x4f, 0x3c
    // -------------------
};

void setup()
{
  Serial.begin(115200);

  // Wait for serial availability
  while (!Serial)
    ;

  DeviceIdentifier *dispIdentifier = new DeviceIdentifier(
      "PMD",
      RandomCharSequenceLen32(),
      DeviceOperationStates::good,
      permissions
      // -------------------------
  );

  dispIdentifier->SetAESKey(key);
  dispIdentifier->SetAESInstance(&aes128);
  dispIdentifier->GenerateSSID();

  String bleId = "PMD";
  bleId.concat(dispIdentifier->GetSSID());
  bleId = bleId.substring(0, 20);

  BLEDevice::init(bleId.c_str());
  ble_server = new MainBLEServer(new ServerCallbacks());

  // Adicionar os callbacks à caracteristica de id do dispositivo -----------------
  BLECharacteristicCallbacks *pDeviceIdentifierCharacteristicCallbacks =
      new DidCallbacks(dispIdentifier, ble_server);

  MainBLECharacteristics caracteristicas[4] = {
      MainBLECharacteristics::PatientID,
      MainBLECharacteristics::DeviceID,
      MainBLECharacteristics::DeviceState,
      MainBLECharacteristics::RoutingMode
      // ---------------------------------
  };

  int err = ble_server->SetCharacteristicsCallbacks(
      pDeviceIdentifierCharacteristicCallbacks,
      caracteristicas,
      4
      // ----
  );
  if (err != 0)
  {
    Serial.printf("FATAL: FAILED TO ADD CALLBACKS TO DEVICE ID CHARACTERISTICS\n");
    exit(-1);
  }
  delay(100);
  // ------------------------------------------------------------------------------

  // BLENetworkNode usage
  // Node(BLEAdvertisedDevice *deviceId);
  BLEServer *server = ble_server->GetBleServerRef();
  thisBLENetworkNode = new BLENetworkNode(server);

  thisBLENetworkNode->StartCommunicationService();
  BLEUUID serviceUUID = thisBLENetworkNode->GetCommunicationServiceUUID();

  ble_server->StartIdService();
  ble_server->StartAdvertising(bleId, 1, {&serviceUUID});

  Serial.println("Finished Setup");
}

void loop()
{
  thisBLENetworkNode->ScanForBLEDevices();
}

void handleBLeRoutines()
{
  thisBLENetworkNode->ScanForBLEDevices();
  bool connected = thisBLENetworkNode->connectToDevice();
  if (!connected)
  {
    return;
  }
  thisBLENetworkNode->echoMessage();
}
