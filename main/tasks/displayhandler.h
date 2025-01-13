#ifndef DISPLAYHANDLER_H
#define DISPLAYHANDLER_H

#include <string>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "ssd1306.h"

class DisplayHandler
{
public:
    static void Start();
    
private:

    static SSD1306_t DisplayDevice;
    static int TestCount; // TEST Var only. To BE REMOVED.

    static void TaskInit(void *pvParameters);
    void TaskLoop();
    void InitDisplayDriver();
    void DrawDisplay();
    
};

#endif
