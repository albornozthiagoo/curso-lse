# include "board.h"

//Etiquetas
#define LED_BLUE  GPIO<1,1
#define delay(t)  for(uint32_t i = 0; i < t; i++); 

int main(void) {
    //inicializo puerto 1
    GPIO_PortInit(GPIO, 1);
    //inicializo pin como salida
    gpio_pin_config_t out_config = { .pinDirection = kGPIO_DigitalOutput, .outputLogic = 1 };
    GPIO_PinInit(GPIO,1,1, &out_config);

    while(1) {
        GPIO_PinWrite(LED_BLUE, !GPIO_PinRead(LED_BLUE));
        delay (5000)

    }
    return 0;
}