#include "displayhandler.h"
#include <cstdio>
#include <cstring>

SSD1306_t DisplayHandler::DisplayDevice;

int DisplayHandler::TestCount;
char DisplayHandler::StatusLine[17];
char DisplayHandler::DeviceName[17];

bool DisplayHandler::ModemPower = false;
bool DisplayHandler::ModemNetwork = false;
bool DisplayHandler::ModemIpPpp = false;
bool DisplayHandler::MqttConnect = false;
bool DisplayHandler::SpareFlag = false;

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
  sprintf(StatusLine, " -- -- -- -- -- ");
  //sprintf(DeviceName, "NAME NOT YET SET"); -- Do not enable, stops "set display name working" (PJS)
    
  instance.TaskLoop();
}

void DisplayHandler::TaskLoop()
{
  ESP_LOGI("DisplayHandler", "Entering Main Task Module Loop");
  
  while (true)
  {
    //ESP_LOGI("DisplayHandler", "Main Loop Tick.");
    
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
  
  char buf[6];
  
  //ssd1306_clear_screen(&DisplayDevice, false);

  sprintf(buf, "%05d", TestCount);

  ssd1306_display_text(&DisplayDevice, 0, "Charger Bill", 12, false);
  ssd1306_display_text_x3(&DisplayDevice, 1, buf, 5, false);

  UpdateStatusFlags();
  ssd1306_display_text(&DisplayDevice,5,StatusLine,20,false);
  
  ssd1306_display_text(&DisplayDevice,7,DeviceName,20,false);

}

void DisplayHandler::UpdateStatusFlags()
{
  if(ModemPower)
  {
    StatusLine[1] = 'M';
    StatusLine[2] = 'P';
  }
  else
  {
    StatusLine[1] = '-';
    StatusLine[2] = '-';    
  }

  if(ModemNetwork)
  {
    StatusLine[4] = 'N';
    StatusLine[5] = 'W';
  }
  else
  {
    StatusLine[4] = '-';
    StatusLine[5] = '-';    
  }

  if(ModemIpPpp)
  {
    StatusLine[7] = 'P';
    StatusLine[8] = 'P';
  }
  else
  {
    StatusLine[7] = '-';
    StatusLine[8] = '-';    
  }

  if(MqttConnect)
  {
    StatusLine[10] = 'M';
    StatusLine[11] = 'Q';
  }
  else
  {
    StatusLine[10] = '-';
    StatusLine[11] = '-';    
  }

  if(SpareFlag)
  {
    StatusLine[13] = '#';
    StatusLine[14] = '#';
  }
  else
  {
    StatusLine[13] = '-';
    StatusLine[14] = '-';    
  }

}

// Called from other tasks to set the device name to be displayed.
void DisplayHandler::SetDisplayName(const char *name)
{
  ESP_LOGI("DisplayHandler", "Displayname being set to : %s", name);
  
  memset(DeviceName, 0, 16);
  strncpy(DeviceName, name, 16);
}

void DisplayHandler::SetModemPowerFlag(bool flagState)
{
  ModemPower = flagState;
}

void DisplayHandler::SetModemNetFlag(bool flagState)
{
  ModemNetwork = flagState;
}

void DisplayHandler::SetModemPppFlag(bool flagState)
{
  ModemIpPpp = flagState;
}

void DisplayHandler::SetMqttConnectFlag(bool flagState)
{
  MqttConnect = flagState;
}

void DisplayHandler::SetSpareFlag(bool flagState)
{
  SpareFlag = flagState;
}
