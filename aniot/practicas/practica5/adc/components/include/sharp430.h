#ifndef SHARP430_H
#define SHARP430_H
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_event_base.h"

#define __SHARP_A 23.35902732
#define __SHARP_B 17.37947336
#define __SHARP_C -1.73252891
#define __SHARP_X(X) (-1.21826282 + (0.66378555 * X))
#define __SHARP_VOLTAGE_MIN 0.3
#define __SHARP_VOLTAGE_MAX 3.05

#define SHARP430_UNIT CONFIG_SHARP430_UNIT
#define SHARP430_CHAN CONFIG_SHARP430_CHAN
#define SHARP430_BITWIDTH CONFIG_SHARP430_BITWIDTH
#define SHARP430_ATTEN CONFIG_SHARP430_ATTEN
#define SHARP430_NUM_AVERAGES CONFIG_SHARP430_NUM_AVERAGES

typedef enum {
    SENSOR_EVENT_DISTANCE_NEW,
} dist_event_id_t ;
ESP_EVENT_DECLARE_BASE(SENSOR_EVENTS);

typedef struct sharp430_cfg {
    int unit;
    int channel;
    int bitwidth;
    int atten;
} sharp430_cfg_t;

esp_err_t sharp430_init(const sharp430_cfg_t *c);
esp_err_t sharp430_calibrate(const sharp430_cfg_t *c);

esp_err_t sharp430_measuring_init(esp_event_loop_handle_t e);
esp_err_t sharp430_measuring_start(uint64_t interval);
esp_err_t sharp430_measuring_stop(adc_oneshot_unit_handle_t s);

esp_err_t sharp430_check_bounds(double voltage);
double sharp430_voltage_to_distance(double voltage);
#endif
