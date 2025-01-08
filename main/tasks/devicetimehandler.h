#ifndef DEVICETIMEHANDLER_H
#define DEVICETIMEHANDLER_H

#include <string>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_sntp.h"

#include "TaskEventBits.h"

class DeviceTimeHandler
{
  public:
  
    static void Start();
    
  private:
  
    static void TaskInit(void *pvParameters);
    void TaskLoop();
    
    void StartTimeSync();
    static void TimeSyncCallback(struct timeval *tv);
     
};

#endif /* DEVICETIMEHANDLER_H */
