---
title: Práctica 5. El protocolo MQTT (I y II). Despliegue de clientes y servidores/brokers. Análisis de tráfico. Despliegue de clientes en el ESP32
author: Pablo C. Alcalde
...
# Descripción
Este proyecto está realizado partiendo de la base del [ejemplo](~/esp/esp-idf/examples/protocols/mqtt/tcp) de mqtt sobre tcp proporcionado por esp32.

Se ha modificado mqtt event handler para gestionar los endpoints requeridos, comprobando el endpoint y extrayendo los datos correspondientes.

Del mismo modo se usan 4 timers y se utiliza el mismo timer y 4 flags para mantener el estado de inicialización de los diferentes sensores.

Se controlan los casos de que esté el timer activo y ningún sensor activado así como que esté algún sensor activado pero el timer apagado tras cada modificación.

Del mismo modo se empezó a trabajar en una manera de simplificar el codigo, que se adjunta como componente para gestionar sensores, sin embargo dicho componente no se ha terminado.
