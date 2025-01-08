#include "ModemHandler.h"

std::string ModemHandler::ipAddress = "NOT SET";
bool ModemHandler::isConnected = false;

void ModemHandler::Start()
{
  BaseType_t xReturned = xTaskCreate(TaskInit, 
  "ModemHandlerTask", 
  8192,
  NULL, 
  tskIDLE_PRIORITY, 
  NULL);
  
  if (xReturned != pdPASS)
  {
    ESP_LOGE("ModemHandler", "FATAL: Failed to Create ModemHandler Task. (NO Modem Services Available)");
    return;
  }
}

void ModemHandler::TaskInit(void *pvParameters)
{
  ESP_LOGI("ModemHandler", "Initialising Task Module");
  ModemHandler instance;
  
  instance.InitModem();
  instance.TaskLoop();
  
}

void ModemHandler::TaskLoop()
{
  ESP_LOGI("ModemHandler", "Entering Main Task Module Loop");
  
  while (true)
  {
    //ESP_LOGI("ModemHandler", "Main Loop Tick (1 sec)");
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void ModemHandler::InitModem()
{
  ESP_LOGI("ModemHandler", "Initialising Modem");
  
  // System Init
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &ipEvent, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(NETIF_PPP_STATUS, ESP_EVENT_ANY_ID, &pppChangedEvent, NULL));
  
  // DTE Configuration
  dteConfig = ESP_MODEM_DTE_DEFAULT_CONFIG();
  dteConfig.uart_config.tx_io_num = TX_PIN;
  dteConfig.uart_config.rx_io_num = RX_PIN;
  //dteConfig.uart_config.rx_buffer_size = 1024;
  //dteConfig.uart_config.tx_buffer_size = 512;
  //dteConfig.uart_config.cts_io_num = UART_PIN_NO_CHANGE;
  //dteConfig.uart_config.rts_io_num = UART_PIN_NO_CHANGE;
  //dteConfig.uart_config.event_queue_size = 30;
  //dteConfig.task_stack_size = 2048;
  //dteConfig.task_priority = 5;
  //dteConfig.dte_buffer_size = 1024 / 2;
  
  dteConfig.uart_config.baud_rate = 115200;
  //dteConfig.uart_config.data_bits = UART_DATA_8_BITS;
  //dteConfig.uart_config.parity = UART_PARITY_DISABLE;
  //dteConfig.uart_config.stop_bits = UART_STOP_BITS_1;
  //dteConfig.uart_config.flow_control = ESP_MODEM_FLOW_CONTROL_HW;
      
  // DCE Configuration
  dceConfig = ESP_MODEM_DCE_DEFAULT_CONFIG(PPP_APN);
  
  // Netif Configuration
  netifConfig = ESP_NETIF_DEFAULT_PPP();
  pppInterface = esp_netif_new(&netifConfig);
  assert(pppInterface);
  
  // Start DTE device
  auto dte = esp_modem::create_uart_dte(&dteConfig);
  assert(dte);
  
  // Start DCE Device
  dce = esp_modem::create_BG96_dce(&dceConfig, dte, pppInterface);
  assert(dce);
  
  // Configure everything for PPP etc
  if (dteConfig.uart_config.flow_control == ESP_MODEM_FLOW_CONTROL_HW)
  {
    esp_modem::command_result flowRes = dce->set_flow_control(2, 2);
    if (esp_modem::command_result::OK != flowRes)
    {
      if(flowRes == esp_modem::command_result::TIMEOUT)
      {
        ESP_LOGE("ModemHandler", "Time out trying to set HW flow control mode on DCE");  
      }
      else if(flowRes == esp_modem::command_result::FAIL)
      {
        ESP_LOGE("ModemHandler", "Failed trying to set HW flow control mode on DCE");
      }
    }
    ESP_LOGI("ModemHandler", "set_flow_control OK");
  } 
  else 
  {
    ESP_LOGI("ModemHandler", "not set_flow_control, because 2-wire mode active.");
  }
  
  if (dce->set_mode(esp_modem::modem_mode::CMUX_MODE))
  {
    ESP_LOGI("ModemHandler", "Modem has correctly entered multiplexed command/data mode");
  }
  else
  {
    ESP_LOGE("ModemHandler", "Failed to configure multiplexed command mode.");
  }

  //std::string str;
  //esp_modem::command_result opNameRes;
  
  //opNameRes = dce->get_operator_name(str); 
  
  //while (opNameRes != esp_modem::command_result::OK)
  //{
    // Getting operator name could fail... retry after 500 ms
  //  ESP_LOGI("ModemHandler", "Retrying Operator Name, prev result %d", (int)opNameRes);
  //  vTaskDelay(pdMS_TO_TICKS(500));
  //  opNameRes = dce->get_operator_name(str);
  //}
  //ESP_LOGI("ModemHandler", "Operator name: %s", str.c_str());

  // Check if modem is ready
  //if (dce->power_on(dce))
  //{
    // Attach DCE to DTE
  //  dte->set_net_mode(dte, MODEM_NETWORK_MODE);
      
    // Do necessary modem commands to connect to the network
    //dce->set_net_mode(dce, MODEM_NETWORK_MODE_LTE);
      
    //ipAddress = "Connected IP";  // Update with actual logic
  //}
}

void ModemHandler::pppChangedEvent(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  ESP_LOGI("ModemHandler", "PPP state changed event %d", (int)event_id);
}

void ModemHandler::ipEvent(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  ESP_LOGI("ModemHandler", "IP event %d", (int)event_id);
  
  if (event_id == IP_EVENT_PPP_GOT_IP)
  {
    ESP_LOGI("ModemHandler", "PPP Connection has been created.");
    auto *event = (ip_event_got_ip_t *)event_data;
    ESP_LOGI("ModemHandler", "IP          : " IPSTR, IP2STR(&event->ip_info.ip));
    ESP_LOGI("ModemHandler", "Netmask     : " IPSTR, IP2STR(&event->ip_info.netmask));
    ESP_LOGI("ModemHandler", "Gateway     : " IPSTR, IP2STR(&event->ip_info.gw));

    char ipBuffer[25];
    sprintf(ipBuffer, "" IPSTR, IP2STR(&event->ip_info.ip));
    
    ipAddress = std::string(ipBuffer);
    isConnected = true;

    xEventGroupSetBits(ApplicationEvents, COMMS_AVAILABLE_BIT);
  } 
  else if (event_id == IP_EVENT_PPP_LOST_IP)
  {
    ESP_LOGI("ModemHandler", "PPP Connection has been lost.");
    
    ipAddress = "NOT SET";
    isConnected = false;
    
    xEventGroupClearBits(ApplicationEvents, COMMS_AVAILABLE_BIT);
  }
}
