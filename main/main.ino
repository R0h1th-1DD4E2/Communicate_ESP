#include <esp_now.h>
#include <WiFi.h>
#include <WebSocketsServer.h>

// Structure for messages
typedef struct message_format {
    char type[10];
    char message[180];
    int status;
    uint8_t target_mac[6];  // Target MAC address
    bool reset;
} message_format;

esp_now_peer_info_t peerInfo = {};
WebSocketsServer webSocket = WebSocketsServer(8080);

#define CHANNEL 0
#define MAX_PEERS 20

uint8_t peerAddresses[MAX_PEERS][6];
uint8_t clientID = 0;

int peerCount = 0;
uint ArrMesCount = 0;

unsigned long lastDiscoveryTime = 0;
const unsigned long discoveryInterval = 10000;

bool reset = true;
bool switchCon = false;
bool initzwebsock = false;

message_format messageToStore[20];

//Wifi
const char* ssid = "81NE-S010404 4362";
const char* password = "9054Nw&0";

void discoverSubNodes() {
    message_format discoveryMessage;
    strcpy(discoveryMessage.type, "Discover");
    strcpy(discoveryMessage.message, "Central: Hey! Anyone Here?");
    discoveryMessage.reset = reset;
    discoveryMessage.status = 0;

    // Broadcast discovery message
    uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    memcpy(discoveryMessage.target_mac, broadcastAddress, 6);
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    
    if (esp_now_add_peer(&peerInfo) != ESP_OK) 
    {
        Serial.println("Failed to add peer");
        return;
    }
    
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&discoveryMessage, sizeof(discoveryMessage));
    if (result == ESP_OK) {
        // printStructMessage(discoveryMessage);
        addMessageToArray(discoveryMessage);
        Serial.println("Discover Message Sent with success");
    } else {
        Serial.println("Error Broadcasting Discover Message");
        addErrorToArray("Error Broadcasting Discover Message");
        return;
    }
    esp_now_del_peer(peerInfo.peer_addr);
}

void disconnectSubNodes() {
    message_format disconnectMessage;
    strcpy(disconnectMessage.type, "Discon");
    strcpy(disconnectMessage.message, "I'm Disconnecting ");
    disconnectMessage.reset = reset;
    disconnectMessage.status = 0;

    for (int i = 0; i < peerCount; i++) {
        memcpy(myData.target_mac, peerAddresses[i], 6);
        memcpy(peerInfo.peer_addr, peerAddresses[i], 6);
    
        if (esp_now_add_peer(&peerInfo) != ESP_OK) 
        {
            Serial.println("Failed to add peer");
            return;
        }
        
        esp_err_t result = esp_now_send(peerAddresses[i], (uint8_t *)&disconnectMessage, sizeof(disconnectMessage));
        if (result == ESP_OK) {
            // printStructMessage(discoveryMessage);
            addMessageToArray(disconnectMessage);
            Serial.println("Disconnect Message Sent with success");
        } else {
            Serial.println("Error Broadcasting Disconnect Message");
            addErrorToArray("Error Broadcasting Disconnect Message");
            return;
        }
        esp_now_del_peer(peerInfo.peer_addr);
    }

    // Deinit esp now
    esp_now_deinit();
}

void reconnectSubNodes() {
    message_format reconnectMessage;
    strcpy(reconnectMessage.type, "Recon");
    strcpy(reconnectMessage.message, "I'm Online !");
    reconnectMessage.reset = reset;
    reconnectMessage.status = 0;

    for (int i = 0; i < peerCount; i++) {
        memcpy(myData.target_mac, peerAddresses[i], 6);
        memcpy(peerInfo.peer_addr, peerAddresses[i], 6);
    
        if (esp_now_add_peer(&peerInfo) != ESP_OK) 
        {
            Serial.println("Failed to add peer");
            return;
        }
        
        esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&reconnectMessage, sizeof(reconnectMessage));
        if (result == ESP_OK) {
            // printStructMessage(discoveryMessage);
            addMessageToArray(reconnectMessage);
            Serial.println("Reconnect Message Sent with success");
        } else {
            Serial.println("Error Broadcasting Reconnect Message");
            addErrorToArray("Error Broadcasting Reconnect Message");
            return;
        }
        esp_now_del_peer(peerInfo.peer_addr);
    }
}

void addMessageToArray(const message_format &message) {
    if (ArrMesCount < 20){
        strcpy(messageToStore[ArrMesCount].type, message.type);
        strcpy(messageToStore[ArrMesCount].message, message.message);
        memcpy(messageToStore[ArrMesCount].target_mac, message.target_mac,6);
        messageToStore[ArrMesCount].reset = reset;
        messageToStore[ArrMesCount].status = 0;
        ArrMesCount++;
    }
    else {
        switchCon = true;
    }
}

void addErrorToArray(String error_msg) {
    if (ArrMesCount < 20){
        message_format msg;
        strcpy(messageToStore[ArrMesCount].type, "Error");
        strcpy(messageToStore[ArrMesCount].message, error_msg.c_str());
        messageToStore[ArrMesCount].reset = false;
        messageToStore[ArrMesCount].status = 0;
        ArrMesCount++;
    }
    else {
        switchCon = true;
    }
}

String serializeMessages() {
    String jsonString = "{";
    for (int i = 0; i < 20; i++) {
        jsonString += "{";
        jsonString += "\"type\":\"" + String(messageToStore[i].type) + "\",";
        jsonString += "\"message\":\"" + String(messageToStore[i].message) + "\",";
        jsonString += "\"status\":" + String(messageToStore[i].status) + ",";
        jsonString += "\"target_mac\":[";
        for (int j = 0; j < 6; j++) {
            jsonString += String(messageToStore[i].target_mac[j]);
            if (j < 5) {
                jsonString += ",";
            }
        }
        jsonString += "],";
        jsonString += "\"reset\":" + String(messageToStore[i].reset ? "true" : "false");
        jsonString += "}";
        if (i < 19) {
            jsonString += ",";
        }
    }
    jsonString += "}";
    return jsonString;
}

void sendDeviceToSocket(uint8_t clientID) {
    String jsonString = "{";
    String macaddr = "[";
    for (int i = 0; i < peerCount; i++) {
        char macStr[18];
        sprintf(macStr, "\"%02X:%02X:%02X:%02X:%02X:%02X\"",
                peerAddresses[i][0], peerAddresses[i][1], peerAddresses[i][2],
                peerAddresses[i][3], peerAddresses[i][4], peerAddresses[i][5]);

        macaddr += macStr;
        if (i < peerCount - 1) {
            macaddr += ",";
        }
    }
    macaddr += "]";
    jsonString += "{";
        jsonString += "\"type\":\"" + "Device" + "\",";
        jsonString += "\"message\":\"" + macaddr + "\",";
        jsonString += "\"status\":" + "0" + ",";
        jsonString += "\"reset\":" + reset + ",";
        jsonString += "\"target_mac\":[FF:FF:FF:FF:FF:FF]";
    jsonString += "}}";

    webSocket.sendTXT(clientID, jsonString);
}

void sendMessageToSocket(uint8_t clientID) {
    String jsongString = serializeMessages();
    webSocket.sendTXT(clientID, jsonString);
}

void OnDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len) {
    message_format RecvData;
    memcpy(&RecvData, incomingData, sizeof(RecvData));
    Serial.print("Received message from: ");
    for (int i = 0; i < 6; i++) {
        Serial.print(mac_addr[i], HEX);
        if (i < 5) Serial.print(":");
    }
    printStructMessage(RecvData);
    addMessageToArray(RecvData);

    // Handle registration
    if (strcmp(RecvData.type, "Register") == 0) {
        bool alreadyRegistered = false;
        for (int i = 0; i < peerCount; i++) {
            if (memcmp(peerAddresses[i], mac_addr, 6) == 0) {
                alreadyRegistered = true;
                message_format ackMessage;
                strcpy(ackMessage.type, "NoAckReq");
                strcpy(ackMessage.message, "Already Registered");
                ackMessage.status = 1;
                ackMessage.reset = reset;

                memcpy(peerInfo.peer_addr, mac_addr, 6);
                if (esp_now_add_peer(&peerInfo) != ESP_OK) 
                {
                    Serial.println("Failed to add peer (reg)");
                    return;
                }
                esp_err_t result = esp_now_send(mac_addr, (uint8_t *) &ackMessage, sizeof(ackMessage));
                if (result == ESP_OK) {
                    addMessageToArray(ackMessage);
                    Serial.println("Acknowlegde Sent with success");
                    reset = false;
                } 
                else {
                    addErrorToArray("Error sending the Acknowlege Message");
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
            message_format ackMessage;
            strcpy(ackMessage.type, "ack");
            strcpy(ackMessage.message, "Registered");
            ackMessage.status = 1;
            ackMessage.reset = reset;

            memcpy(peerInfo.peer_addr, mac_addr, 6);
            if (esp_now_add_peer(&peerInfo) != ESP_OK) 
            {
                Serial.println("Failed to add peer (reg)");
                return;
            }
            esp_err_t result = esp_now_send(mac_addr, (uint8_t *) &ackMessage, sizeof(ackMessage));
            if (result == ESP_OK) {
                addMessageToArray(ackMessage);
                Serial.println("Acknowlegde Sent with success");
                reset = false;
            } 
            else {
                addErrorToArray("Error sending the Acknowlege Message");
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

void printStructMessage(const message_format &msg) {
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

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.println("Disconnected from WebSocket server");
            break;
        case WStype_CONNECTED:
            Serial.println("Connected to WebSocket server");
            break;
        case WStype_TEXT:
            Serial.printf("Received: %s\n", payload);
            clientID = num;
            break;~
    }
}

void initWebsocket() {
    // Attempt to connect to WiFi
    WiFi.begin(ssid, password);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 5) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
        attempts++;
    }
    // Check if connected or maximum attempts reached
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Connected to WiFi");
        // Start WebSocket server
        webSocket.begin();
        webSocket.onEvent(webSocketEvent);

        Serial.println("Websocket Started ");
    } else {
        Serial.println("Failed to connect to WiFi");
    }
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

    if (!switchCon) {
        // Connect New ESP
        unsigned long currentMillis = millis();
        if (currentMillis - lastDiscoveryTime >= discoveryInterval) {
            discoverSubNodes();
            lastDiscoveryTime = currentMillis;
        }

        // Test only 
        message_format myData;
        if (Serial.available() > 0) 
        {
            strcpy(myData.type, "Request");
            String input = Serial.readStringUntil('\n');
            input.toCharArray(myData.message, sizeof(myData.message));
            myData.status = 0;
            myData.reset = false;

            for (int i = 0; i < peerCount; i++) {
                memcpy(myData.target_mac, peerAddresses[i], 6);
                memcpy(peerInfo.peer_addr, peerAddresses[i], 6);
        
                if (esp_now_add_peer(&peerInfo) != ESP_OK) 
                {
                    Serial.println("Failed to add peer");
                    return;
                }

                memcpy(myData.target_mac, peerAddresses[i], 6);
                esp_err_t result = esp_now_send(peerAddresses[i], (uint8_t *) &myData, sizeof(myData));
                if (result == ESP_OK) {
                    addMessageToArray(myData);
                    Serial.println("Sent with success");
                } else {
                    addErrorToArray("Error sending the request data");
                    Serial.println("Error sending the request data");
                }
                esp_now_del_peer(peerInfo.peer_addr);
            }
        }
    }
    else {

        // Here some function

        if (!initzwebsock) {
            // before Disconnecting esp send a message
            disconnectSubNodes();

            // Start Websocket
            initWebsocket();
            initzwebsock = true;
        }

        webSocket.loop();
        // Send all the messages
        // here some function 
        sendDeviceToSocket(clientID);
        sendMessageToSocket(clientID);

        // Once Done sending info disconnect
        WiFi.disconnect();
    } 
    
    delay(1000);

}

