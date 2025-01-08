#include "mqtthandler.h"
#include "TaskEventBits.h"
#include "modemhandler.h"

#include <azure_ca.h>

//#define mqtt_broker_uri "mqtt://mqtt.digital-solutions.me.uk/"
#define mqtt_port 8883
//#define mqtt_user "shawty"
//#define mqtt_paswd "dsfoo"
//#define mqtt_client_id "ESP32-S3-IDF-FREERTOS"

bool MqttHandler::_isModemConnected = false;
bool MqttHandler::_isMqttConnected = false;
bool MqttHandler::_isSubscribed = false;

esp_mqtt_client_handle_t MqttHandler::_client = NULL;
IotHubClient MqttHandler::_iotClient;

void MqttHandler::Start()
{
  BaseType_t xReturned = xTaskCreate(TaskInit, 
  "MqttHandlerTask", 
  8192, // 8k 
  NULL, 
  tskIDLE_PRIORITY, 
  NULL);

  if (xReturned != pdPASS) {
    // Handle task creation failure
    ESP_LOGE("MqttHandler", "FATAL: Failed to create MQTT Handling task (NO MQTT Services available).");
    return;
  }
}

void MqttHandler::TaskInit(void *pvParameters)
{
  ESP_LOGI("MqttHandler", "Initialising Task Module");
  MqttHandler instance;
  
  // This service pauses here until DEVICE_TIME_SET_BIT is set in the "ApplicationEvents" event group
  ESP_LOGI("MqttHandler", "Waiting for Device time to be set.");
  xEventGroupWaitBits(ApplicationEvents, DEVICE_TIME_SET_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
  
  // NOTE: This *MUST* be called before trying to setup and use the actual MQTT instance
  // as this sets up EVERYTHING needed for the Azure SaS token Generation.
  _iotClient = *new IotHubClient();
  _iotClient.Initialise();

  instance.StartMqtt();
  instance.TaskLoop();
}

void MqttHandler::TaskLoop()
{
  ESP_LOGI("MqttHandler", "Entering Main Task Module Loop");
  
  while (true)
  {    
    if(_isMqttConnected)
    {
      if (_iotClient.HasSasTokenExpired())
      {
        ESP_LOGI("MqttHandler", "SAS token expired; reconnecting with a new one.");
        (void)esp_mqtt_client_destroy(_client);
        StartMqtt();
      }
    }
    else
    {
      ESP_LOGI("MqttHandler", "MQTT Connection Lost, reconnecting.");
      (void)esp_mqtt_client_destroy(_client);
      StartMqtt();
    }
    
    vTaskDelay(pdMS_TO_TICKS(1000)); // Delay to simulate task timing (1 second for this task)
  }
}

void MqttHandler::SubscribeToNotifications()
{
  _iotClient.SubscribeToAzureHandlers(_client);  
  _isSubscribed = true;
  
}

void MqttHandler::EventHandler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    MqttHandler instance;
    
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    //esp_mqtt_client_handle_t client = event->client;
    int msg_id = event->msg_id;

    switch (event->event_id)
    {
      case MQTT_EVENT_CONNECTED:
        ESP_LOGI("MqttHandler", "MQTT_EVENT_CONNECTED");
        _isMqttConnected = true;
        xEventGroupSetBits(ApplicationEvents, MQTT_AVAILABLE_BIT);
        instance.SubscribeToNotifications();
        
        break;
        
      case MQTT_EVENT_DISCONNECTED:
      
        ESP_LOGI("MqttHandler", "MQTT_EVENT_DISCONNECTED");
        _isMqttConnected = false;
        _isSubscribed = false;
        xEventGroupClearBits(ApplicationEvents, MQTT_AVAILABLE_BIT);
        break;

      case MQTT_EVENT_SUBSCRIBED:
      
        ESP_LOGI("MqttHandler", "MQTT_EVENT_SUBSCRIBED, msg_id=%d", msg_id);
        break;
        
      case MQTT_EVENT_UNSUBSCRIBED:
      
        ESP_LOGI("MqttHandler", "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", msg_id);
        break;
        
      case MQTT_EVENT_PUBLISHED:
      
        ESP_LOGI("MqttHandler", "MQTT_EVENT_PUBLISHED, msg_id=%d", msg_id);
        break;
        
      case MQTT_EVENT_DATA:
      
        // TODO: Dispatch event containing topic details
        ESP_LOGI("MqttHandler", "MQTT_EVENT_DATA");
        ESP_LOGI("MqttHandler", "TOPIC=%.*s", event->topic_len, event->topic);
        ESP_LOGI("MqttHandler", "DATA=%.*s", event->data_len, event->data);
        break;
        
      case MQTT_EVENT_ERROR:
      
        ESP_LOGI("MqttHandler", "MQTT_EVENT_ERROR");
        break;
        
      default:
      
        ESP_LOGI("MqttHandler", "Other event id:%d", event->event_id);
        break;
    }
}

void MqttHandler::StartMqtt()
{
  ESP_LOGI("MqttHandler", "Starting MQTT Client");

  _iotClient.GenerateSasToken();
         
  esp_mqtt_client_config_t mqtt_config;
  memset(&mqtt_config, 0, sizeof(mqtt_config));

  mqtt_config.broker.address.uri = _iotClient.GetHubUri();

  mqtt_config.broker.address.port = mqtt_port;

  mqtt_config.credentials.client_id = _iotClient.GetHubClientId();

  mqtt_config.credentials.username = _iotClient.GetHubUsername();

  mqtt_config.credentials.authentication.password = _iotClient.GetHubPassword();
  
  mqtt_config.session.keepalive = 30;

  mqtt_config.session.disable_clean_session = 0;

  mqtt_config.network.disable_auto_reconnect = true;
  
  mqtt_config.broker.verification.certificate = (const char*)ca_pem;

  _client = esp_mqtt_client_init(&mqtt_config);
  if (_client == NULL)
  {
    ESP_LOGE("MqttHandler","Failed to Create MQTT Client Object");
    return;
  }

  esp_mqtt_client_register_event(_client, (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID, EventHandler, NULL); // NEW IDF 3.0.x and above

  esp_err_t start_result = esp_mqtt_client_start(_client);
  if (start_result != ESP_OK)
  {
    ESP_LOGE("MqttHandler","MQTT Client Failed to Start, error code: %d", start_result);
    return;
  }
  else
  {
    ESP_LOGI("MqttHandler", "DEBUG: IotClient Hub Uri  : %s", _iotClient.GetHubUri());
    ESP_LOGI("MqttHandler", "DEBUG: IotClient ClientId : %s", _iotClient.GetHubClientId());
    ESP_LOGI("MqttHandler", "DEBUG: IotClient Username : %s", _iotClient.GetHubUsername());
    ESP_LOGI("MqttHandler", "DEBUG: IotClient Password : %s", _iotClient.GetHubPassword());
    ESP_LOGI("MqttHandler","MQTT Client Started Successfully");
    return;
  }
  
}

void MqttHandler::SendMqttMessageToAzure(std::string messageText)
{
  if(_isMqttConnected)
  {
    _iotClient.SendDeviceToCloudMessage(_client, messageText);
  }
  else
  {
    ESP_LOGW("MqttHandler", "Attempt to send Device to Cloud Message Before Mqtt Connected!");
  }
}

