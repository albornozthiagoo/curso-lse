#include "board.h"
#include "FreeRTOS.h"
#include "task.h"

#include "tareas_app.h"

// Programa principal
int main(void) {
	// Clock del sistema a 30 MHz
	BOARD_BootClockFRO30M();

	// Creacion de tareas
	xTaskCreate(tarea_inic, "Init", tskINIT_STACK, NULL, tskINIT_PRIORITY, NULL);
	xTaskCreate(tarea_adc, "ADC", tskADC_STACK, NULL, tskADC_PRIORITY, NULL);
	xTaskCreate(tarea_display_change, "Button", tskDISPLAY_CHANGE_STACK, NULL, tskDISPLAY_CHANGE_PRIORITY, NULL);
	xTaskCreate(tarea_control, "Write", tskCONTROL_STACK, NULL, tskCONTROL_PRIORITY, NULL);
	xTaskCreate(tarea_display, "Display", tskDISPLAY_STACK, NULL, tskDISPLAY_PRIORITY, &handle_display);
	xTaskCreate(tarea_pwm, "PWM", tskPWM_STACK, NULL, tskPWM_PRIORITY, NULL);
	xTaskCreate(tarea_bh1750, "BH1750", tskBH1750_STACK, NULL, tskBH1750_PRIORITY, NULL);
	xTaskCreate(tarea_animation, "Animation", tskANIMATION_STACK, NULL, tskANIMATION_PRIORITY, NULL);
	xTaskCreate(tarea_blinky, "Blinky LED", tskBLINKY_STACK, NULL, tskBLINKY_PRIORITY, NULL);
	xTaskCreate(tarea_buzzer, "Buzzer", tskBUZZER_STACK, NULL, tskBUZZER_PRIORITY, NULL);
	xTaskCreate(tarea_contador, "Counter", tskCOUNTER_STACK, NULL, tskCOUNTER_PRIORITY, NULL);
	xTaskCreate(tarea_contador_btns, "Counter Btns", tskCOUNTER_BTNS_STACK, NULL,tskCOUNTER_BTNS_PRIORITY, NULL);

	vTaskStartScheduler();
}