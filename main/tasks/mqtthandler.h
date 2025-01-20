#ifndef MQTTHANDLER_H
#define MQTTHANDLER_H

#include <string>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "esp_event.h"

#include "../IotHubClient.h"

class MqttHandler
{
  public:
  
    static void Start();
    void SendMqttMessageToAzure(std::string messageText);
    const char *FetchDeviceName();
    static bool IsConnected();

private:

    static bool _hasmadewebcall;
    static bool _isModemConnected;
    static bool _isMqttConnected;
    static char *IoTHubDeviceName;
    static esp_mqtt_client_handle_t _client;
    static IotHubClient _iotClient;
    static bool _isSubscribed;

    static void TaskInit(void *pvParameters);
    void TaskLoop();
    
    void ConnectedEventHandler(bool eventState);
    static void EventHandler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
    void StartMqtt();
    void SubscribeToNotifications();
};

#endif
