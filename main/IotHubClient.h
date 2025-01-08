#ifndef IOTHUBCLIENT_H
#define IOTHUBCLIENT_H

#include "mqtt_client.h"
#include <string>

#include <az_core.h>
#include <az_iot.h>
#include <mqtt_client.h>

class IotHubClient
{
  public:

    // Properties
    static az_iot_hub_client client;

    // CTOR
    IotHubClient();

    // Functions/Methods
    static void Initialise();
    static char *GetHubClientId();
    static char *GetHubUsername();
    static char *GetHubPassword();
    const char *GetHubUri();
    static bool HasSasTokenExpired();
    static void GenerateSasToken();
    void SubscribeToAzureHandlers(esp_mqtt_client_handle_t mqttClient);
    static void SendDeviceToCloudMessage(esp_mqtt_client_handle_t mqttClient, std::string telemetry_payload);
    static void ReplyDirectMessage(esp_mqtt_client_handle_t mqttClient, int webCode, std::string requestId, std::string responsePayload = "");
    
  private:
  
    static esp_mqtt_client_handle_t _mqttClient;
    
};

extern IotHubClient IotHub;

#endif // IOTHUBCLIENT_H
