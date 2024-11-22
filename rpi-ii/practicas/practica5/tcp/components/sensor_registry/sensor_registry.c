#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "sensor_registry.h"
#include "mqtt_client.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

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

const char *sensor_typetoext(const sensor_type_t type)
{
    switch(type) {
        case SENSOR_TYPE_TEMP:
            return SENSOR_TEMP_EXT;

        case SENSOR_TYPE_HUM:
            return SENSOR_HUM_EXT;

        case SENSOR_TYPE_LUX:
            return SENSOR_LUX_EXT;

        case SENSOR_TYPE_VIBR:
            return SENSOR_VIBR_EXT;

        default:
            return NULL;
    }
}

const char *sensor_typetokey(const sensor_type_t type)
{
    switch(type) {
        case SENSOR_TYPE_TEMP:
            return SENSOR_TEMP_KEY;

        case SENSOR_TYPE_HUM:
            return SENSOR_HUM_KEY;

        case SENSOR_TYPE_LUX:
            return SENSOR_LUX_KEY;

        case SENSOR_TYPE_VIBR:
            return SENSOR_VIBR_KEY;

        default:
            return NULL;
    }
}

static const char* TAG = "Sensor Registry";

int sensor_registry_new_entry(const sensor_info_t s, sensor_registry_t* reg)
{
    reg->len += 1;
    //copy here helps against fragmentation and cache miss?
    sensor_info_t *tmp = realloc(reg->entries, sizeof(sensor_info_t) * reg->len);
    if (tmp == NULL) {
        return -1;
    } else {
        reg->entries = tmp;
    }
    return 0;
}

int sensor_registry_check_event(sensor_registry_t *const reg, esp_mqtt_event_handle_t event)
{
    char* building_name = strtok(event->topic, "/");
    char* building_story = strtok(NULL, "/");
    char* building_wing = strtok(NULL, "/");
    char* building_room = strtok(NULL, "/");
    char* sensor_type = strtok(NULL, "/");
    size_t sensor_type_len = event->topic_len - (sensor_type - event->topic);
    char* command = strtok(NULL, "/");
    size_t command_len = event->topic_len - (command - event->topic);
        // optimization can be reached using lexicographic order and strcomp nature
    bool found_match = false;
    int i = 0;
    sensor_info_t s;
    while (!found_match & (i < reg->len)) {
        s = reg->entries[i];
        if (strncmp(sensor_type, sensor_typetoext(s.type), sensor_type_len) == 0) {
            found_match = true;
            if(strncmp(command, INTERVAL_EXTENSION, command_len) == 0) {
                event->data[event->data_len] = '\0';
                int maybe_interval = (int) strtol(event->data, (char**) NULL, 10);
                if (maybe_interval != 0) {
                    s.interval = maybe_interval;
                    ESP_LOGI(TAG, "Received interval=%llu", s.interval);
                    if (esp_timer_is_active(s.measurement_timer)) {
                        esp_timer_restart(s.measurement_timer, s.interval);
                    }
                }
            } else if (strncmp(command, ENABLE_EXTENSION, command_len) == 0) {
                if (esp_timer_is_active(s.measurement_timer)) {
                    ESP_LOGI(TAG, "Attempted to enable to enable an already running sensor");
                } else {
                    esp_timer_start_periodic(s.measurement_timer, s.interval);
                }
            } else if (strncmp(command, DISABLE_EXTENSION, command_len) == 0) {
                if (esp_timer_is_active(s.measurement_timer)) {
                    esp_timer_stop(s.measurement_timer);
                } else {
                    ESP_LOGI(TAG, "Attempted to stop an already stopped sensor");
                }
            } else {
                ESP_LOGI(TAG, "Unregistered MQTT extension %*.s", command_len, command);
            }
        }
        i++;
    }
    return found_match;
}
