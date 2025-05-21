# include "board.h"

int main(void) {
    //inicializo puerto 1
    GPIO_PortInit(GPIO, 1);
    //inicializo pin como salida
    gpio_pin_config_t out_config = { .pinDirection = kGPIO_DigitalOutput, .outputLogic = 1 };
    GPIO_PinInit(GPIO,1,1, &out_config);

    while(1) {
        GPIO_PinWrite(GPIO,1,1,1);
        for(uint32_t i = 0; i < 50000; i++);
        GPIO_PinWrite(GPIO, 1,1,0);
        for(uint32_t i = 0; i < 50000; i++);

    }
    return 0;
}