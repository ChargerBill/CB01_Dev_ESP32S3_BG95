#include "displayhandler.h"
#include <cstdio>

SSD1306_t DisplayHandler::DisplayDevice;

int DisplayHandler::TestCount;

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

  instance.InitDisplayDriver();
  instance.TestCount = 0;
    
  instance.TaskLoop();
}

void DisplayHandler::TaskLoop()
{
  ESP_LOGI("DisplayHandler", "Entering Main Task Module Loop");
  
  while (true)
  {
    //ESP_LOGI("DisplayHandler", "Main Loop Tick. (0.25 sec)");
    DrawDisplay();
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    TestCount++;
  }
}

void DisplayHandler::InitDisplayDriver()
{
  
  ESP_LOGI("DisplayHandler", "Initialising OLED display panel.");
  ESP_LOGI("DisplayHandler", "CONFIG_SDA_GPIO=%d",CONFIG_SDA_GPIO);
  ESP_LOGI("DisplayHandler", "CONFIG_SCL_GPIO=%d",CONFIG_SCL_GPIO);
  //ESP_LOGI("DisplayHandler", "CONFIG_RESET_GPIO=%d",CONFIG_RESET_GPIO);
  
  i2c_master_init(&DisplayDevice, CONFIG_SDA_GPIO, CONFIG_SCL_GPIO, CONFIG_RESET_GPIO);
  
  ESP_LOGI("DisplayHandler", "Panel is 128x64");
  ssd1306_init(&DisplayDevice, 128, 64);

  ssd1306_clear_screen(&DisplayDevice, false);
  ssd1306_contrast(&DisplayDevice, 0xff);

}

void DisplayHandler::DrawDisplay()
{
  
  char buf[20];
  
  //ssd1306_clear_screen(&DisplayDevice, false);

  sprintf(buf, "%02d", TestCount);

  ssd1306_display_text(&DisplayDevice, 0, "Charger Bill", 12, false);
  ssd1306_display_text_x3(&DisplayDevice, 1, buf, 5, false);

}
