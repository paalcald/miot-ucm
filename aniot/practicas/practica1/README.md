---
title: Práctica 1. Introducción al entorno.
author: Pablo C. Alcalde
...

# Primera Parte

## Cuestiones

> - ¿Qué versión es la actualmente estable de ESP-IDF?

La versión 5.3.1

> + ¿Cuál es la salida estándar por defecto de nuestro proyecto?

El bus de monitorización.

>+ ¿En qué unidades debemos expresar el argumento de `vTaskDelay()`? Investiga

Milisegundos

## Cuestiones
+ ¿Por qué no parpadea el LED de la placa? ¿Dónde está conectado ese LED? (compruébalo en los esquemáticos placa DevKitC-v4)

Por que estamos intentando hacer parpadear el pin 5, que no tiene el led conectado, en la placa en cuestión el led está conectado al pin 7.

+ Cambia la frecuencia de parpadeo usando `menuconfig`. Compila de nuevo y comprueba que el cambio ha surtido efecto.

Si, surte efecto.

+ Conecta la placa con el ESP32 a un LED del entrenador del laboratorio. ¿Puedes usar cualquier pin de la placa?

Salvo pines reservados para otras funciones que no se recomienda su uso ya que puede causar malfuncionamiento.

# Segunda parte

## Tarea

- Si no tienes un usuario Github, créalo.
- Realiza una operación de fork sobre el repositorio ESP-IDF de Espressif sobre tu usuario.
- Clónalo en tu máquina (puedes usar línea de comandos o bien la herramienta Github Desktop).
- Crea el fichero .github/workflows/idf_build.yml con el contenido anterior.
- Sube los cambios al repositorio (`git commit` + `git push`) y observa, en la pestaña Actions de tu repositorio, que efectivamente se ha construido con éxito el artifact resultado de la compilación.
- Adapta el trabajo para otro ejemplo (elige tú mismo/a el ejemplo) y consigue una imagen compilada para cualquiera de tus placas. ¿Serías capaz de descargarla y flashearla manualmente en la placa (necesitarás determinar qué comandos son necesarios para un proceso de flasheo manual observando la salida de VSCode para dicho proceso)?
- Proporciona al profesor un enlace al repositorio creado para que pueda revisar la correcta compilación del proyecto.

Se adjunta el repositorio solicitado github.com/paalcald/esp-idf y se puntualiza que el binario que facilita no es un elf válido y no puede ser flasheado sin instalar la herramienta de compilación, por lo que tenemos que tener el entorno esp-idf instalado de manera local de todos modos.

## Tarea

>Compila y estudia el ejemplo system/heap_task_tracking de ESP-IDF. Observa el uso de las anteriores funciones. Moficia el código para que la cantidad de memoria reservada en cada iteración de la tarea sea constante. Prueba a eliminar la invocación a free cada cierto número de iteraciones, para observar cómo va agotándose el espacio libre en el heap. Intenta observar la fragmentación en el heap volviendo a hacer reservas de tamaño aleatorias, y liberaciones sólo cada cierto número de iteraciones.

Se modificó para que no siempre hiciese el `free()`
```c
static void example_task(void *args)
{
    uint32_t iter = 0;
    ...
    while (1) {
	...
        if (iter % 2 == 0) {
            free(ptr);
        }
    ...
}
```

Se modificó el tamaño de cada reserva de memoria y se obtuvo el [siguiente log](./heap_task_tracking/monitor.log).
## Tarea

>Modifica el anterior ejemplo para que se invoque a tus hooks de información en cada reserva y liberación de memoria, y que se aporte en el mensaje que se mostrará por pantalla información sobre la misma.

```c
static size_t last_alloc_size = 0;
static void* last_alloc_loc = 0;
static void* last_free_loc = 0;
static void esp_dump_per_task_heap_info(void)
{
...
    printf("Last Allocation at %p of size %d\n", last_alloc_loc, last_alloc_size);
...
}

void esp_heap_trace_alloc_hook(void* ptr, size_t size, uint32_t caps) {
    last_alloc_size = size;
    last_alloc_loc = ptr;
}

void esp_heap_trace_free_hook(void* ptr) {
    last_free_loc = ptr;
}
```

## Tarea

>Estudia el código asociado a la tarea, así como la salida proporcionada en cada caso. Observa cómo se utiliza la API para seleccionar qué contadores hardware se desean reportar. Apóyate en la descripción de la API proporcionada en la documentación oficial para ayudarte en la tarea. Por último, modifica la función de ejemplo para implementar algún tipo de funcionalidad avanzada (por ejemplo, que incluya operaciones aritméticas), y observa los cambios en la salida. Experimenta con distintas opciones de compilador para optimizacón (a través de menuconfig), para ver si puedes obtener cambios significativos en los valores reportados (adicionalmente, puedes observar el impacto de estos flags en el tamaño del ejecutable).

Es una herramienta muy util a la hora de observar como podemos mejorar el código y cuanto repercute en nuestro código cada elección de algoritmo y abstracción.
