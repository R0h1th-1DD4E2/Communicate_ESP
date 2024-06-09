#include <esp_now.h>
#include <WiFi.h>

// Structure for messages
typedef struct message_format {
    char type[10];
    char message[180];
    int status;
    uint8_t target_mac[6];
    bool reset;
} message_format;

esp_now_peer_info_t peerInfo = {};

uint8_t centralMac[6] = {0x34, 0x98, 0x7A, 0xB8, 0x0C, 0xC4};
bool isregister = false;
bool reset = false;

void registerWithCentral() {
    message_format RegData;
    strcpy(RegData.type, "Register");
    strcpy(RegData.message, "ESP32-CAM");
    RegData.status = 0;
    memcpy(peerInfo.peer_addr, centralMac, 6);
    
    if (esp_now_add_peer(&peerInfo) != ESP_OK) 
    {
        Serial.println("Failed to add peer");
        return;
    }
    
    esp_err_t result = esp_now_send(centralMac, (uint8_t *) &RegData, sizeof(RegData));
    if (result == ESP_OK) {
        Serial.println("Registration Message Sent with success");
    } else {
        Serial.println("Error Registering to Central Node");
    }
    esp_now_del_peer(peerInfo.peer_addr);
}

void sendUpdate(const uint8_t *mac_addr) {
    message_format response;
    strcpy(response.type, "Update");
    strcpy(response.message, "Spot 1 Status");
    response.status = 1;
    memcpy(response.target_mac, mac_addr, 6);
    memcpy(peerInfo.peer_addr, mac_addr, 6);
    
    if (esp_now_add_peer(&peerInfo) != ESP_OK) 
    {
        Serial.println("Failed to add peer to update the request");
        return;
    }
    esp_err_t result = esp_now_send(mac_addr, (uint8_t *) &response, sizeof(response));
    if (result == ESP_OK) {
        Serial.println("Update Message Sent with success");
    } 
    else {
        Serial.println("Error Updating to Central Node");
    }
    esp_now_del_peer(peerInfo.peer_addr);
}

void OnDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len) {
    message_format RecvData;
    memcpy(&RecvData, incomingData, sizeof(RecvData));
    reset = RecvData.reset;

    Serial.print("Received message from: ");
    for (int i = 0; i < 6; i++) {
        Serial.print(mac_addr[i], HEX);
        if (i < 5) Serial.print(":");
    }

    printStructMessage(RecvData);

    // Handle acknowledgment
    if (strcmp(RecvData.type, "ack") == 0) 
    {
        Serial.println("Registered with Central Node");
        isregister = true;
    }
    else if (strcmp(RecvData.type, "NoAckReq") == 0) 
    {
        Serial.println("Already Registered with Central Node");
        isregister = true;
    } 
    else if (strcmp(RecvData.type, "Request") == 0) 
    {   
        if (isregister) {
            sendUpdate(mac_addr);
        }
        else {
            Serial.println("Not Registered to send update");
        }
    }
    else if (strcmp(RecvData.type, "Discover") == 0 && (reset || !isregister) )
    {
        registerWithCentral();
    }
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    Serial.print("Send Status: ");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

void printStructMessage(const message_format &msg) {
    Serial.println();
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
    // Example: Send status update
    message_format myData;
    if (Serial.available() > 0) {
        String input = Serial.readStringUntil('\n');
        strcpy(myData.type, "Update");
        input.toCharArray(myData.message, sizeof(myData.message));
        myData.status = 1;
        memcpy(peerInfo.peer_addr, centralMac, 6);
        if (esp_now_add_peer(&peerInfo) != ESP_OK) 
        {
            Serial.println("Failed to add peer to update the request");
            return;
        }
        esp_err_t result = esp_now_send(centralMac, (uint8_t *) &myData, sizeof(myData));
        if (result == ESP_OK) {
            Serial.println("Sent with success");
        } else {
            Serial.println("Error sending the data");
        }
        esp_now_del_peer(peerInfo.peer_addr);
    }

    delay(1000);
}
