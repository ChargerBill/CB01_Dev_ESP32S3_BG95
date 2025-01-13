#include <cstddef>
#include <stdio.h>
#include <stdbool.h>
#include <sys/select.h>
#include <unistd.h>

#include "driver/gpio.h"
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

// Main application wide event group, uses flag definitions in 'TaskEventBits.h'
EventGroupHandle_t ApplicationEvents;

extern "C" void setup_gpio_defaults()
{
  ESP_LOGI("ApplicationTask", "Setting default GPIO pin states.");
  
  gpio_set_direction(GPIO_NUM_7, GPIO_MODE_OUTPUT);
  gpio_set_level(GPIO_NUM_7,1);
  
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

//#define CONFIG_INTERFACE FART

//#define CONFIG_SDA_GPIO 15
//#define CONFIG_SCL_GPIO 16

//#define CONFIG_RESET_GPIO 0
//#define CONFIG_MODEL TWAT

//#define I2C_DISPLAY_ADDRESS 0x3C         /* Display I2C address */

extern "C" void testI2C()
{
  // quick test before doing any driver code, to test that the display can be
  // found at 0x3C and that the I2C system is working
  ESP_LOGI("ApplicationTask", "Testing I2C for display");
  
  SSD1306_t dev;
  int center, top, bottom;
  char lineChar[20];
  
  ESP_LOGI("ApplicationTask", "INTERFACE is i2c");
  ESP_LOGI("ApplicationTask", "CONFIG_SDA_GPIO=%d",CONFIG_SDA_GPIO);
  ESP_LOGI("ApplicationTask", "CONFIG_SCL_GPIO=%d",CONFIG_SCL_GPIO);
  ESP_LOGI("ApplicationTask", "CONFIG_RESET_GPIO=%d",CONFIG_RESET_GPIO);
  
  i2c_master_init(&dev, CONFIG_SDA_GPIO, CONFIG_SCL_GPIO, CONFIG_RESET_GPIO);
  
  ESP_LOGI("ApplicationTask", "Panel is 128x64");
  ssd1306_init(&dev, 128, 64);
  
  ssd1306_clear_screen(&dev, false);
  ssd1306_contrast(&dev, 0xff);
  ssd1306_display_text_x3(&dev, 0, "Hello", 5, false);

  
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
  
  testI2C();
  
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
  
  //ESP_LOGI("Main", "Starting DisplayHandler Task...");
  //DisplayHandler::StartTask();
  
  display_timedate();
  
  uint startTicks = xTaskGetTickCount();
  
  std::string _dummyHb = "{\"tt\":\"heartbeat\", \"mrp\":true, \"is\":false, \"ia\":true, \"hpis\":true, \"mr\":654.321, \"ec\":0, \"ct\":17.00000, \"cp\":0, \"ss\":13, \"otf\":false, \"ocf\":false, \"hm\":false}";
  
  //AppMqtt->SendMqttMessageToAzure(_dummyHb);
    
  // Main application loop/task NEVER ends
  ESP_LOGI("ApplicationTask", "Starting main application task loop.");
  while (true) 
  {
    //ESP_LOGI("APP_MAIN", "Main loop 1 second tick");
    
    //display_timedate();
    
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
