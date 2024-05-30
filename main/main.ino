#include <esp_now.h>
#include <WiFi.h>
#include "soc/soc.h"           // Brownout detector
#include "soc/rtc_cntl_reg.h"  // Brownout detector

typedef struct struct_message 
{
  char text[250];
} struct_message;

struct_message myData;
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) 
{
  Serial.print("Last Packet Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) 
{
    struct_message receivedData;
    memcpy(&receivedData, incomingData, sizeof(receivedData));
    Serial.print("Received from ");
    for (int i = 0; i < 6; i++) 
    {
        Serial.print(mac[i], HEX);
        if (i < 5) Serial.print(":");
    }
    Serial.print(" - ");
    Serial.println(receivedData.text);
}

void setup() 
{
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // Disable brownout detector
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    
    if (esp_now_init() != ESP_OK) 
    {
        Serial.println("Error initializing ESP-NOW");
        return;
    }
    
    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(OnDataRecv);
    
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    
    if (esp_now_add_peer(&peerInfo) != ESP_OK) 
    {
        Serial.println("Failed to add peer");
        return;
    }
}

void loop() 
{
    if (Serial.available() > 0) 
    {
        String input = Serial.readStringUntil('\n');
        input.toCharArray(myData.text, 250);
        
        esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));
        if (result == ESP_OK) 
        {
        Serial.println("Sent with success");
        } 
        else 
        {
        Serial.println("Error sending the data");
        }
    }
}
