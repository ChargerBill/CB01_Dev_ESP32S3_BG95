#include "devicetimehandler.h"
#include "TaskEventBits.h"

void DeviceTimeHandler::Start()
{
  BaseType_t xReturned = xTaskCreate(TaskInit,
  "DeviceTimeHandlerTask",
  8192, // 8k
  NULL, 
  tskIDLE_PRIORITY, 
  NULL);
    
  if (xReturned != pdPASS)
  {
    // Handle task creation failure
    ESP_LOGE("DeviceTimeHandler", "FATAL: Failed to create Device Time Handling Task!, (NO Time Sync Services Available)");
    return;
  }
}

void DeviceTimeHandler::TaskInit(void *pvParameters)
{
  ESP_LOGI("DeviceTimeHandler", "Initialising Task Module");
  DeviceTimeHandler instance;
  
  // This service pauses here until COMMS_AVAILABLE_BIT is set in the "ApplicationEvents" event group
  ESP_LOGI("DeviceTimeHandler", "Waiting for modem comms to be ready...");
  xEventGroupWaitBits(ApplicationEvents, COMMS_AVAILABLE_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
  
  instance.StartTimeSync();
  instance.TaskLoop();  
}

void DeviceTimeHandler::TaskLoop()
{
  ESP_LOGI("DeviceTimeHandler", "Entering Main Task Module Loop");
  
  while (true)
  {
    
    // TODO: read ApplicationEvents group bits, every time a "time sync" check is wanted
    // and if comms still available at that point sync time, otherwise do nothing
    //
    // INFO: This probably only wants to be something like every 12 hours or so, we really
    // don't need to re-sync the time every 5 seconds. Might even be better that we DON'T
    // loop in this task, make it short lived, and have it synced elsewhere that just
    // kicks off the task when a sync is wanted.
    //
    // TO BE DECIDED (For now a pretend loop of 5 seconds shows task is running)
    //  
    //ESP_LOGI("DeviceTimeHandler", "Main Loop Tick. (5 sec)");
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

void DeviceTimeHandler::StartTimeSync()
{
 ESP_LOGI("DeviceTimeHandler", "Initializing Network Time");
 esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
 esp_sntp_setservername(0, "pool.ntp.org");
 sntp_set_time_sync_notification_cb(TimeSyncCallback);
 
 xEventGroupClearBits(ApplicationEvents, DEVICE_TIME_SET_BIT);
 
 esp_sntp_init();
   
}

void DeviceTimeHandler::TimeSyncCallback(struct timeval *tv)
{
  ESP_LOGI("DeviceTimeHandler", "Device Time Successfully Syncronised");
  xEventGroupSetBits(ApplicationEvents, DEVICE_TIME_SET_BIT);
}
