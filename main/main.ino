#include <esp_now.h>
#include <WiFi.h>

// Structure for messages
typedef struct struct_message {
    char type[10];
    char message[32];
    int status;
    uint8_t target_mac[6];  // Target MAC address
    bool reset;
} struct_message;

esp_now_peer_info_t peerInfo = {};

#define CHANNEL 0
#define MAX_PEERS 20
uint8_t peerAddresses[MAX_PEERS][6];
int peerCount = 0;
bool reset = true;
unsigned long lastDiscoveryTime = 0;
const unsigned long discoveryInterval = 10000;

void discoverSubNodes() {
    struct_message discoveryMessage;
    strcpy(discoveryMessage.type, "Discover");
    strcpy(discoveryMessage.message, "Central: Hey! Anyone Here?");
    discoveryMessage.reset = reset;
    discoveryMessage.status = 0;

    // Step 1: Broadcast discovery message
    uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    
    if (esp_now_add_peer(&peerInfo) != ESP_OK) 
    {
        Serial.println("Failed to add peer");
        return;
    }
    
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&discoveryMessage, sizeof(discoveryMessage));
    if (result == ESP_OK) {
        Serial.println("Discover Message Sent with success");
    } else {
        Serial.println("Error Broadcasting Discover Message");
        return;
    }
    esp_now_del_peer(peerInfo.peer_addr);
}


void OnDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len) {
    struct_message RecvData;
    memcpy(&RecvData, incomingData, sizeof(RecvData));
    Serial.print("Received message from: ");
    for (int i = 0; i < 6; i++) {
        Serial.print(mac_addr[i], HEX);
        if (i < 5) Serial.print(":");
    }

    Serial.println();

    Serial.print("Type: ");
    Serial.println(RecvData.type);
    Serial.print("Message: ");
    Serial.println(RecvData.message);
    Serial.print("Status: ");
    Serial.println(RecvData.status);

    // Handle registration
    if (strcmp(RecvData.type, "Register") == 0) {
        bool alreadyRegistered = false;
        for (int i = 0; i < peerCount; i++) {
            if (memcmp(peerAddresses[i], mac_addr, 6) == 0) {
                alreadyRegistered = true;
                struct_message ackMessage;
                strcpy(ackMessage.type, "NoAckReq");
                strcpy(ackMessage.message, "Already Registered");
                ackMessage.status = 1;

                memcpy(peerInfo.peer_addr, mac_addr, 6);
                if (esp_now_add_peer(&peerInfo) != ESP_OK) 
                {
                    Serial.println("Failed to add peer (reg)");
                    return;
                }
                esp_err_t result = esp_now_send(mac_addr, (uint8_t *) &ackMessage, sizeof(ackMessage));
                if (result == ESP_OK) {
                    Serial.println("Acknowlegde Sent with success");
                    reset = false;
                } 
                else {
                    Serial.println("Error sending the Acknowlege Message");
                }
                esp_now_del_peer(peerInfo.peer_addr);
                break;
            }
        }
        if (!alreadyRegistered && peerCount < MAX_PEERS) {
            memcpy(peerAddresses[peerCount], mac_addr, 6);
            peerCount++;
            Serial.println("New device registered");

            // Send acknowledgment
            struct_message ackMessage;
            strcpy(ackMessage.type, "ack");
            strcpy(ackMessage.message, "Registered");
            ackMessage.status = 1;
            memcpy(peerInfo.peer_addr, mac_addr, 6);
            if (esp_now_add_peer(&peerInfo) != ESP_OK) 
            {
                Serial.println("Failed to add peer (reg)");
                return;
            }
            esp_err_t result = esp_now_send(mac_addr, (uint8_t *) &ackMessage, sizeof(ackMessage));
            if (result == ESP_OK) {
                Serial.println("Acknowlegde Sent with success");
                reset = false;
            } 
            else {
                Serial.println("Error sending the Acknowlege Message");
            }
            esp_now_del_peer(peerInfo.peer_addr);
        }
    } 
    else if (strcmp(RecvData.type, "Update") == 0) {
        Serial.println("Status update received");
    }
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    Serial.print("Send Status: ");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

void printStructMessage(const struct_message &msg) {
    Serial.print("Type: ");
    Serial.println(msg.type);
    Serial.print("Message: ");
    Serial.println(msg.message);
    Serial.print("Status: ");
    Serial.println(msg.status);
    Serial.print("Reset: ");
    Serial.println(msg.reset);
    Serial.print("Target MAC Address: ");
    for (int i = 0; i < 6; i++) {
        Serial.print(msg.target_mac[i], HEX);
        if (i < 5) Serial.print(":");
    }
    Serial.println();
}

void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }

    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    esp_now_register_recv_cb(OnDataRecv);
    esp_now_register_send_cb(OnDataSent);
}

void loop() {
    unsigned long currentMillis = millis();
    if (currentMillis - lastDiscoveryTime >= discoveryInterval) {
        discoverSubNodes();
        lastDiscoveryTime = currentMillis;
    }

    struct_message myData;
    if (Serial.available() > 0) {
        strcpy(myData.type, "Request");
        String input = Serial.readStringUntil('\n');
        input.toCharArray(myData.message, sizeof(myData.message));
        myData.status = 0;
        myData.reset = false;

        for (int i = 0; i < peerCount; i++) {
            memcpy(peerInfo.peer_addr, peerAddresses[i], 6);
    
            if (esp_now_add_peer(&peerInfo) != ESP_OK) 
            {
                Serial.println("Failed to add peer");
                return;
            }
            memcpy(myData.target_mac, peerAddresses[i], 6);
            esp_err_t result = esp_now_send(peerAddresses[i], (uint8_t *) &myData, sizeof(myData));
            if (result == ESP_OK) {
                Serial.println(sizeof(myData));
                Serial.println("Sent with success");
            } else {
                Serial.println("Error sending the data");
            }
            esp_now_del_peer(peerInfo.peer_addr);
        }
    }
    delay(1000);
}

