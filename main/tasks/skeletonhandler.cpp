#include "skeletonhandler.h"

void SkeletonHandler::Start()
{
  BaseType_t xReturned = xTaskCreate(TaskInit,
  "SkeletonHandlerTask",
  8192, // 8k
  NULL, 
  tskIDLE_PRIORITY, 
  NULL);
    
  if (xReturned != pdPASS)
  {
    // Handle task creation failure
    ESP_LOGE("SkeletonHandler", "FATAL: Failed to create Skeleton Handling Task!, (NO Skeleton Services Available)");
    return;
  }
}

void SkeletonHandler::TaskInit(void *pvParameters)
{
  ESP_LOGI("SkeletonHandler", "Initialising Task Module");
  SkeletonHandler instance;
    
  instance.TaskLoop();
}

void SkeletonHandler::TaskLoop()
{
  ESP_LOGI("SkeletonHandler", "Entering Main Task Module Loop");
  
  while (true)
  {
    ESP_LOGI("SkeletonHandler", "Main Loop Tick. (30 sec)");
    vTaskDelay(pdMS_TO_TICKS(30000));
  }
}


//#include "event.h"

//BooleanEvent ModemHandler::PppConnectedEvent;
//static Event dataReceivedEvent;

//MqttHandler::dataReceivedEvent.subscribe([&instance]() {
//  instance.updateDisplay(MqttHandler::latestData);
//});

//ModemHandler::PppConnectedEvent.subscribe([&instance](bool pppConnected) {
//  instance.ConnectedEventHandler(pppConnected);
//});

//void MqttHandler::ConnectedEventHandler(bool eventState)
//{
//  if(eventState)
//  {
//    _isModemConnected = true;
//    ESP_LOGI("MqttHandler", "PPP Event Handler Called (PPP Connected)");
//    StartMqtt();
//  }
//  else
//  {
//    _isModemConnected = false;
//    ESP_LOGI("MqttHandler", "PPP Event Handler Called (PPP Disconnected)");
//  }
//}
//PppConnectedEvent.notify(true);
//PppConnectedEvent.notify(false);
