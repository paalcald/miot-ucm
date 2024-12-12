/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdint.h>
#include <stdio.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "hal/gpio_types.h"
#include "led_strip.h"
#include "portmacro.h"
#include "sdkconfig.h"
#include "shtc3.h"

static const char *TEMP = "temp";

/* Use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define BLINK_GPIO CONFIG_BLINK_GPIO
#define ESP_INTR_FLAG_DEFAULT 0

//static led_strip_handle_t led_strip;
//static SemaphoreHandle_t gpio_print_semaphore = NULL;
static char* TAG = "TEMP";

struct rgb_t
{
    uint8_t state;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
};
float temp;
float hum;
shtc3_t tempSensor;
i2c_master_bus_handle_t bus_handle;
// struct rgb_t rgb_val;

void init_i2c(void)
{
    i2c_master_bus_config_t i2c_bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = 22,
        .sda_io_num = 21,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, &bus_handle));

    ESP_ERROR_CHECK(shtc3_init(&tempSensor, bus_handle, 0x40));
}
void rgb_from_temp(struct rgb_t *rgb_val, float temp)
{
    uint8_t min_temp = 20;
    uint8_t max_temp = 40;
    uint8_t green_max = 200;
    uint8_t green_min = 0;
    uint8_t red_max = 200;
    uint8_t red_min = 0;
    float temp_validated = ((temp < max_temp && temp > min_temp) ? temp : (temp < min_temp ? min_temp : max_temp));
    float delta_green = (green_max - green_min) * (temp_validated - min_temp) / (max_temp - min_temp);
    float delta_red = (red_max - red_min) * (temp_validated - min_temp) / (max_temp - min_temp);
    rgb_val->green = green_max - delta_green;
    rgb_val->red = red_min + delta_red;
    rgb_val->state = (temp > 20) ? 1 : 0;
    return;
}

static void periodic_temp_measurement_callback(void *args)
{
    shtc3_get_temp_and_hum(&tempSensor, &temp, &hum);
    // rgb_from_temp(&rgb_val, temp);

    // /* If the addressable LED is enabled */
    // if (rgb_val.state)
    // {
    //     /* Set the LED pixel using RGB from 0 (0%) to 255 (100%) for each color */
    //     led_strip_set_pixel(led_strip, 0, rgb_val.red, rgb_val.green, rgb_val.blue);
    //     /* Refresh the strip to send data */
    //     led_strip_refresh(led_strip);
    // }
    // else
    // {
    //     /* Set all LED off to clear all pixels */
    //     led_strip_clear(led_strip);
    // }
}
static void periodic_temp_display_callback(void *args)
{
    ESP_LOGI(TEMP, "Temperature is %.2f", temp);
    // ESP_LOGI(TEMP, "RGB at (%d, %d, %d)", rgb_val.red, rgb_val.green, rgb_val.blue);
}

// static void configure_led(void)
// {
//     ESP_LOGI(TAG, "Example configured to blink addressable LED!");
//     /* LED strip initialization with the GPIO and pixels number*/
//     led_strip_config_t strip_config = {
//         .strip_gpio_num = BLINK_GPIO,
//         .max_leds = 1, // at least one LED on board
//     };
// #if CONFIG_BLINK_LED_STRIP_BACKEND_RMT
//     led_strip_rmt_config_t rmt_config = {
//         .resolution_hz = 10 * 1000 * 1000, // 10MHz
//         .flags.with_dma = false,
//     };
//     ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
// #elif CONFIG_BLINK_LED_STRIP_BACKEND_SPI
//     led_strip_spi_config_t spi_config = {
//         .spi_bus = SPI2_HOST,
//         .flags.with_dma = true,
//     };
//     ESP_ERROR_CHECK(led_strip_new_spi_device(&strip_config, &spi_config, &led_strip));
// #else
// #error "unsupported LED strip backend"
// #endif
//     /* Set all LED off to clear all pixels */
//     led_strip_clear(led_strip);
// }
// 
// #elif CONFIG_BLINK_LED_GPIO
// 
// static void blink_led(void)
// {
//     /* Set the GPIO level according to the state (LOW or HIGH)*/
//     gpio_set_level(BLINK_GPIO, s_led_state);
// }
// 
// static void configure_led(void)
// {
//     ESP_LOGI(TAG, "Example configured to blink GPIO LED!");
//     gpio_reset_pin(BLINK_GPIO);
//     /* Set the GPIO as a push/pull output */
//     gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
// }
// 
// #else
// #error "unsupported LED type"
// #endif
// static void IRAM_ATTR gpio_interrupt_handler(void* args)
// {
//     xSemaphoreGiveFromISR(gpio_print_semaphore, NULL);
// }

// void display_task(void* arg) {
//     for (;;) {
//         if (xSemaphoreTake(gpio_print_semaphore, portMAX_DELAY)) {
//             ESP_LOGI(TEMP, "Temperature is %.2f", temp);
//         }
//     }
// }

void app_main(void)
{
    const esp_timer_create_args_t periodic_temp_measurement_args = {
        .callback = &periodic_temp_measurement_callback,
        .name = "temp_measurement"};
    esp_timer_handle_t periodic_temp_measurement;
    const esp_timer_create_args_t periodic_temp_display_args = {
        .callback = &periodic_temp_display_callback,
        .name = "temp_measurement"};
    esp_timer_handle_t periodic_temp_display;
    init_i2c();

    // const gpio_config_t boot_button_config = {
    //     .pin_bit_mask = (1ULL << GPIO_NUM_9),
    //     .mode = GPIO_MODE_INPUT,
    //     .pull_up_en = GPIO_PULLUP_ENABLE,
    //     .pull_down_en = GPIO_PULLUP_DISABLE,
    //     .intr_type = GPIO_INTR_NEGEDGE,
    // };
    // ESP_ERROR_CHECK(gpio_config(&boot_button_config));
    // gpio_set_intr_type(GPIO_NUM_9, GPIO_INTR_NEGEDGE);
    // gpio_print_semaphore = xSemaphoreCreateBinary();
    // xTaskCreate(display_task, "gpio displayer", 2048, NULL, 10, NULL);
    // gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    // gpio_isr_handler_add(GPIO_NUM_9, gpio_interrupt_handler, (void *) GPIO_NUM_9);
    // /* Configure the peripheral according to the LED type */
    // configure_led();
    ESP_ERROR_CHECK(esp_timer_create(&periodic_temp_measurement_args, &periodic_temp_measurement));
    ESP_ERROR_CHECK(esp_timer_create(&periodic_temp_display_args, &periodic_temp_display));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_temp_measurement, 1000000));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_temp_display, 10000000));
}
