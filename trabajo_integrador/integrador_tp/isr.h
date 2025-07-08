#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

#include "wrappers.h"

// Semaforos y colas externas

extern xQueueHandle cola_adc;
extern xSemaphoreHandle semaforo_buzzer;
extern xSemaphoreHandle semaforo_usr;
extern xSemaphoreHandle semaforo_touch;

// Prototipos de callbacks

void cny70_callback(pint_pin_int_t pintr, uint32_t pmatch_status);
void usr_callback(pint_pin_int_t pintr, uint32_t pmatch_status);