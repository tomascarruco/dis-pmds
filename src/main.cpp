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

DeviceRoutingMode permissions = DeviceRoutingModes::receives + DeviceRoutingModes::transmits;

elapsedMillis timerOne;

AES128_ESP aes128;
MainBLEServer *ble_server;
BLENetworkNode *thisBLENetworkNode;
// BLEScan *ble_scanner;

const byte key[16] = {
    0x2b, 0x7e, 0x15, 0x16,
    0x28, 0xae, 0xd2, 0xa6,
    0xab, 0xf7, 0x15, 0x88,
    0x09, 0xcf, 0x4f, 0x3c
    // -------------------
};

// The remote service we wish to connect to.
static BLEUUID serviceUUID("3b98eff8-b21d-4958-88bc-aca8688f1bb4");
// The characteristic of the remote service we are interested in.
static BLEUUID charUUID("45ca38e6-db5b-44b9-a0ab-85c2b754d60c");

static BLERemoteCharacteristic *pRemoteCharacteristic;
static BLEAdvertisedDevice *myDevice;

BLEScan *pBLEScan;
static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;

void connectToDevice()
{
  BLEClient *pClient = BLEDevice::createClient();

  if (pClient->connect(myDevice))
  {
    Serial.println("Connected!");
  }
  else
    Serial.println("Failed to connect");

  BLERemoteService *pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr)
  {
    Serial.println("Failed to get service");
    return;
  }
  else
    Serial.println("Got service");

  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  if (pRemoteCharacteristic == nullptr)
  {
    Serial.println("Failed to get characteristic");
    return;
  }
  else
    Serial.println("Got characteristic");
}

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
  /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice device)
  {
    if (!device.haveName())
      return;

    String device_name = device.getName();
    if (device_name.length() != 20)
    {
      Serial.println("Not a valid NAME length");
      return;
    }

    String device_name_start = device_name.substring(0, 2);
    if (device_name_start.equals("PMD") == !0)
    {
      Serial.println("Not a valid NAME format");
      return;
    }

    Serial.printf("Device found.... %s\n", device_name.c_str());

    BLEDevice::getScan()->stop();
    myDevice = new BLEAdvertisedDevice(device);

    Serial.println("Quitting");
  }
}; // MyAdvertisedDeviceCallbacks

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

  // Empty for no password
  // const char *password = "";
  // dispIdentifier->SetApPassword(password);

  String bleId = "PMD";
  bleId.concat(dispIdentifier->GetSSID());
  bleId = bleId.substring(0, 20);

  BLEDevice::init(bleId.c_str());
  ble_server = new MainBLEServer(new ServerCallbacks());

  // Adicionar os callbacks à caracteristica de id do dispositivo -----------------
  BLECharacteristicCallbacks *pDeviceIdentifierCharacteristicCallbacks =
      new DidCallbacks(dispIdentifier, ble_server);

  /* int err = ble_server.SetCharacteristicCallbacks(
      MainBLECharacteristics::PatientID,
      patientIdCharacteristicCallbacks
  ); */

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

  // dispIdentifier->InitWifi();
  // BLENetworkNode usage
  // Node(BLEAdvertisedDevice *deviceId);
  BLEServer *server = ble_server->GetBleServerRef();
  thisBLENetworkNode = new BLENetworkNode(server);

  thisBLENetworkNode->StartCommunicationService();
  ble_server->StartIdService();
  ble_server->StartAdvertising(bleId);
}

void sendDataToRemoteCharacteristic()
{
  Serial.println("blue");
  String blue = "Supper Cool stuff";
  pRemoteCharacteristic->writeValue(blue, sizeof(blue));
  delay(2000);
}

void handleDeviceConnection()
{
  if (!connected)
  {
    pBLEScan->start(5, false);
    connectToDevice();
    connected = true;
  }

  if (connected)
    sendDataToRemoteCharacteristic();
}

void loop()
{
  // handleDeviceConnection();
  thisBLENetworkNode->ScanForBLEDevices();
  thisBLENetworkNode->connectToDevice();
}
