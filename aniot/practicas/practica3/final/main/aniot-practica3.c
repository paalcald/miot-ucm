#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "esp_err.h"
#include "esp_event_base.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/idf_additions.h"
#include "mock_wifi.h"
#include "mock_flash.h"
#include "mock_temp.h"
#include "esp_event.h"
#include "fsm_events.h"
#include "driver/gpio.h"
#include "esp_pm.h"
/* TODO implement a component to describe STATE_EVENTs */
/* #include "state_event.h" */

#define GPIO_STATE_SEND 0
#define MAX_FLASH_CAPACITY 1024

static char *TAG = "MAIN APP";
esp_event_loop_handle_t g_h_monitorization_event_loop;

static void send_or_store(void *data, size_t len)
{
    esp_err_t send_state = send_data_wifi(data, len);
    switch (send_state) {
        case ESP_ERR_INVALID_STATE:
            writeToFlash(data, len);
            break;

        case ESP_OK:
            ESP_LOGD(TAG, "Temperature measurement correctly sent using wifi");
            break;

        default:
            ESP_LOGD(TAG, "Unhandled return state");
            break;
    }
}


static void temp_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispached from event loop base=%s, event_id=%" PRIi32
             "", base, event_id);
    float temp = *((float*) event_data);
    switch ((temp_event_id_t) event_id) {
        case TEMP_MOCK_NEW_MEASUREMENT:
            send_or_store(&temp, 1);
            break;

        case TEMP_MOCK_PAUSED:
            temp_mock_start();
            ESP_LOGI(TAG, "Temperature measurement restarted");
            break;

        default:
            ESP_LOGD(TAG, "Unhandled event_id");
            break;
    }
}

static void wifi_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispached from event loop base=%s, event_id=%" PRIi32
             "", base, event_id);
    size_t bytes_stored;
    void *flashContents;
    switch ((wifi_event_id_t)event_id) {
        case WIFI_MOCK_EVENT_WIFI_CONNECTED:
            ESP_LOGI("TAG", "Wifi connected, waiting for IP");
            break;

        case WIFI_MOCK_EVENT_WIFI_GOT_IP:
            bytes_stored = getDataLeft();
            if (bytes_stored > 0) {
                flashContents = readFromFlash(bytes_stored); //does malloc inside! we need to free
                send_or_store(flashContents, bytes_stored);
                free(flashContents);
            }
            break;

        case WIFI_MOCK_EVENT_WIFI_DISCONNECTED:
            wifi_connect();
    }
}

static void state_event_handler(void * handler_args, esp_event_base_t base,
                                int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispached from event loop base=%s, event_id=%" PRIi32
             "", base, event_id);
    switch ((state_event_id_t)event_id) {
        case STATE_EVENT_MONITORING:
            /* Logic to stop the console state if needed.*/
            ESP_ERROR_CHECK(esp_event_handler_register_with(g_h_monitorization_event_loop,
                                                            TEMP_MOCK,
                                                            ESP_EVENT_ANY_ID,
                                                            temp_event_handler,
                                                            NULL));

            ESP_ERROR_CHECK(esp_event_handler_register_with(g_h_monitorization_event_loop,
                                                            WIFI_MOCK,
                                                            ESP_EVENT_ANY_ID,
                                                            wifi_event_handler,
                                                            NULL));
            break;

        case STATE_EVENT_CONSOLE:
            /* If we mirror the autoreconnect functionality of wifi on the temp
             * sensor it could save us on manually reconnecting on monitoring*/
            ESP_ERROR_CHECK(esp_event_handler_unregister_with(g_h_monitorization_event_loop,
                                                            TEMP_MOCK,
                                                            ESP_EVENT_ANY_ID,
                                                            temp_event_handler));

            /* If we call wifi_disconnect() prior to unregistering, it will
             * attempt to connect back up inside the handler*/
            ESP_ERROR_CHECK(esp_event_handler_unregister_with(g_h_monitorization_event_loop,
                                                            WIFI_MOCK,
                                                            ESP_EVENT_ANY_ID,
                                                            wifi_event_handler));
            /* This should still generate a WIFI_MOCK_EVENT_WIFI_DISCONNECTED
             * so that once the handler gets registered again it should connect
             * automatically, if it doesn't that logic needs to get added */
            wifi_disconnect();
            temp_mock_pause();
            /* TODO! implement console logic */
            break;
    }
}

void app_main(void)
{
//    gpio_config_t config = {
//            .pin_bit_mask = BIT64(GPIO_STATE_SEND),
//            .mode = GPIO_MODE_INPUT,
//            .pull_down_en = false,
//            .pull_up_en = false,
//            .intr_type = GPIO_INTR_DISABLE
//    };
//    ESP_ERROR_CHECK(gpio_config(&config));

    esp_pm_config_t power_management_cfg = {
        .max_freq_mhz = 80,
        .min_freq_mhz = 40,
        .light_sleep_enable = true,
    };
    ESP_ERROR_CHECK(esp_pm_configure(&power_management_cfg));

    ESP_ERROR_CHECK(mock_flash_init(MAX_FLASH_CAPACITY));

    esp_event_loop_args_t monitorization_event_loop_args = {
        .queue_size =5,
        .task_name = "monitorization_task", /* since it is a task it can be stopped */
        .task_stack_size = 4096,
        .task_priority = uxTaskPriorityGet(NULL),
        .task_core_id = tskNO_AFFINITY,
    };
    esp_event_loop_handle_t h_state_event_loop;
    esp_event_loop_args_t state_event_loop_args = {
        .queue_size =5,
        .task_name = "state_switcher_task", /* since it is a task it can be stopped */
        .task_stack_size = 4096,
        .task_priority = uxTaskPriorityGet(NULL),
        .task_core_id = tskNO_AFFINITY,
    };
    ESP_ERROR_CHECK(esp_event_loop_create(&monitorization_event_loop_args,
                                          &g_h_monitorization_event_loop));

    ESP_ERROR_CHECK(esp_event_loop_create(&state_event_loop_args,
                                          &h_state_event_loop));

    ESP_ERROR_CHECK(esp_event_handler_register_with(h_state_event_loop,
                                                    STATE_EVENT,
                                                    ESP_EVENT_ANY_ID,
                                                    state_event_handler,
                                                    NULL));
    wifi_mock_init(g_h_monitorization_event_loop);
    temp_mock_init(g_h_monitorization_event_loop);
    esp_event_post_to(h_state_event_loop, STATE_EVENT, STATE_EVENT_MONITORING, NULL, 0, NULL);
    ESP_ERROR_CHECK(wifi_connect());
    ESP_ERROR_CHECK(temp_mock_start());

}
