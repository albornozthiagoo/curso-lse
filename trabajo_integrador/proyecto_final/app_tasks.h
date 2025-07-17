#ifndef _APP_TASKS_H_
#define _APP_TASKS_H_

#include "board.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "wrappers.h"
#include "isr.h"

typedef enum {
    kDISPLAY_LUZ,
    kDISPLAY_SETPOINT
} display_variable_t;

// Prioridades de tareas

#define tskINIT_PRIORITY						(tskIDLE_PRIORITY + 3UL)
#define tskADC_PRIORITY							(tskIDLE_PRIORITY + 1UL)
#define tskDISPLAY_CHANGE_PRIORITY	(tskIDLE_PRIORITY + 2UL)
#define tskCONTROL_PRIORITY					(tskIDLE_PRIORITY + 1UL)
#define tskDISPLAY_PRIORITY					(tskIDLE_PRIORITY + 1UL)
#define tskPWM_PRIORITY							(tskIDLE_PRIORITY + 1UL)
#define tskBH1750_PRIORITY					(tskIDLE_PRIORITY + 1UL)
#define tskANIMATION_PRIORITY				(tskIDLE_PRIORITY + 1UL)
#define tskBUZZER_PRIORITY					(tskIDLE_PRIORITY + 2UL)
#define tskCOUNTER_BTNS_PRIORITY		(tskIDLE_PRIORITY + 2UL)
#define tskSETPOINT_PRIORITY (tskIDLE_PRIORITY + 2UL)
#define tskCNY70_PRIORITY (tskIDLE_PRIORITY + 1UL)
#define tsk_leds_tricolor_PRIORITY (tskIDLE_PRIORITY + 1UL)


// Stacks para tareas

#define tskINIT_STACK						(3 * configMINIMAL_STACK_SIZE)
#define tskADC_STACK						(configMINIMAL_STACK_SIZE)
#define tskDISPLAY_CHANGE_STACK	(configMINIMAL_STACK_SIZE)
#define tskCONTROL_STACK				(configMINIMAL_STACK_SIZE)
#define tskDISPLAY_STACK				(3 * configMINIMAL_STACK_SIZE / 2)
#define tskPWM_STACK						(3 * configMINIMAL_STACK_SIZE / 2)
#define tskBH1750_STACK					(configMINIMAL_STACK_SIZE)
#define tskANIMATION_STACK			(3 * configMINIMAL_STACK_SIZE / 2)
#define tskBUZZER_STACK					(configMINIMAL_STACK_SIZE)
#define tskCOUNTER_BTNS_STACK		(configMINIMAL_STACK_SIZE)
#define tskSETPOINT_STACK    (configMINIMAL_STACK_SIZE)
#define tskCNY70_STACK    (configMINIMAL_STACK_SIZE)
#define tsk_leds_tricotrol_STACK (configMINIMAL_STACK_SIZE)


// Handler para la tarea de display write
extern TaskHandle_t handle_display;

// Prototipos de funciones
void tarea_inic(void *params);
void tarea_adc(void *params);
void tarea_display_change(void *params);
void tarea_control(void *params);
void tarea_display(void *params);
void tarea_pwm(void *params);
void tarea_bh1750(void *params);
void tarea_animation(void *params);
void tarea_buzzer(void *params);
void tarea_counter_btns(void *params);
void tarea_setpoint(void *params);
void tarea_cny70(void *params);
void tarea_leds_tricolor(void *params);

/**
 * @brief Wrapper que verifica el estado de un pulsador con pull-up
 * aplicando un antirebote
 * @param btn estructura al GPIO del pulsador
 */
static inline bool wrapper_btn_get_with_debouncing_with_pull_up(gpio_t btn) {
	//	Pregunto si se precionó el pulsador
	if(!wrapper_btn_get(btn)) {
		// Antirebote
		vTaskDelay(pdMS_TO_TICKS(20));
		if(!wrapper_btn_get(btn)) {
			return true;
		}
	}
	return false;
}

#endif