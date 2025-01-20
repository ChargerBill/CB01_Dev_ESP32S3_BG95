#include <cstddef>
#include <stdio.h>
#include <stdbool.h>
#include <sys/select.h>
#include <unistd.h>

#include "driver/gpio.h"
#include "hal/rtc_io_types.h"
#include "ssd1306.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "hal/gpio_types.h"
#include "tasks/TaskEventBits.h"

#include "tasks/devicetimehandler.h"
#include "tasks/displayhandler.h"
#include "tasks/mqtthandler.h"
#include "tasks/modemhandler.h"

// System TASK Module definitions, all defined in the 'tasks' folder
ModemHandler *AppModem;
DeviceTimeHandler *AppTime;
MqttHandler *AppMqtt;
DisplayHandler *AppDisplay;

// Temp PINS defines, until we build a proper GPIO Task
#define MODEM_PSM GPIO_NUM_41
#define MODEM_NET_STATUS GPIO_NUM_11

// Main application wide event group, uses flag definitions in 'TaskEventBits.h'
EventGroupHandle_t ApplicationEvents;

extern "C" void setup_gpio_defaults()
{
  ESP_LOGI("ApplicationTask", "Setting default GPIO pin states.");
  
  gpio_set_direction(GPIO_NUM_7, GPIO_MODE_OUTPUT);
  gpio_set_level(GPIO_NUM_7,1);
  
  // Modem PSM (input) on Cubix pinout
  gpio_set_direction(MODEM_PSM, GPIO_MODE_INPUT);
  
  // Modem Network Status (input) on Cubix pinout
  gpio_set_direction(MODEM_NET_STATUS, GPIO_MODE_INPUT);
  
  ESP_LOGI("ApplicationTask", "GPIO setup done...");
}

extern "C" void display_timedate()
{
  time_t theTime = time(NULL);
  struct tm tm = *localtime(&theTime);
  struct timeval myTime;
  
  int gtRes = gettimeofday(&myTime, NULL);
  
  printf("System Date is: %02d/%02d/%04d\n", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);
  printf("System Time is: %02d:%02d:%02d\n", tm.tm_hour, tm.tm_min, tm.tm_sec);
  printf("Modem is in %d state (IP: %s)\n", AppModem->isConnected, AppModem->ipAddress.c_str());
}

// Main APPLICATION Task entry point, this is the DEFAULT FreeRTOS task created
// by ESP-IDF to kick start the firmware.  All other tasks and modules shall be
// spawned from here.
extern "C" void app_main(void)
{
  ApplicationEvents = xEventGroupCreate();
  
  ESP_LOGI("ApplicationTask", "APP_MAIN executing.");
  ESP_LOGI("ApplicationTask", "Free memory: %d bytes", (int)esp_get_free_heap_size());
  ESP_LOGI("ApplicationTask", "IDF version: %s", esp_get_idf_version());
  
  setup_gpio_defaults();
  
  ESP_LOGI("ApplicationTask", "Starting ModemHandler Task...");
  AppModem = new ModemHandler();
  AppModem->PowerModemOff();
  AppModem->Start();
  
  ESP_LOGI("ApplicationTask", "Starting Device Time Task...");
  AppTime = new DeviceTimeHandler();
  AppTime->Start();
  
  ESP_LOGI("ApplicationTask", "Starting MQTT Task...");
  AppMqtt = new MqttHandler();
  AppMqtt->Start();
  
  ESP_LOGI("Main", "Starting DisplayHandler Task...");
  AppDisplay = new DisplayHandler();
  AppDisplay->Start();
  
  display_timedate();
  
  uint startTicks = xTaskGetTickCount();
  
  std::string _dummyHb = "{\"tt\":\"heartbeat\", \"mrp\":true, \"is\":false, \"ia\":true, \"hpis\":true, \"mr\":654.321, \"ec\":0, \"ct\":17.00000, \"cp\":0, \"ss\":13, \"otf\":false, \"ocf\":false, \"hm\":false}";
  
  //AppMqtt->SendMqttMessageToAzure(_dummyHb);

  AppDisplay->SetDisplayName(AppMqtt->FetchDeviceName());
    
  // Main application loop/task NEVER ends
  ESP_LOGI("ApplicationTask", "Starting main application task loop.");
  while (true) 
  {
    //ESP_LOGI("APP_MAIN", "Main loop tick");
    
    //display_timedate();
  
    // NOTE FOR COLIN:
    // The following 4 AppDisplay->SetXXXXX calls, will set the first 4 flags on the display.
    // The first one is sampling GPIO 41, which according to the Cubix diagram is the modem power indicator
    // The second is sampling GPIO 11, which according to the Cubix diagram is the modem "network flag"
    // The third one is reading the "PPP Connected" flag that I maintain in the ModemHandler task to tell me if modem is data connected
    // The fourth one is reading the "MQTT Connected" flag that I maintain in the MqttHandler task to tell me when connected to IoT hub
    //
    // the string accross the middle is as follows " MP NW PP MQ ## "
    // MP = Modem Power
    // NW = Modem Network
    // PP = PPP Connection
    // MQ = MQTT Connection
    // ## = Spare
    //
    // When not set, the flag at a given position will show '--' 
  
    // Update Display Status Flags
    if(gpio_get_level(MODEM_PSM)>0) // GPIO Pin 41 (Define at top of this file)
    {
      AppDisplay->SetModemPowerFlag(true);
    }
    else
    {
      AppDisplay->SetModemPowerFlag(false);
    }
    
    if(gpio_get_level(MODEM_NET_STATUS)>0) // GPIO Pin 11 (Define at top of this file)
    {
      AppDisplay->SetModemNetFlag(true);
    }
    else
    {
      AppDisplay->SetModemNetFlag(false);
    }
        
    AppDisplay->SetModemPppFlag(AppModem->isConnected);  // Maintained in "ModemHandler" task
    AppDisplay->SetMqttConnectFlag(AppMqtt->IsConnected()); // Maintained in "MqttHandler" task
    
    // NOTE FOR COLIN:
    // The following line sets or doesn't set the "SPARE" flag on the display.  if this is set to true the
    // display will show '##' in the 5th position on the display, if it is set to false then '--' will be shown.
    // I've set this to true for now, but if the result of a GPIO Pin, or status from elswhere is put in as the
    // parameter, then '##' will appear/disapear on the display depending on the value.
    AppDisplay->SetSpareFlag(true);
    
    uint currentTicks = xTaskGetTickCount();
    if((currentTicks - startTicks) > (6000*10)) // 10 Minutes in tick resolution
    {
      if(AppModem->isConnected)
      {
        ESP_LOGI("ApplicationTask","10 minute mark, sending heartbeat...");
        AppMqtt->SendMqttMessageToAzure(_dummyHb);
        startTicks = currentTicks;
      }
      else
      {
        ESP_LOGI("ApplicationTask", "Modem and/or Mqtt NOT connected cannot send heartbeat!");
        startTicks = currentTicks;
      }  
    }

    vTaskDelay(pdMS_TO_TICKS(500));
  }  
  
}
