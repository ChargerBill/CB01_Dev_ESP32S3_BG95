#include "ModemHandler.h"
#include "portmacro.h"

std::string ModemHandler::ipAddress = "NOT SET";
bool ModemHandler::isPoweredOn = false;
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
  
  if(instance.isPoweredOn) // If the modem is already ON, switch it off first
  {
    instance.PowerModemOff();
  }
  
  instance.PowerModemOn();
  instance.InitModem();
  instance.TaskLoop();
  
}

void ModemHandler::TaskLoop()
{
  ESP_LOGI("ModemHandler", "Entering Main Task Module Loop");
  
  while (true)
  {
    ESP_LOGI("ModemHandler", "Main Loop Tick (10 sec)");
    
    // Check the modem status
    CheckModemStatus();
    
    vTaskDelay(pdMS_TO_TICKS(10000));
  }
}


// This is a wierd way of doing this, as we (As of yet anyway) don't have any hardware
// way of knowing if the modem is actually powered on or not, the dev boards we have for
// the BG95 don't have a pin/terminal ont hem that we can monitor to know the actual modem
// state.  The cubix boards however, I believe do, so for now we going to have to rely on
// tracking the power state ourselves in software, just to make sure that we are/are not 
// switching the modem off/on when we think we are.  
void ModemHandler::PowerModemOn()
{
  if(!isPoweredOn)
  {
    ESP_LOGI("ModemHandler", "Attempting to switch modem on.");
    PulseModemPowerKey();
    isPoweredOn = true;
  }
}

void ModemHandler::PowerModemOff()
{
  if(isPoweredOn)
  {
    ESP_LOGI("ModemHandler", "Attempting to switch modem off.");
    PulseModemPowerKey();
    isPoweredOn = false;
  }  
}

void ModemHandler::PulseModemPowerKey()
{
  //gpio_set_direction(GPIO_NUM_7, GPIO_MODE_OUTPUT); // Should have been set at beginning of Application Task

  ESP_LOGI("ModemHandler", "Pulling Modem Reset Line LOW for 1 second");
  gpio_set_level(GPIO_NUM_7,0);
  
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  
  ESP_LOGI("ModemHandler", "Returning Modem Reset Line HIGH and waiting for 20 seconds for modem to stabalise.");
  gpio_set_level(GPIO_NUM_7,1);
  
  vTaskDelay(20000 / portTICK_PERIOD_MS);
  
  ESP_LOGI("ModemHandler", "Modem hard reset done...");
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
    dteConfig.uart_config.baud_rate = 115200;
    //dteConfig.uart_config.rx_buffer_size = 1024;
    //dteConfig.uart_config.tx_buffer_size = 512;
    //dteConfig.uart_config.cts_io_num = UART_PIN_NO_CHANGE;
    //dteConfig.uart_config.rts_io_num = UART_PIN_NO_CHANGE;
    //dteConfig.uart_config.event_queue_size = 30;
    //dteConfig.task_stack_size = 2048;
    //dteConfig.task_priority = 5;
    //dteConfig.dte_buffer_size = 1024 / 2;
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
    dte = esp_modem::create_uart_dte(&dteConfig); // Assign to the class member
    assert(dte);
    
    // Start DCE Device
    dce = esp_modem::create_BG96_dce(&dceConfig, dte, pppInterface); // Pass shared_ptr directly
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
        //ESP_LOGI("ModemHandler", "Retrying Operator Name, prev result %d", (int)opNameRes);
        //vTaskDelay(pdMS_TO_TICKS(500));
        //opNameRes = dce->get_operator_name(str);
    //}
    //ESP_LOGI("ModemHandler", "Operator name: %s", str.c_str());

    // Check if modem is ready
    //if (dce->power_on(dce))
    //{
        // Attach DCE to DTE
        //dte->set_net_mode(dte, MODEM_NETWORK_MODE);
        
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

void ModemHandler::LogGPIOStatus()
{
    int apReadyStatus = gpio_get_level(MODEM_AP_READY_SIGNAL_PIN);
    int networkStatus = gpio_get_level(MODEM_NETWORK_STATUS_PIN);

    ESP_LOGI("ModemHandler", "Modem AP ready Signal (GPIO04): %d", apReadyStatus);
    ESP_LOGI("ModemHandler", "Modem Network Status (GPIO11): %d", networkStatus);

    // Additional logging for debugging
    if (apReadyStatus == 0)
    {
        ESP_LOGI("ModemHandler", "Modem AP is not ready.");
    }
    else
    {
        ESP_LOGI("ModemHandler", "Modem AP is ready.");
    }

    if (networkStatus == 0)
    {
        ESP_LOGI("ModemHandler", "Modem is not connected to the network.");
    }
    else
    {
        ESP_LOGI("ModemHandler", "Modem is connected to the network.");
    }
}

void ModemHandler::CheckModemStatus()
{
    // Initial AT command to verify communication
    SendATCommand("AT");
    std::string atResponse = ReadATResponse();
    ESP_LOGI("ModemHandler", "Initial AT Response: %s", atResponse.c_str());

    if (atResponse.find("OK") == std::string::npos)
    {
        ESP_LOGE("ModemHandler", "Failed to communicate with modem. Aborting status check.");
        return;
    }

    // Send AT command to set APN
    SendATCommand("AT+CGDCONT=1,\"IP\",\"quectel.tn.std\"");
    vTaskDelay(pdMS_TO_TICKS(1000));

    // Check network registration
    SendATCommand("AT+CREG?");
    std::string networkReg = ReadATResponse();
    ESP_LOGI("ModemHandler", "Network Registration Response: %s", networkReg.c_str());

    // Check signal quality
    SendATCommand("AT+CSQ");
    std::string signalQuality = ReadATResponse();
    ESP_LOGI("ModemHandler", "Signal Quality Response: %s", signalQuality.c_str());

    // Check attach status
    SendATCommand("AT+CGATT?");
    std::string attachStatus = ReadATResponse();
    ESP_LOGI("ModemHandler", "Attach Status Response: %s", attachStatus.c_str());
    
    LogGPIOStatus();
}


void ModemHandler::SendATCommand(const std::string &command)
{
    if (dte)
    {
        ESP_LOGI("ModemHandler", "Sending AT Command: %s", command.c_str());
        int written = dte->write((uint8_t*)command.c_str(), command.length());
        if (written != command.length())
        {
            ESP_LOGE("ModemHandler", "Failed to send full command. Written: %d, Expected: %d", written, command.length());
        }
        dte->write((uint8_t*)"\r\n", 2);
    }
    else
    {
        ESP_LOGE("ModemHandler", "DTE not initialized");
    }
}


std::string ModemHandler::ReadATResponse()
{
    if (dte)
    {
        uint8_t* buffer;
        size_t length = 256;

        int len = dte->read(&buffer, length);
        if (len > 0)
        {
            return std::string((char*)buffer, len);
        }
    }
    else
    {
        ESP_LOGE("ModemHandler", "DTE not initialized");
    }
    return "";
}








