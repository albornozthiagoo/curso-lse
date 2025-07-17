#include "board.h"
#include "FreeRTOS.h"
#include "task.h"

#include "app_tasks.h"

/**
 * @brief Programa principal
 */
int main(void) {
	// Clock del sistema a 30 MHz
	BOARD_BootClockFRO30M();

	// Creación de tareas
	xTaskCreate(tarea_inic, "Init", tskINIT_STACK, NULL, tskINIT_PRIORITY, NULL);
	xTaskCreate(tarea_adc, "ADC", tskADC_STACK, NULL, tskADC_PRIORITY, NULL);
	xTaskCreate(tarea_display_change, "Button", tskDISPLAY_CHANGE_STACK, NULL, tskDISPLAY_CHANGE_PRIORITY, NULL);
	xTaskCreate(tarea_control, "Write", tskCONTROL_STACK, NULL, tskCONTROL_PRIORITY, NULL);
	xTaskCreate(tarea_display, "Display", tskDISPLAY_STACK, NULL, tskDISPLAY_PRIORITY, &handle_display);
	xTaskCreate(tarea_pwm, "PWM", tskPWM_STACK, NULL, tskPWM_PRIORITY, NULL);
	xTaskCreate(tarea_bh1750, "BH1750", tskBH1750_STACK, NULL, tskBH1750_PRIORITY, NULL);
	xTaskCreate(tarea_buzzer, "Buzzer", tskBUZZER_STACK, NULL, tskBUZZER_PRIORITY, NULL);
	xTaskCreate(tarea_counter_btns, "Counter Btns", tskCOUNTER_BTNS_STACK, NULL,tskCOUNTER_BTNS_PRIORITY, NULL);
	xTaskCreate(tarea_setpoint, "Setpoint", tskSETPOINT_STACK, NULL, tskSETPOINT_PRIORITY, NULL);
	xTaskCreate(tarea_cny70, "CNY70", tskCNY70_STACK, NULL, tskCNY70_PRIORITY, NULL);
	xTaskCreate(tarea_leds_tricolor, "LEDs", tsk_leds_tricotrol_STACK, NULL, tsk_leds_tricolor_PRIORITY, NULL);
	vTaskStartScheduler();
}