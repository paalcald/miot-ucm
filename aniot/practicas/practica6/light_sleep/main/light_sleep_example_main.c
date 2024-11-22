/*
 * SPDX-FileCopyrightText: 2021-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "light_sleep_example.h"

#define GET_TIME_INTERVAL_US (500 * 1000)
static int sleep_count = 0;

static void get_time_timer_callback(void *args) {
    ESP_LOGI("TIME", "since boot%" PRId64 "",esp_timer_get_time());
}

static void light_sleep_task(void *args)
{
    while (true) {
        if (sleep_count < 5) {
            printf("Entering light sleep\n");
        } else {
            printf("Entering deep sleep\n");
        }
        /* To make sure the complete line is printed before entering sleep mode,
         * need to wait until UART TX FIFO is empty:
         */
        uart_wait_tx_idle_polling(CONFIG_ESP_CONSOLE_UART_NUM);

        /* Get timestamp before entering sleep */
        int64_t t_before_us = esp_timer_get_time();

        /* Enter sleep mode */
        if (sleep_count < 5) {
            sleep_count += 1;
            esp_light_sleep_start();
        } else {
            sleep_count = 0;
            esp_deep_sleep_start();
        }

        /* Get timestamp after waking up from sleep */
        int64_t t_after_us = esp_timer_get_time();

        /* Determine wake up reason */
        const char* wakeup_reason;
        switch (esp_sleep_get_wakeup_cause()) {
            case ESP_SLEEP_WAKEUP_TIMER:
                wakeup_reason = "timer";
                break;
            case ESP_SLEEP_WAKEUP_GPIO:
                wakeup_reason = "pin";
                break;
            case ESP_SLEEP_WAKEUP_UART:
                wakeup_reason = "uart";
                /* Hang-up for a while to switch and execute the uart task
                 * Otherwise the chip may fall sleep again before running uart task */
                vTaskDelay(1);
                break;
#if EXAMPLE_TOUCH_LSLEEP_WAKEUP_SUPPORT
            case ESP_SLEEP_WAKEUP_TOUCHPAD:
                wakeup_reason = "touch";
                break;
#endif
            default:
                wakeup_reason = "other";
                break;
        }
#if CONFIG_NEWLIB_NANO_FORMAT
        /* printf in newlib-nano does not support %ll format, causing example test fail */
        printf("Returned from light sleep, reason: %s, t=%d ms, slept for %d ms\n",
                wakeup_reason, (int) (t_after_us / 1000), (int) ((t_after_us - t_before_us) / 1000));
#else
        printf("Returned from light sleep, reason: %s, t=%" PRId64 " ms, slept for %" PRId64" ms\n",
                wakeup_reason, t_after_us / 1000, (t_after_us - t_before_us) / 1000);
#endif
        if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_GPIO) {
            /* Waiting for the gpio inactive, or the chip will continuously trigger wakeup*/
            example_wait_gpio_inactive();
        }
    }
    vTaskDelete(NULL);
}

void app_main(void)
{
    /* Enable wakeup from light sleep by gpio */
    example_register_gpio_wakeup();
    /* Enable wakeup from light sleep by timer */
    example_register_timer_wakeup();
    /* Enable wakeup from light sleep by uart */
    // example_register_uart_wakeup();
#if EXAMPLE_TOUCH_LSLEEP_WAKEUP_SUPPORT
    /* Enable wakeup from light sleep by touch element */
    example_register_touch_wakeup();
#endif
    esp_timer_create_args_t get_time_timer_args = {
        .callback = get_time_timer_callback,
        .arg = NULL,
        .name = "get_time",
    };
    esp_timer_handle_t h_get_time_timer;
    esp_timer_create(&get_time_timer_args, &h_get_time_timer);
    esp_timer_start_periodic(h_get_time_timer, GET_TIME_INTERVAL_US);
    vTaskDelay(3 * pdMS_TO_TICKS(1000));
    xTaskCreate(light_sleep_task, "light_sleep_task", 4096, NULL, 6, NULL);
}
