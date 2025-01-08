#include <cstddef>
#include <stdio.h>
#include <stdbool.h>
#include <sys/select.h>
#include <unistd.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "tasks/TaskEventBits.h"

#include "tasks/devicetimehandler.h"
#include "tasks/displayhandler.h"
#include "tasks/mqtthandler.h"
#include "tasks/modemhandler.h"

// System TASK Module definitions, all defined in the 'tasks' folder
ModemHandler *AppModem;
DeviceTimeHandler *AppTime;
MqttHandler *AppMqtt;

// Main application wide event group, uses flag definitions in 'TaskEventBits.h'
EventGroupHandle_t ApplicationEvents;

// Main APPLICATION Task entry point, this is the DEFAULT FreeRTOS task created
// by ESP-IDF to kick start the firmware.  All other tasks and modules shall be
// spawned from here.
extern "C" void app_main(void)
{
  ApplicationEvents = xEventGroupCreate();
  
  ESP_LOGI("Main", "APP_MAIN executing.");
  ESP_LOGI("Main", "Free memory: %d bytes", (int)esp_get_free_heap_size());
  ESP_LOGI("Main", "IDF version: %s", esp_get_idf_version());
  
  ESP_LOGI("Main", "Starting ModemHandler Task...");
  AppModem = new ModemHandler();
  AppModem->Start();
  
  ESP_LOGI("Main", "Starting Device Time Task...");
  AppTime = new DeviceTimeHandler();
  AppTime->Start();
  
  ESP_LOGI("Main", "Starting MQTT Task...");
  AppMqtt = new MqttHandler();
  AppMqtt->Start();
  
  //ESP_LOGI("Main", "Starting DisplayHandler Task...");
  //DisplayHandler::StartTask();
  
  //ESP_LOGI("Main", "Starting MqttHandler Task..."); 
  //MqttHandler::StartTask();

  time_t theTime = time(NULL);
  struct tm tm = *localtime(&theTime);
  
  printf("System Date is: %02d/%02d/%04d\n", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);
  printf("System Time is: %02d:%02d:%02d\n", tm.tm_hour, tm.tm_min, tm.tm_sec);

  struct timeval myTime;

  int gtRes = gettimeofday(&myTime, NULL);

  printf("Get time result : %d\n", gtRes);
  
  printf("myTime->tv_sec : %lld\n", myTime.tv_sec);
  printf("myTime->tv_usec : %ld\n", myTime.tv_usec);
  
  uint startTicks = xTaskGetTickCount();
  
  std::string _dummyHb = "{\"tt\":\"heartbeat\", \"mrp\":true, \"is\":false, \"ia\":true, \"hpis\":true, \"mr\":654.321, \"ec\":0, \"ct\":17.00000, \"cp\":0, \"ss\":13, \"otf\":false, \"ocf\":false, \"hm\":false}";
  
  AppMqtt->SendMqttMessageToAzure(_dummyHb);
  
  while (true) 
  {
    //ESP_LOGI("APP_MAIN", "Main loop 1 second tick");
    
    //theTime = time(NULL);
    //tm = *localtime(&theTime);
  
    //printf("System Date is: %02d/%02d/%04d\n", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);
    //printf("System Time is: %02d:%02d:%02d\n", tm.tm_hour, tm.tm_min, tm.tm_sec);
    //printf("Modem is in %d state (IP: %s)\n", AppModem->isConnected, AppModem->ipAddress.c_str());

    uint currentTicks = xTaskGetTickCount();
    if((currentTicks - startTicks) > (6000*10)) // 10 Minutes in tick resolution
    {
      ESP_LOGI("Main","10 minute mark, sending heartbeat...");
      AppMqtt->SendMqttMessageToAzure(_dummyHb);
      startTicks = currentTicks;  
    }
    //printf("CURRENT: %u\r\n", currentTicks);

    vTaskDelay(pdMS_TO_TICKS(500));
  }  
  
}
