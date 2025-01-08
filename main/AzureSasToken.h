#ifndef AZIOTSASTOKEN_H
#define AZIOTSASTOKEN_H

#include <stdlib.h>
#include <time.h>
#include "esp_log.h"

#include <az_result.h>
#include <az_iot_hub_client.h>
#include <az_span.h>

#include <mbedtls/base64.h>
#include <mbedtls/md.h>
#include <mbedtls/sha256.h>

class AzureSasToken
{
  public:
    AzureSasToken(
      az_iot_hub_client* client,
      az_span deviceKey,
      az_span signatureBuffer,
      az_span sasTokenBuffer);
    int Generate(unsigned int expiryTimeInMinutes);
    bool IsExpired();
    az_span Get();

  private:
    az_iot_hub_client* client;
    az_span deviceKey;
    az_span signatureBuffer;
    az_span sasTokenBuffer;
    az_span sasToken;
    uint32_t expirationUnixTime;
};

#endif // AZIOTSASTOKEN_H
