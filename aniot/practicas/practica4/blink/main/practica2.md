# Práctica 2. Entorno de compilación. Uso de timers.

## Tarea
> La placa de desarrollo ESP32-C3-DevKit-RUST-1 tiene dos sensores de temperatura. Uno está integrado en el propio ESP32-C3 (puedes encontrar su API en la documentación de ESP-IDF). El segundo está en la placa, tal y como se indica en su documentación. Se trata de un sensor SHTC3 de Sensirion que está conectado al SoC mediante el bus I2C.
> - A partir del código disponible en este repositorio de GitHub https://github.com/mauriciobarroso/shtc3 crea un componente llamado shtc3 para poder utilizar el sensor sin necesidad de consultar el datasheet. Los ficheros del repositorio ya están preparados para usarse como un componente en ESP-IDF v5.3 Sólo debes ubicarlos en la carpeta correcta.
> - En el fichero principal de tu aplicación, deberás inicializar el driver del bus I2C y el propio sensor. Puedes usar el código que se proporciona a continuación.

Puesto que utilicé la version 5.4 del esp-idf fue necesario añadir al `CMakeLists.txt` de la carpeta `main` los ficheros añadidos como dependencias.
```cmake
idf_component_register(SRCS "hello_world_main.c"
                    PRIV_REQUIRES spi_flash shtc3
                    INCLUDE_DIRS "")
```

Por otro lado, en el main hicimos los siguientes insertos

```c
#include "driver/i2c_master.h"
#include "shtc3.h"

shtc3_t tempSensor;
i2c_master_bus_handle_t bus_handle;

void init_i2c(void) {
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
void app_main(void) {
    ...

    init_i2c();
    float temp;
    float hum;
    shtc3_get_temp_and_hum(&tempSensor, &temp, &hum);
    printf("The temperature is %.2f and the humidity is %.2f\n", temp, hum);
    ...
}
```

## Tarea
>En relación al código anterior:
> - ¿Por qué `scl_io_num` es 8? ¿ Por qué `sda_io_num` es 10? ¿Se pueden cambiar?

Según las especificaciones del ESP32-C3-DevKit-RUST-1 que estamos usando, este utiliza los siguientes puertos para la comunicación usando el protocolo *I2C*, el `GPIO10` como *SDA* y el `GPIO8` como *SCL*.

> - ¿Por qué la llamada a `shtc3_init()` recibe `0x70` como tercer argumento?

Para la comunicación por *I2C*, ya que múltiples dispositivos, conocidos como _slaves_, pueden estar en comunicación con un mismo dispositivo director, conocido como _master_, hemos de enviar, en primer lugar la dirección del dispositivo con el que tratamos de comunicarnos antes de mandar los datos que queramos comunicarle. En el caso del sensor SHTC3, esta dirección es `0x70`. Sin embargo, debido a que esto es una constante del dispositivo en cuestión, parece cuestionable en el diseño de la librería que sea necesario especificarla.

## Tarea
> Configura el GPIO asociado al LED como salida. Programa un _timer_ para cambiar el estado del LED cada segundo.

```c
// Definimos el callback que ejecutara cada segundo.
static void blink_each_second_callback(void *arg) {
    gpio_set_level(GPIO_NUM_7, 1);
    ESP_LOGI(GPIO_TAG, "set to Vcc");
    vTaskDelay(pdMS_TO_TICKS(300));
    gpio_set_level(GPIO_NUM_7, 0);
    ESP_LOGI(GPIO_TAG, "set to Gnd");
}
// Configuramos el pin
const gpio_config_t led_gpio_config = {
    .pin_bit_mask = (1ULL << GPIO_NUM_7),
    .mode = GPIO_MODE_OUTPUT,
    .pull_up_en = GPIO_PULLUP_ENABLE,
    .pull_down_en = GPIO_PULLDOWN_ENABLE,
    .intr_type = GPIO_INTR_DISABLE
};

ESP_ERROR_CHECK(gpio_config(&led_gpio_config));

// Configuramos nuestro timer y lo ejecutamos cada segundo.
const esp_timer_create_args_t blink_each_second_args = {
    .callback = &blink_each_second_callback,
    .name = "blink_each_second",
};

esp_timer_handle_t blink_each_second;
ESP_ERROR_CHECK(esp_timer_create(&blink_each_second_args, &blink_each_second));
ESP_ERROR_CHECK(esp_timer_start_periodic(blink_each_second, 1000000));
```

# Ejercicio final

## Tareas

>Partiendo del ejemplo Blink (usando el denominado LED_STRIP en GPIO 2, no como la usamos el primer día), crea una applicación que:
>
> + Incluya el componente shtc3 en tu proyecto.
Ya explicado anteriormente.
> + Muestree la temperatura cada segundo utilizando un timer.
Se crea el siguiente timer y se programa para ejecutarse cada segundo.

```c
...
static void periodic_temp_measurement_callback(void *args)
{
    shtc3_get_temp_and_hum(&tempSensor, &temp, &hum);
}
...

void app_main(void)
{
    const esp_timer_create_args_t periodic_temp_measurement_args = {
        .callback = &periodic_temp_measurement_callback,
        .name = "temp_measurement"};
    esp_timer_handle_t periodic_temp_measurement;
    ...
    ESP_ERROR_CHECK(esp_timer_create(&periodic_temp_measurement_args, &periodic_temp_measurement));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_temp_measurement, 1000000));
    ...
}
```


> + Muestre el progreso de la temperatura en el LED programable de la placa. Si la temperatura es inferior a 20 grados, estará apagado. Por cada grado que suba la temperatura, se modificará el color/intensidad del LED.

Se modifica la función del apartado anterior para incluir la representación de los datos en la misma, en este caso se hace por medio de la función facilitada en el ejemplo para interactuar con el LED integrado en la placa via GPIO.

Se incluye la funcionalidad de mapear estas temperaturas a colores, del siguiente modo:
+ $t < 20$ => (0,200,0)
+ $20 < t < 40$ => (x, 200 - x, 0) con $x$ dependiendo linearmente de la temperatura.
+ $40 < t$ => (200, 0, 0)

```c
struct rgb_t
{
    uint8_t state;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
};
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
static void periodic_temp_measurement_callback(void* args)
{
    rgb_from_temp(&rgb_val, temp);

    /* If the addressable LED is enabled */
    if (rgb_val.state)
    {
        /* Set the LED pixel using RGB from 0 (0%) to 255 (100%) for each color */
        led_strip_set_pixel(led_strip, 0, rgb_val.red, rgb_val.green, rgb_val.blue);
        /* Refresh the strip to send data */
        led_strip_refresh(led_strip);
    }
    else
    {
        /* Set all LED off to clear all pixels */
        led_strip_clear(led_strip);
    }
}
```
> + Se programará un segundo timer que mostrará por pantalla (puerto serie) la última medida de temperatura realizada cada 10 segundos.

Se añade, el timer en cuestión para mostrar la última medida por el puerto serie.

```c
static void periodic_temp_display_callback(void *args)
{
    ESP_LOGI(TEMP, "Temperature is %.2f", temp);
    ESP_LOGI(TEMP, "RGB at (%d, %d, %d)", rgb_val.red, rgb_val.green, rgb_val.blue);
}
void app_main(void) {
    ...
    const esp_timer_create_args_t periodic_temp_display_args = {
        .callback = &periodic_temp_display_callback,
        .name = "temp_measurement"};
    esp_timer_handle_t periodic_temp_display;
    ...
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_temp_display, 10000000));
    ...
}
```
> + [Opcional] Configura el GPIO 9, al que está conectado el botón BOOT, para que genere interrupciones cuando soltemos el botón. ¿Qué valor lógico se lee del GPIO 9 con el botón pulsado?. Consulta la documentación de GPIO y el ejemplo de GPIO genérico para entender cómo configurar un GPIO como entrada por interrupciones.

Como confirma los esquemas de la placa, el pin GPIO 9 está conectado a *Vcc*, al pulsar el botón este pasa a estar a *Gnd*, de este modo.

Configuramos el GPIO correspondiente al botón para generar una interrupción por falling edge, i.e. baja desde un valor de voltaje alto a uno bajo.

En nuestro ejemplo hemos decidido imprimir en pantalla la temperatura al recibir la interrupción, puesto que hemos de interactuar con los dispositivos de entrada/salida lo tendremos que hacer fuera de la interrupción, ya que debemos de limitar el tiempo de cómputo y el acceso a recursos bloqueantes de las interrupciones. Por esto usamos un semáforo para señalizar entre procesos.

```c
static SemaphoreHandle_t gpio_print_semaphore = NULL;
static void IRAM_ATTR gpio_interrupt_handler(void* args)
{
    xSemaphoreGiveFromISR(gpio_print_semaphore, NULL);
}

void display_task(void* arg) {
    for (;;) {
        if (xSemaphoreTake(gpio_print_semaphore, portMAX_DELAY)) {
            ESP_LOGI(TEMP, "Temperature is %.2f", temp);
        }
    }
}

void app_main(void) {
    ...
    const gpio_config_t boot_button_config = {
        .pin_bit_mask = (1ULL << GPIO_NUM_9),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    ESP_ERROR_CHECK(gpio_config(&boot_button_config));
    gpio_set_intr_type(GPIO_NUM_9, GPIO_INTR_NEGEDGE);
    gpio_print_semaphore = xSemaphoreCreateBinary();
    xTaskCreate(display_task, "gpio displayer", 2048, NULL, 10, NULL);
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(GPIO_NUM_9, gpio_interrupt_handler, (void *) GPIO_NUM_9);
    ...
}
```
