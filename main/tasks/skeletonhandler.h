#ifndef SKELETONHANDLER_H
#define SKELETONHANDLER_H

#include <string>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

class SkeletonHandler
{
  public:
    static void Start();
    
  private:
    static void TaskInit(void *pvParameters);
    void TaskLoop();
};

#endif /* SKELETONHANDLER_H */
