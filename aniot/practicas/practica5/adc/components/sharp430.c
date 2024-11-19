#include "esp_adc/adc_cali.h"
#include "esp_err.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali_scheme.h"
#include <math.h>
#include "esp_check.h"
#include "esp_event.h"
#include "esp_timer.h"
#include "esp_event_base.h"
#include "esp_err.h"
#include "esp_log.h"
#include "hal/adc_types.h"
#include "portmacro.h"
#include "sharp430.h"

ESP_EVENT_DEFINE_BASE(SENSOR_EVENTS);

static char* TAG = "Sharp 430";
static adc_oneshot_unit_init_cfg_t unit_cfg;
static adc_oneshot_chan_cfg_t chan_cgf;
static adc_cali_line_fitting_config_t cali_cfg;
static adc_cali_handle_t cali_h;
static esp_event_loop_handle_t loop_connect;
static esp_timer_handle_t timer_h;


static adc_oneshot_unit_handle_t unit_h;
static adc_channel_t chan;
static float curr_distance;

void check_cfgs( void ) {
    ESP_LOGI(TAG, "atten = %d, bitwidth = %d", cali_cfg.atten, cali_cfg.bitwidth );
}

double sharp430_voltage_to_distance(double voltage)
{
    return __SHARP_A + (__SHARP_B * __SHARP_X(1 / voltage)) + (__SHARP_C * pow(__SHARP_X(1 / voltage), 2));
}

static void sharp430_measuring_callback( void * arg)
{
    ESP_LOGI(TAG, "called callback");
    int raw_measurement;
    int voltage;
    int measured_voltage = 0;
    for (int i =0; i < SHARP430_NUM_AVERAGES; i++) {
        ESP_ERROR_CHECK(adc_oneshot_read(unit_h, chan, &raw_measurement));
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali_h, raw_measurement, &voltage));
        measured_voltage += voltage;
    }
    measured_voltage /= SHARP430_NUM_AVERAGES;
    ESP_LOGI(TAG, "Average voltage is %d", measured_voltage);
    curr_distance = sharp430_voltage_to_distance(((float) measured_voltage / 1000));
    ESP_LOGI(TAG, "Distance measured: %.2f cm", curr_distance);
    esp_event_post_to(loop_connect, SENSOR_EVENTS, SENSOR_EVENT_DISTANCE_NEW, &curr_distance, sizeof(float), portMAX_DELAY);
}

esp_err_t sharp430_check_bounds(double voltage){
    if (voltage > __SHARP_VOLTAGE_MAX) {
        ESP_LOGE(TAG, "Input voltage is too high (%.2fV)", voltage);
        return ESP_ERR_INVALID_ARG;
    } else if (voltage < __SHARP_VOLTAGE_MIN) {
        ESP_LOGE(TAG, "Input voltage is too low (%.2fV)", voltage);
        return ESP_ERR_INVALID_ARG;
    } else {
        return ESP_OK;
    }
}

esp_err_t sharp430_init(const sharp430_cfg_t *c) {
    unit_cfg.unit_id = c->unit;
    ESP_RETURN_ON_ERROR(adc_oneshot_new_unit(&unit_cfg, &unit_h), TAG, "Unable to start unit");
    chan_cgf.bitwidth = c->bitwidth;
    chan_cgf.atten = c->atten;
    chan = c->channel;
    ESP_RETURN_ON_ERROR(adc_oneshot_config_channel(unit_h,
                                                   chan,
                                                   &chan_cgf),
                        TAG, "Unable to config ADC channel");
    return ESP_OK;
}

esp_err_t sharp430_calibrate(const sharp430_cfg_t *c) {
    cali_cfg.atten = c->atten;
    cali_cfg.bitwidth = c->bitwidth;
    cali_cfg.unit_id = c->unit;
    esp_err_t ret = adc_cali_create_scheme_line_fitting(&cali_cfg, &cali_h);
    return ret;
}

esp_err_t sharp430_measuring_init(esp_event_loop_handle_t e) {
    loop_connect = e;

    const esp_timer_create_args_t timer_args = {
        .callback = &sharp430_measuring_callback,
        .name = "measurement",
    };

    return esp_timer_create(&timer_args, &timer_h);
}

esp_err_t sharp430_measuring_start(uint64_t interval) {
    return esp_timer_start_periodic(timer_h, interval);
};
esp_err_t sharp430_measuring_stop(adc_oneshot_unit_handle_t s);
