/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include "esp_err.h"
#include "esp_timer.h"
#include "freertos/projdefs.h"
#include "hal/gpio_types.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "shtc3.h"
#include "soc/gpio_num.h"

static const char *GPIO_TAG = "GPIO";

shtc3_t tempSensor;
i2c_master_bus_handle_t bus_handle;

void init_i2c(void) {
 uint16_t id;
i2c_master_bus_config_t i2c_bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = 8,
        .sda_io_num = 10,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, &bus_handle));

   shtc3_init(&tempSensor, bus_handle, 0x70);
}

static void blink_each_second_callback(void *arg) {
    gpio_set_level(GPIO_NUM_7, 1);
    ESP_LOGI(GPIO_TAG, "set to Vcc");
    vTaskDelay(pdMS_TO_TICKS(300));
    gpio_set_level(GPIO_NUM_7, 0);
    ESP_LOGI(GPIO_TAG, "set to Gnd");
}

void app_main(void)
{
    printf("Hello world!\n");

    /* Print chip information */
    esp_chip_info_t chip_info;
    uint32_t flash_size;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU core(s), %s%s%s%s, ",
           CONFIG_IDF_TARGET,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi/" : "",
           (chip_info.features & CHIP_FEATURE_BT) ? "BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "BLE" : "",
           (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");

    unsigned major_rev = chip_info.revision / 100;
    unsigned minor_rev = chip_info.revision % 100;
    printf("silicon revision v%d.%d, ", major_rev, minor_rev);
    if(esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
        printf("Get flash size failed");
        return;
    }

    printf("%" PRIu32 "MB %s flash\n", flash_size / (uint32_t)(1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("Minimum free heap size: %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());
    init_i2c();
    float temp;
    float hum;
    shtc3_get_temp_and_hum(&tempSensor, &temp, &hum);
    printf("The temperature is %.2f and the humidity is %.2f\n", temp, hum);

    const gpio_config_t led_gpio_config = {
        .pin_bit_mask = (1ULL << GPIO_NUM_7),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    ESP_ERROR_CHECK(gpio_config(&led_gpio_config));

    const esp_timer_create_args_t blink_each_second_args = {
        .callback = &blink_each_second_callback,
        .name = "blink_each_second",
    };

    esp_timer_handle_t blink_each_second;
    ESP_ERROR_CHECK(esp_timer_create(&blink_each_second_args, &blink_each_second));
    ESP_ERROR_CHECK(esp_timer_start_periodic(blink_each_second, 1000000));
    for (int i = 0; i < 5; ++i) {
        ESP_ERROR_CHECK(esp_timer_dump(stdout));
        usleep(2000000);
    }

    fflush(stdout);
}
