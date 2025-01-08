#include "displayhandler.h"

void DisplayHandler::Start()
{
  BaseType_t xReturned = xTaskCreate(TaskInit,
  "DisplayHandlerTask",
  8192, // 8k
  NULL, 
  tskIDLE_PRIORITY, 
  NULL);
    
  if (xReturned != pdPASS)
  {
    // Handle task creation failure
    ESP_LOGE("DisplayHandler", "FATAL: Failed to create Display Handling Task!, (NO Display Services Available)");
    return;
  }
}

void DisplayHandler::TaskInit(void *pvParameters)
{
  ESP_LOGI("DisplayHandler", "Initialising Task Module");
  DisplayHandler instance;
    
  instance.TaskLoop();
}

void DisplayHandler::TaskLoop()
{
  ESP_LOGI("DisplayHandler", "Entering Main Task Module Loop");
  
  while (true)
  {
    //ESP_LOGI("DisplayHandler", "Main Loop Tick. (0.25 sec)");
    vTaskDelay(pdMS_TO_TICKS(250));
  }
}
