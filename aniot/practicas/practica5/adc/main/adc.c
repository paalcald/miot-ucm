#include <stdio.h>
#include "esp_event.h"
#include "esp_event_base.h"
#include "freertos/projdefs.h"
#include "hal/adc_types.h"
#include "sharp430.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali_scheme.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static char* TAG = "ADC";
static int adc_raw;
static int voltage;
static float distance;

static void distance_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "Event received");
    ESP_LOGD(TAG, "Event dispached from event loop base=%s, event_id=%" PRIi32
             "", base, event_id);
    float dist = *((float*) event_data);
    switch ((dist_event_id_t) event_id) {
        case SENSOR_EVENT_DISTANCE_NEW:
            ESP_LOGI(TAG, "Distance measured: %.2f cm", dist);
            break;

        default:
            ESP_LOGD(TAG, "Unhandled event_id");
            break;
    }
}

void app_main(void)
{
    #ifdef TRYING_ALTERNATIVES
    adc_oneshot_unit_handle_t adc1_h;
    adc_oneshot_unit_init_cfg_t adc1_unit_cfg = {
        .unit_id = ADC_UNIT_1,
    };

    ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc1_unit_cfg, &adc1_h));

    adc_oneshot_chan_cfg_t adc1_chan_cgf = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten = ADC_ATTEN_DB_12,
    };

    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_h,
                                               ADC_CHANNEL_4,
                                               &adc1_chan_cgf));
    ESP_LOGI(TAG, "ADC configuration done");

    /* This first time we go with no calibration,
     * we first take a look at the unattenuated reading */
    adc_cali_line_fitting_config_t adc1_cali_cfg = {
        .unit_id = ADC_UNIT_1,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };

    adc_cali_handle_t adc1_cali_h;
    adc_cali_create_scheme_line_fitting(&adc1_cali_cfg , &adc1_cali_h);
    #endif
    sharp430_cfg_t sharp430_cfg = {
        .unit = ADC_UNIT_1,
        .channel = ADC_CHANNEL_4,
        .bitwidth = ADC_BITWIDTH_12,
        .atten = ADC_ATTEN_DB_12,
    };

    esp_event_loop_args_t sensor_event_loop_args = {
        .queue_size = 5,
        .task_name = "sensoring",
        .task_stack_size = 3072,
        .task_priority = uxTaskPriorityGet(NULL) + 1,
        .task_core_id = tskNO_AFFINITY,
    };
    esp_event_loop_handle_t sensor_event_loop;
    ESP_ERROR_CHECK(esp_event_loop_create(&sensor_event_loop_args, &sensor_event_loop));
    esp_event_handler_instance_t distance_register_instance;
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(sensor_event_loop,
                                                    SENSOR_EVENTS,
                                                    SENSOR_EVENT_DISTANCE_NEW,
                                                    distance_event_handler,
                                                    NULL,
                                                    &distance_register_instance));
    ESP_ERROR_CHECK(sharp430_init(&sharp430_cfg));
    ESP_ERROR_CHECK(sharp430_calibrate(&sharp430_cfg));
    ESP_ERROR_CHECK(sharp430_measuring_init(sensor_event_loop));
    ESP_ERROR_CHECK(sharp430_measuring_start(1000 * 1000));
    check_cfgs();



    //double distance = sharp430_voltage_to_distance(voltage);
    //ESP_LOGI(TAG, "%.2f V => %.2f cm", voltage, distance);
}
