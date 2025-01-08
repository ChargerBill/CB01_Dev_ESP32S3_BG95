/*
 * TaskEventBits.h
 *
 *  Created on : 2 Dec 2024
 *  Author     : P.J.Shaw
 *  Purpose    : Hold's system wide DEFINES for the ESP-IDF event group system, allowing event based
 *             : Task syncronisation, all TASK modules should include this header.
 */

#ifndef TASKEVENTBITS_H
#define TASKEVENTBITS_H

#define COMMS_AVAILABLE_BIT (1 << 0)
#define DEVICE_TIME_SET_BIT (1 << 1)
#define MQTT_AVAILABLE_BIT (1 << 2)

#include "freertos/FreeRTOS.h"

// Main application wide event group, uses flag definitions in 'TaskEventBits.h' definition in main task.
extern EventGroupHandle_t ApplicationEvents;

#endif /* TASKEVENTBITS_H */
