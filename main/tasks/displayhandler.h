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
    void SetDisplayName(const char *name);
    void SetModemPowerFlag(bool flagState);
    void SetModemNetFlag(bool flagState);
    void SetModemPppFlag(bool flagState);
    void SetMqttConnectFlag(bool flagState);
    void SetSpareFlag(bool flagState);

private:

    static SSD1306_t DisplayDevice;
    static int TestCount; // TEST Var only. To BE REMOVED.
    static char StatusLine[17];
    static char DeviceName[17];
    
    static bool ModemPower;
    static bool ModemNetwork;
    static bool ModemIpPpp;
    static bool MqttConnect;
    static bool SpareFlag;


    static void TaskInit(void *pvParameters);
    void TaskLoop();
    void InitDisplayDriver();
    void DrawDisplay();
    void UpdateStatusFlags();
    
};

#endif
