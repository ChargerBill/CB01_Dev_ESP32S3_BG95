#include "IotHubClient.h"
//#include "ConfigData.h"
//#include "Logging.h"
//#include "MqttClient.h"
#include "AzureSasToken.h"
#include "esp_log.h"
#include "mqtt_client.h"

// SAS token validity in minutes is currently set for 4 hours (240 minutes)
// TODO: Add these to the SDK Config file
#define SAS_TOKEN_DURATION_IN_MINUTES 20
#define AZURE_SDK_CLIENT_USER_AGENT "c%2F" AZ_SDK_VERSION_STRING "(esp-idf:ESP32-S3)"
#define IOT_CONFIG_IOTHUB_FQDN "CB-IoT-Hub-Development.azure-devices.net"
#define IOT_CONFIG_DEVICE_ID "MQTT-SK-TEST-02"
#define IOT_CONFIG_DEVICE_KEY "OO+W7XDaPrVhJI8BxMjXE0jeGSRv77eLM+JarHEaL8U="

#define sizeofarray(a) (sizeof(a) / sizeof(a[0]))

#define MQTT_QOS1 1
#define DO_NOT_RETAIN_MSG 0

static const char* host = IOT_CONFIG_IOTHUB_FQDN;
static const char *uri = "mqtts://" IOT_CONFIG_IOTHUB_FQDN;
static const char* device_id = IOT_CONFIG_DEVICE_ID;
static char mqtt_client_id[128];
static char mqtt_username[256];
static char mqtt_password[256];

static uint8_t sas_signature_buffer[256];

static char telemetry_topic[128];

static AzureSasToken SasToken(
    &IotHub.client,
    AZ_SPAN_FROM_STR(IOT_CONFIG_DEVICE_KEY),
    AZ_SPAN_FROM_BUFFER(sas_signature_buffer),
    AZ_SPAN_FROM_BUFFER(mqtt_password));

az_iot_hub_client IotHubClient::client;

IotHubClient::IotHubClient() 
{
}

char *IotHubClient::GetHubClientId()
{
  return mqtt_client_id;
}

char *IotHubClient::GetHubUsername()
{
  return mqtt_username;
}

char *IotHubClient::GetHubPassword()
{
  return mqtt_password;
}

const char *IotHubClient::GetHubUri()
{
  return uri;  
}

bool IotHubClient::HasSasTokenExpired()
{
  return SasToken.IsExpired();
}

void IotHubClient::GenerateSasToken()
{
  ESP_LOGI("IotHubClient","Generating New SAS Token");

  if (SasToken.Generate(SAS_TOKEN_DURATION_IN_MINUTES) != 0)
  {
    ESP_LOGE("IotHubClient","(Generate) Failed generating SAS token for %d minutes", SAS_TOKEN_DURATION_IN_MINUTES);
    return; //1;
  }

}

void IotHubClient::Initialise()
{
  ESP_LOGI("IotHubClient","Initialising IotHub Client");

  az_iot_hub_client_options options = az_iot_hub_client_options_default();

  options.user_agent = AZ_SPAN_FROM_STR(AZURE_SDK_CLIENT_USER_AGENT);

  if (az_result_failed(az_iot_hub_client_init(
          &client,
          az_span_create((uint8_t*)host, strlen(host)),
          az_span_create((uint8_t*)device_id, strlen(device_id)),
          &options)))
  {
    ESP_LOGE("IotHubClient","Failed initializing Azure IoT Hub client");
    return;
  }

  size_t client_id_length;
  if (az_result_failed(az_iot_hub_client_get_client_id(
          &client, mqtt_client_id, sizeof(mqtt_client_id) - 1, &client_id_length)))
  {
    ESP_LOGE("IotHubClient","(init) Failed to get IoT Hub MQTT Client ID");
    return;
  }

  int myRes = az_iot_hub_client_get_user_name(&client, mqtt_username, sizeofarray(mqtt_username), NULL);
  if (az_result_failed(myRes))
  {
    ESP_LOGE("IotHubClient","(init) Failed to get IoT Hub MQTT User Name result code %d", myRes);
  }

}

void IotHubClient::SendDeviceToCloudMessage(esp_mqtt_client_handle_t mqttClient, std::string telemetry_payload)
{
  //Logger.Info("Calling Device to Cloud Message Send ...");

  // The topic could be obtained just once during setup,
  // however if properties are used the topic need to be generated again to reflect the
  // current values of the properties.
  if (az_result_failed(az_iot_hub_client_telemetry_get_publish_topic(
          &client, NULL, telemetry_topic, sizeof(telemetry_topic), NULL)))
  {
    ESP_LOGE("IotHubClient", "Failed to compute IoT hub publish topic.");
    return;
  }

  if (esp_mqtt_client_publish(
          mqttClient,
          telemetry_topic,
          (const char*)telemetry_payload.c_str(),
          telemetry_payload.length(),
          MQTT_QOS1,
          DO_NOT_RETAIN_MSG)
      == 0)
  {
    ESP_LOGE("IotHubClient", "Failed to send Device To Cloud Message");
  }
  
}

void IotHubClient::ReplyDirectMessage(esp_mqtt_client_handle_t mqttClient, int webCode, std::string requestId, std::string responsePayload)
{
    // Wierdly the devices SDK has a define for EVERY topic they use, except for the
    // direct commands response one!!!

    char directResponseTopic[512];
    sprintf(directResponseTopic, "$iothub/methods/res/%d/?$rid=%s", webCode, requestId.c_str());

    //Logger.Info("Replying to IoT hub direct message using the following topic:");
    //Logger.Info(directResponseTopic);

    int publishResult = 0;
    if(responsePayload == "")
    {/* TODO: Connect to actual MqttHandler client instance
        publishResult = esp_mqtt_client_publish(
          AzureMqtt.mqtt_client,
          directResponseTopic,
          NULL,
          0,
          MQTT_QOS1,
          DO_NOT_RETAIN_MSG);
    */}
    else
    {
      /*  publishResult = esp_mqtt_client_publish(
          AzureMqtt.mqtt_client,
          directResponseTopic,
          (const char*)responsePayload.c_str(),
          responsePayload.length(),
          MQTT_QOS1,
          DO_NOT_RETAIN_MSG);*/
    }

    if (publishResult == 0)
    {
      //Logger.Error("Failed to Publish Direct Method Response");
    }
}

void IotHubClient::SubscribeToAzureHandlers(esp_mqtt_client_handle_t mqttClient)
{
    int subsResult;
    
    // Messages recieved containing Cloud to Device messages
    subsResult = esp_mqtt_client_subscribe(mqttClient, AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC, 1);
    if (subsResult == -1)
    {
      ESP_LOGE("IotHubClient","Could not subscribe for cloud-to-device messages.");
    }
    else
    {
      ESP_LOGI("IotHubClient","Subscribed for cloud-to-device messages; message id: %d", subsResult);
      ESP_LOGI("IotHubClient",AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC);
    }

      // Messages received on the Direct Methods topic will be method commands to be invoked.
    subsResult = esp_mqtt_client_subscribe(mqttClient, AZ_IOT_HUB_CLIENT_METHODS_SUBSCRIBE_TOPIC, 1);
    if (subsResult == -1)
    {
      ESP_LOGE("IotHubClient","Could not subscribe for direct method messages.");
    }
    else
    {
      ESP_LOGI("IotHubClient","Subscribed for direct method messages; message id: %d", subsResult);
      ESP_LOGI("IotHubClient",AZ_IOT_HUB_CLIENT_METHODS_SUBSCRIBE_TOPIC);
    }

    // Messages received on the Twin Response topic will be responses from device twins.
    subsResult = esp_mqtt_client_subscribe(mqttClient, AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_SUBSCRIBE_TOPIC, 1);
    if (subsResult == -1)
    {
      ESP_LOGE("IotHubClient","Could not subscribe for device twin response messages.");
    }
    else
    {
      ESP_LOGI("IotHubClient","Subscribed for device twin response messages; message id: %d", subsResult);
      ESP_LOGI("IotHubClient",AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_SUBSCRIBE_TOPIC);
    }

    // Messages received on the Twin Patch topic will be requests from device twins to patch things.
    // This is called when a "desired" property is changed in the device twin.
    subsResult = esp_mqtt_client_subscribe(mqttClient, AZ_IOT_HUB_CLIENT_TWIN_PATCH_SUBSCRIBE_TOPIC, 1);
    if (subsResult == -1)
    {
      ESP_LOGE("IotHubClient","Could not subscribe for device twin patch messages.");
    }
    else
    {
      ESP_LOGI("IotHubClient","Subscribed for device twin patch messages; message id: %d", subsResult);
      ESP_LOGI("IotHubClient",AZ_IOT_HUB_CLIENT_TWIN_PATCH_SUBSCRIBE_TOPIC);
    }
}