#ifndef SENSOR_REGISTRY_H
#define SENSOR_REGISTRY_H

#include "esp_timer.h"
#include "mqtt_client.h"

#define SENSOR_TEMP_EXT "TEMP"
#define SENSOR_HUM_EXT "HUM"
#define SENSOR_LUX_EXT "LUX"
#define SENSOR_VIBR_EXT "VIBR"
#define SENSOR_TEMP_KEY "temperatura"
#define SENSOR_HUM_KEY "humedad"
#define SENSOR_LUX_KEY "luz"
#define SENSOR_VIBR_KEY "vibracion"
#define SENSOR_EXTENSION_MAX_LEN 8
#define SENSOR_MESSAGE_KEY_MAX_LEN 16
#define ENABLE_EXTENSION "enable"
#define DISABLE_EXTENSION "disable"
#define INTERVAL_EXTENSION "interval"

typedef enum sensor_type {
    SENSOR_TYPE_TEMP,
    SENSOR_TYPE_HUM,
    SENSOR_TYPE_LUX,
    SENSOR_TYPE_VIBR,
} sensor_type_t;

const char *sensor_typetoext(const sensor_type_t type);
const char *sensor_typetokey(const sensor_type_t type);

typedef struct sensor_handle {
    int handle;
} sensor_handle_t;

typedef struct sensor_info {
    sensor_type_t type;
    uint64_t interval;
    sensor_handle_t handle;
    esp_timer_handle_t measurement_timer;
} sensor_info_t;

typedef struct sensor_registry {
    int enabled_sensors;
    sensor_info_t *entries;
    size_t len;
} sensor_registry_t;

int sensor_registry_new_entry(const sensor_info_t s, sensor_registry_t* reg);

int sensor_registry_check_event(sensor_registry_t *const reg, esp_mqtt_event_handle_t event);
#endif // !SENSOR_REGISTRY_H
