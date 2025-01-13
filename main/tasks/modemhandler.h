#ifndef MODEMHANDLER_H
#define MODEMHANDLER_H

#include <memory>
#include <string>

#include "esp_netif_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_modem_config.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "esp_event.h"
#include "cxx_include/esp_modem_dte.hpp"
#include "cxx_include/esp_modem_api.hpp"
#include "esp_modem_config.h"
#include "esp_netif_defaults.h"
#include "esp_netif_ppp.h"
#include "cxx_include/esp_modem_api.hpp"
#include "cxx_include/esp_modem_types.hpp"
#include "esp_modem_c_api_types.h"
#include "driver/gpio.h"
#include "hal/gpio_types.h"

#include "TaskEventBits.h"
#include "event.h"

#define TX_PIN 17
#define RX_PIN 18

#define PPP_APN "quectel.tn.std"

using dce_config = ::esp_modem_dce_config;
using dte_config = ::esp_modem_dte_config;
using netif_config = ::esp_netif_config_t;

class ModemHandler
{
  
  public:
    static std::string ipAddress;
    static bool isPoweredOn;
    static bool isConnected;
    static BooleanEvent PppConnectedEvent;

    static void Start();
    void PowerModemOn();
    void PowerModemOff();

  private:
    dte_config dteConfig;
    dce_config dceConfig;
    netif_config netifConfig;
    std::shared_ptr<esp_modem::DTE> dte;
    std::shared_ptr<esp_modem::DCE> dce;
    esp_netif_t *pppInterface;

    static void TaskInit(void *pvParameters);
    void TaskLoop();
    
    void PulseModemPowerKey();
    void InitModem();
    static void pppChangedEvent(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
    static void ipEvent(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
        
};

#endif
