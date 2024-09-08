
#include <cinttypes>
#include "DISNet.h"
#include <UUID.h>

class MyClientCallback : public BLEClientCallbacks
{
    void onConnect(BLEClient *pClient)
    {
        Serial.println("onConnect");
    }

    void onDisconnect(BLEClient *pClient)
    {

        Serial.println("onDisconnect");
    }
};

Packet::Packet(uint8_t ttl, char destination[36 + 1], PacketPriority priority)
{
    UUID uuidGen;

    this->ttl = ttl;
    strncpy(this->destination, destination, 37);

    // ID generation from UUID
    uuidGen.generate();
    strncpy(this->id, uuidGen.toCharArray(), 37);

    this->payload = Payload();

    this->priority = priority;

    this->path_length = 0;
    *this->path = {};
}

int Packet::AddNodeToPath(NodeConnectionInfo_t *node)
{
    if (this->path_length + 1 > 4)
    {
        return -1;
    }

    this->path[this->path_length++] = node;
    return this->path_length;
}

void Packet::CycleTTL()
{
    if (this->ttl - 1 < 0)
        return;

    this->ttl -= 1;
}

const char *Packet::GetDestination()
{
    return destination;
}

PacketPriority Packet::GetPacketPriority()
{
    return this->priority;
}

NodeConnectionInfo_t *Packet::GetNodeFromPath(uint index)
{
    if (index >= this->path_length)
        return nullptr;

    return this->path[index];
}

Payload::Payload(String content)
{
    this->payload = content;
    this->length = content.length();
}

int Payload::SetPayload(String content)
{
    this->payload = content;
    this->length = content.length();
    return this->length;
}

uint Payload::GetLength()
{
    return length;
}

String Payload::GetPayload()
{
    return payload;
}

Node::Node(String id, BLEAdvertisedDevice *deviceId)
{
    this->id = id;
    this->deviceId = deviceId->getAddress().toString().c_str();
}

Node::Node(BLEAdvertisedDevice *device)
{
    Node(RandomCharSequenceLen32(), device);
}

Node::Node(String deviceId)
{
    this->id = RandomCharSequenceLen32();
    this->deviceId = deviceId;
}

String Node::GetId()
{
    return id;
}

String Node::GetDeviceId()
{
    return this->deviceId;
}

/// @returns Nothing
/// @brief
void PacketQueue::SortQueueByPriority()
{
    const int MAX_QUEUE_SIZE = PacketPriority::__PRIORITY_COUNT * PACKET_QUEUE_COUNT;

    int empty_spots = 0;
    for (int i = 0; i < MAX_QUEUE_SIZE; ++i)
    {
        if (i % PACKET_QUEUE_COUNT == 0 and !this->queue[i])
        {
            empty_spots = 0;
            if (i == (MAX_QUEUE_SIZE - 1) - 2 and this->queue[i])
            {
                break;
            }
        }

        if (!this->queue[i])
        {
            empty_spots += 1;
            continue;
        }

        if (i > 0 && this->queue[i])
        {
            this->queue[i - (empty_spots--)] = this->queue[i];
            this->queue[i] = nullptr;
            continue;
        }
    }
}

PacketQueue::PacketQueue()
{
    for (int i = 0; i < PacketPriority::__PRIORITY_COUNT * PACKET_QUEUE_COUNT; ++i)
    {
        this->queue[i] = nullptr;
    }
}

PacketQueue::QueueResult PacketQueue::InsertPacket(Packet *packet)
{
    int offset = packet->GetPacketPriority();
    if (offset > PacketPriority::__PRIORITY_COUNT || offset < 0)
        return QueueResult::ERR_PacketPriorityOutOfBounds;

    int i = -1;
    for (int j = 0; j < PACKET_QUEUE_COUNT; ++j)
        if (!queue[(PACKET_QUEUE_COUNT * offset) + j])
        {
            i = j;
            break;
        }

    if (i < 0)
        return QueueResult::ERR_FailedAttemptToInsertPacket;

    queue[PACKET_QUEUE_COUNT * offset + i] = packet;
    return QueueResult::SUC_OK;
}

bool ScalablePacketQueue::__T_PriorityHasPacket(PacketPriority priority)
{
    return this->queue[priority];
}

ScalablePacketQueue::ScalablePacketQueue()
{
    this->metaData = ScalablePacketQueue::PacketQueueMeta();
    *this->queue = {};

    for (int i = 0; i < PacketPriority::__PRIORITY_COUNT; ++i)
        this->queue[i] = nullptr;
}

void ScalablePacketQueue::PushPacket(Packet *packet)
{
    N *new_node = N::Default();
    new_node->self = packet;

    if (!this->queue[packet->GetPacketPriority()])
    {
        this->queue[packet->GetPacketPriority()] = new_node;
        this->metaData.IncrementPacketCount(packet->GetPacketPriority());
        return;
    }

    N *node = this->queue[packet->GetPacketPriority()];
    while (node->next)
        node = node->next;

    new_node->previous = node;
    node->next = new_node;

    this->metaData.IncrementPacketCount(packet->GetPacketPriority());
    return;
}

Packet *ScalablePacketQueue::PopPacket(PacketPriority priority)
{
    if (!this->queue[priority])
        return nullptr;

    N *node = this->queue[priority];

    if (node->next)
    {
        this->queue[priority] = node->next;
        this->queue[priority]->previous = nullptr;
    }
    else
    {
        this->queue[priority] = nullptr;
    }

    Packet *p = node->self;
    free(node);

    this->metaData.IncrementPacketCount(priority);
    return p;
}

NetworkNode::NetworkNode()
{
    this->packetQueue = new PacketQueue();
}

void BLENetworkNode::StopBLEDeviceScan()
{
    this->pBleScanner->stop();
}

void BLENetworkNode::StartBLEDeviceScan()
{
    this->ScanForBLEDevices();
}

BLENetworkNode::BLENetworkNode(BLEServer *s /* , Node *n */)
{
    // BLE client and server registration / scanner referencing
    this->pClient = BLEDevice::createClient();
    this->pServerReference = s;
    this->pBleScanner = BLEDevice::getScan();

    // Setup ble scanner
    this->pBleScanner->setAdvertisedDeviceCallbacks(this, false, true);
    this->pBleScanner->setActiveScan(true);
    this->pBleScanner->setInterval(1000);
    this->pBleScanner->setWindow(50);

    // Criação da identificação do serviço de comunicação BLE
    this->pCommunicationService =
        this->pServerReference->createService(
            this->communicationServiceUUID
            // ---
        );

    // Criação da caracteristica do serviço de comunicação.
    uint32_t comsServiceRxCharacteristics =
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY;

    // Criação da caracteristica de receção de informação.
    this->pRXCharacteristic =
        this->pCommunicationService->createCharacteristic(
            this->rx_CharacteristicUUID,
            comsServiceRxCharacteristics
            // --------------------------------------
        );

    // Criação da caracteristica do serviço de comunicação.
    uint32_t comsServiceTxCharacteristics =
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_NOTIFY;

    // Criação da caracteristica de receção de informação.
    this->pTXCharacteristic =
        this->pCommunicationService->createCharacteristic(
            this->tx_CharacteristicUUID,
            comsServiceTxCharacteristics
            // --------------------------------------
        );

    this->pRXCharacteristic->setCallbacks(new BLENetworkNode::RxCallBacks());
}

void BLENetworkNode::StartCommunicationService()
{
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();

    // Registar advertising para o serviço de identificação
    pAdvertising->addServiceUUID(this->communicationServiceUUID);

    this->pCommunicationService->start();
}

void BLENetworkNode::SetAdvertisedDeviceCallbacks(BLEAdvertisedDeviceCallbacks *pCallbacks)
{
    this->pAdvertisedDeviceCallbacks = pCallbacks;
}

BLEUUID BLENetworkNode::GetCommunicationServiceUUID()
{
    BLEUUID uuid = BLEUUID::fromString(this->communicationServiceUUID);
    return uuid;
}

BLEUUID BLENetworkNode::GetCommunicationCharacteristicTxUUID()
{
    BLEUUID uuid = BLEUUID::fromString(this->tx_CharacteristicUUID);
    return uuid;
}

BLEUUID BLENetworkNode::GetCommunicationCharacteristicRxUUID()
{
    BLEUUID uuid = BLEUUID::fromString(this->rx_CharacteristicUUID);
    return uuid;
}

void BLENetworkNode::ScanForBLEDevices()
{
    if (this->doScan)
        this->pBleScanner->start(10, false);
}

void BLENetworkNode::onResult(BLEAdvertisedDevice device)
{
    if (this->connected)
        return;

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
    this->pBleScanner->stop();

    this->pServerReference->addPeerDevice(&device, false, peerDeviceCount++);
    std::map<uint16_t, conn_status_t> peers = this->pServerReference->getPeerDevices(false);

    /*
    for (auto elem : peers)
        Serial.printf("Conn ID: %d\n", elem.first);
    */

    int service_count = device.getServiceUUIDCount();
    Serial.printf("Device has %d services\n", service_count);
    for (int i = 0; i < service_count; ++i)
    {
        Serial.printf("Service: %s \n", device.getServiceUUID(i).toString().c_str());
        Serial.printf("* ---- ADV Service?: %s \n", device.isAdvertisingService(device.getServiceUUID(i)) ? "t" : "f");
    }

    if (device.haveServiceUUID() && device.isAdvertisingService(this->GetCommunicationServiceUUID()))
    {
        this->device = new BLEAdvertisedDevice(device);
        this->doConnect = true;
        this->doScan = false;

        Serial.println("Device is advertising the correct services");
        return;
    }

    this->doScan = true;
    this->doScan = true;
    this->connected = false;
}

bool BLENetworkNode::connectToDevice()
{
    if (!this->doConnect)
        return false;

    this->pClient->setClientCallbacks(new MyClientCallback());

    int service_count = device->getServiceUUIDCount();

    // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    bool connected = this->pClient->connect(device);
    Serial.println(" - Connected to server");

    if (!connected)
    {
        Serial.println("Failed to connect to advertised device!");
        this->connected = false;
        this->doConnect = false;
        this->doScan = true;
        return false;
    }

    Serial.println("Connected to advertised device!");

    this->connected = true;
    this->doConnect = true;
    this->doScan = false;

    return true;
    // TODO: transform the BLEAddress into a NODE
}

void BLENetworkNode::echoMessage()
{
    MsgPack::Packer packer;
    Payload payload = Payload("Some text");
    packer.pack(payload);

    BLERemoteService *pRemoteService =
        this->pClient->getService(this->GetCommunicationServiceUUID());

    if (pRemoteService == nullptr)
    {
        Serial.println("Failed to get the remote service!");
        return;
    }
    Serial.println("Got the service!");

    BLERemoteCharacteristic *pRemoteCharacteristic =
        pRemoteService->getCharacteristic(this->GetCommunicationCharacteristicRxUUID());

    if (pRemoteCharacteristic == nullptr)
    {
        Serial.println("Failed to get the remote characteristic!");
        return;
    }

    if (!pRemoteCharacteristic->canWrite())
    {
        Serial.println("Characteristic does not allow write operations");
        return;
    }

    Serial.println("Got the service!");
    if (packer.data() == nullptr)
    {
        Serial.println("Deref on nullptr in packer.data() on connectToDevice()");
        return;
    }
    // ! Probably Dangerous code, deref of a static pointer
    uint8_t *data = (uint8_t *)packer.data();
    pRemoteCharacteristic->writeValue(data, packer.size(), false);

    this->connected = true;
    this->doConnect = false;
    this->doScan = false;
}
