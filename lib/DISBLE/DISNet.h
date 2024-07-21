#pragma once

/* ---- BLE Libraries ---- */
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLEUUID.h>
#include <BLE2902.h>
/* ---- BLE Libraries ---- */

/* ---- Private Libs ---- */
#include <MsgPack.h>
#include <DISBLE.h>
#include <DIS.h>
/* ---- Private Libs ---- */

struct NodeConnectionInfo_t
{
    MsgPack::str_t nodeAddress;
    int rssi;

    MSGPACK_DEFINE(rssi, nodeAddress);
};

static const int PACKET_QUEUE_COUNT = 3;

enum PacketPriority
{
    Severe,
    High,
    Regular,
    None,
    __PRIORITY_COUNT
};

struct Payload
{
    MsgPack::str_t payload;
    uint length;

    ~Payload() {};
    Payload(String content);
    Payload() : payload(""), length(0) {};

    // ---- Setters START ----
    int SetPayload(String content);
    // ---- Setters END ----

    // ---- Getters START ----
    uint GetLength();
    String GetPayload();
    // ---- Getters END ----

    MSGPACK_DEFINE(length, payload);
};

class Packet
{
private:
    uint8_t ttl;
    PacketPriority priority;
    char id[36 + 1];
    char destination[36 + 1];

    uint8_t path_length;
    NodeConnectionInfo_t *path[4];

public:
    Payload payload;

    // ---- Constructor ----
    Packet(uint8_t ttl, char destination[36 + 1], PacketPriority priority);
    Packet() : ttl(14), priority(PacketPriority::Regular), destination("emtpy"), id("empty") {};

    // ---- Getters START ----
    const char *GetDestination();
    PacketPriority GetPacketPriority();
    NodeConnectionInfo_t *GetNodeFromPath(uint index);
    // ---- Getters END ----

    // ---- Behavior START ----
    void CycleTTL();
    int AddNodeToPath(NodeConnectionInfo_t *node);
    // TODO: Add MsgPacking to the Packet field, being called from this class
    // ---- Behavior END ----
};

class PacketQueue
{
public:
    // --- Class inner types START ---
    typedef Packet *Queue[PacketPriority::__PRIORITY_COUNT * PACKET_QUEUE_COUNT];
    enum QueueResult
    {
        ERR_PacketPriorityOutOfBounds,
        ERR_FailedAttemptToInsertPacket,
        SUC_OK,
    };
    // --- Class inner types END ---

    Queue queue;

    // --- Construtores START ---
    PacketQueue();
    // --- Construtores END ---

    // ---- Behavior START ----
    PacketQueue::QueueResult InsertPacket(Packet *packet);
    void SortQueueByPriority();
    // ---- Behavior END ----
};

struct Node
{
    // The address must be unique
    MsgPack::str_t id;
    MsgPack::str_t deviceId;

    MSGPACK_DEFINE(id, deviceId);

public:
    // ---- Constructors START ----
    Node(String id, BLEAdvertisedDevice *deviceId);
    Node(BLEAdvertisedDevice *deviceId);
    Node(String id);
    // ---- Constructors END ----

    // ---- Getters START ----
    String GetId();
    String GetDeviceId();
    // ---- Getters END ----
};

class NetworkNode
{
    // uint8_t CalculateNodeWeight(std::string destination, uint8_t tolerance);
private:
    PacketQueue *packetQueue;

public:
    ~NetworkNode() {};
    NetworkNode();
};

class ScalablePacketQueue
{
    // --- Class types START ---
    enum Result
    {
        ERR_PacketPriorityOutOfBounds,
        ERR_PacketCountFailedInvalidIndex,
        // ----
        ERR_FailedAttemptToInsertPacket,
        SUC_OK,
    };
    // --- Class types END ---

private:
    // --- Inner Class types START ---
    struct N
    {
        Packet *self;
        N *next;
        N *previous;

    public:
        static N *Default()
        {
            N *node;
            node = (N *)malloc(sizeof(struct N));

            node->self = nullptr;
            node->next = nullptr;
            node->previous = nullptr;

            return node;
        }
    };
    typedef N *Queue[PacketPriority::__PRIORITY_COUNT];
    struct PacketQueueMeta
    {
    private:
        uint packetCount[PacketPriority::__PRIORITY_COUNT];

    public:
        /// @brief Cria a meta structure que armazena meta dados necessários para a queue;
        PacketQueueMeta()
        {
            for (int i = 0; i < PacketPriority::__PRIORITY_COUNT; ++i)
                packetCount[i] = 0;
        };

        // ---- Getters Start ----
        uint getPacketCount(PacketPriority priority)
        {
            if (priority < 0 or priority > PacketPriority::__PRIORITY_COUNT - 1)
                return -1;

            return this->packetCount[priority];
        };
        // ---- Getters END ----

        ScalablePacketQueue::Result IncrementPacketCount(PacketPriority packetType)
        {
            if (packetType < 0 or packetType > PacketPriority::__PRIORITY_COUNT - 1)
                return ScalablePacketQueue::ERR_PacketPriorityOutOfBounds;

            this->packetCount[packetType] += 1;
            return ScalablePacketQueue::SUC_OK;
        };

        ScalablePacketQueue::Result DecrementPacketCount(PacketPriority packetType)
        {
            if (packetType < 0 or packetType > PacketPriority::__PRIORITY_COUNT - 1)
                return ScalablePacketQueue::ERR_PacketPriorityOutOfBounds;

            this->packetCount[packetType] -= 1;
            return ScalablePacketQueue::SUC_OK;
        };
    };
    // --- Inner Class types END ---

    // Class data members;
    Queue queue;
    PacketQueueMeta metaData;

public:
    // TESTING METHODS
    bool __T_PriorityHasPacket(PacketPriority);

    // Constructors
    ScalablePacketQueue();

    // Destructor
    // TODO implement this method
    // ~ScalablePacketQueue();

    // ---- Behavior START ----
    void PushPacket(Packet *);
    Packet *PopPacket(PacketPriority);
    // ---- Behavior END ----
};

class BLENetworkNode : public BLEAdvertisedDeviceCallbacks
{
private:
    BLEAdvertisedDevice *device = nullptr;

    bool doScan = true;
    bool doConnect = false;
    bool connected = false;

    // Inheritance
    void onResult(BLEAdvertisedDevice);
    // ---

    BLEClient *pClient = nullptr;
    BLEServer *pServerReference = nullptr;

    BLEScan *pBleScanner = nullptr;
    Node *BleNodes[4] = {
        nullptr, nullptr,
        nullptr, nullptr
        // ---
    };

    // BLEAdvertise callbacks
    BLEAdvertisedDeviceCallbacks *pAdvertisedDeviceCallbacks;

    // BLE device network communication service
    uint32_t comsServiceRxCharacteristics =
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_NOTIFY;

    uint32_t comsServiceTxCharacteristics =
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY;

    const char communicationServiceUUID[36 + 1] = "3b98eff8-b21d-4958-88bc-aca8688f1bb4";

    char tx_CharacteristicUUID[36 + 1] = "cc6b17c4-815b-4e01-86ab-cbbf9c467425";
    char rx_CharacteristicUUID[36 + 1] = "45ca38e6-db5b-44b9-a0ab-85c2b754d60c";

    BLECharacteristic *pTXCharacteristic = nullptr;

    class RxCallBacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *cr)
        {
            MsgPack::Unpacker unpacker;
            Payload pyl = Payload();

            unpacker.feed(cr->getData(), cr->getLength());
            unpacker.deserialize(pyl);

            Serial.printf("The value is: %s\n", pyl.GetPayload().c_str());
        }
    };
    BLECharacteristic *pRXCharacteristic = nullptr;

    BLEService *pCommunicationService = nullptr;
    // Node *node = nullptr;

public:
    enum ServiceDenomination
    {
        Tx,
        Rx
    };

    /* ----------------------
    Constructors
    ------------------------ */
    BLENetworkNode(BLEServer * /* , Node * */);

    /* ----------------------
    Behaviour
    ------------------------ */
    void StartCommunicationService();
    void ScanForBLEDevices();

    /* ----------------------
    Setters
    ------------------------ */
    void SetAdvertisedDeviceCallbacks(BLEAdvertisedDeviceCallbacks *);

    /* ----------------------
    Getters
    ------------------------ */
    BLEService *GetCommunicationBLEService()
    {
        return this->pCommunicationService;
    }

    /* uint32_t GetComServiceCharacteristics(ServiceDenomination service)
    {
        switch (service)
        {
        case ServiceDenomination::Tx:
            return this->comsServiceTxCharacteristics;
        case ServiceDenomination::Rx:
            return this->comsServiceRxCharacteristics;
        default:
            return 0;
        }
    } */

    BLEUUID GetCommunicationServiceUUID();
    BLEUUID GetCommunicationCharacteristicTxUUID();
    BLEUUID GetCommunicationCharacteristicRxUUID();

    void connectToDevice();
};