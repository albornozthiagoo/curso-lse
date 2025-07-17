# Trabajo Integrador - Aplicaciones de Sistemas Operativos en Tiempo Real

## Descripción

Este proyecto fue desarrollado en el marco de la materia **Aplicaciones de Sistemas Operativos en Tiempo Real** de la **Universidad Tecnológica Nacional – Facultad Regional Avellaneda**, dictada por el profesor **Fabrizio Carlassara**.

El trabajo consiste en una aplicación embebida basada en **FreeRTOS** utilizando un **microcontrolador LPC845-BRK**, diseñada para aplicar conceptos clave de programación multitarea, control de periféricos y sincronización de recursos.

##  Integrantes

Albornoz Thiago, Poggi Lorenzo, Sarniguette Valentino

## Stack Tech 

- Lenguaje de programación: **C**
- Sistema operativo en tiempo real: **FreeRTOS**
- Microcontrolador: **NXP LPC845 Breakout**
- SDK: **LPC845 SDK**
- IDE: **MCUXpresso IDE**


## Estructura del Proyecto

- `labels.h`: Definición de etiquetas y constantes.
- `wrappers.c / wrappers.h`: Encapsulan y simplifican el uso del SDK.
- `app_task.c / app_task.h`: Definición de tareas, prioridades, colas y semáforos.
- `main.c`: Inicialización del sistema, creación de tareas, inicio del scheduler.


## Tareas Implementadas

| Tarea | Función |
|-------|---------|
| `tarea_setpoint` | Ajusta el setpoint de luminosidad con S1 y S2 (25% a 75%) |
| `tarea_adc` | Lanza conversiones ADC cada 0.25s |
| `tarea_display_change` | Lee botones para definir qué valor mostrar |
| `tarea_control` | Controla el contenido del display |
| `tarea_display` | Escribe valores en el display de 7 segmentos |
| `tarea_pwm` | Actualiza el duty del PWM |
| `tarea_bh1750` | Lee el sensor de luz BH1750 |
| `tarea_buzzer` | Activa el buzzer |
| `tarea_counter_btns` | Control manual del contador |
| `tarea_cny70` | Detecta movimiento con sensor infrarrojo |
| `tarea_leds_tricolor` | Muestra colores según luminosidad vs. setpoint |


## Hardware Utilizado

| Componente | Descripción |
|------------|-------------|
| LED Azul | Indicador visual simple |
| LED Tricolor | Indicador RGB controlado por condiciones |
| Sensor CNY70 | Sensor de infrarrojo/reflexión |
| Sensor BH1750 | Sensor de intensidad lumínica I2C |
| Buzzer | Señal acústica |
| Display 7 segmentos | Salida visual |
| 3 Botones | Inputs: USER, S1, S2 |
| 2 Potenciómetros | Entradas analógicas para testeo |


## Conclusiones

El proyecto permitió integrar conocimientos de sistemas embebidos, FreeRTOS, sincronización mediante semáforos/colas y manejo multitarea. Se mejoraron habilidades de programación en C, trabajo en equipo, y organización de código para sistemas reales.


## Referencias

- Repositorio UTN: [github.com/utn-fra-lse/lpc845](https://github.com/utn-fra-lse/lpc845)
- Video explicativo de FreeRTOS: [YouTube - Conceptos FreeRTOS](https://www.youtube.com/watch?v=s5DnmeOwkxo)


## Diagrama del Proyecto

A continuación se muestra el diagrama general del sistema:

![](diagrama.png)