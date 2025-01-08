#ifndef DISPLAYHANDLER_H
#define DISPLAYHANDLER_H

#include <string>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

class DisplayHandler
{
public:
    static void Start();
    
private:
    static void TaskInit(void *pvParameters);
    void TaskLoop();
    
};

#endif
