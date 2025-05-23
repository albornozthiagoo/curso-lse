#include "board.h" 

// Define un alias para acceder fácilmente al pin del LED 
#define LED_BLUE GPIO, 1, 1

// Define un macro para generar una demora 
#define delay(t) for (uint32_t i = 0; i < t; i++)

int main (void) {

    // Inicializa el Puerto 1 
    GPIO_PortInit(GPIO, 1);

    // Configura el pin como salida digital
    gpio_pin_config_t out_config = { .pinDirection = kGPIO_DigitalOutput, .outputLogic = 1};

    // Inicializa el pin del LED azul como salida con la configuración establecida
    GPIO_PinInit(GPIO, 1, 1, &out_config);

    while(1) {

        GPIO_PinWrite(LED_BLUE, 1);  // Enciende el LED azul 
        delay(150000);               // Espera un tiempo 
        GPIO_PinWrite(LED_BLUE, 0);  // Apaga el LED azul 
        delay(150000);               // Espera otro tiempo antes de volver a encender

    }


    return 0; 

}