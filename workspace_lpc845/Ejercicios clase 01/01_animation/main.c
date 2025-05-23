#include <stdint.h>
#include <stdio.h>
#include "board.h"

// Mapeo de segmentos: A, B, C, D, E, F, G
//      --B--
//     |     |
//     F     A
//     |     |
//      --G--
//     |     |
//     E     C
//     |     |
//      --D--

#define LED_A GPIO, 0, 11
#define LED_B GPIO, 0, 10
#define LED_C GPIO, 0, 6
#define LED_D GPIO, 0, 14
#define LED_E GPIO, 0, 0
#define LED_F GPIO, 0, 13
#define LED_G GPIO, 0, 15

// Cada bit representa un segmento
const uint8_t digit_to_segments[10] = {
    0b00111111, // 0: A B C D E F
    0b00000101, // 1: A C
    0b01011011, // 2: A B D E
    0b01001111, // 3: A B C D G
    0b01100101, // 4: B C G F
    0b01101110, // 5: A C D F G
    0b01111110, // 6: A C D E F G
    0b00000111, // 7: A B
    0b01111111, // 8: A B C D E F G
    0b01101111  // 9: A B C D F G
};

void display_digit(int digit)
{
    if (digit < 0 || digit > 9)
        return;
    uint8_t seg = digit_to_segments[digit];

    // Encender los segmentos correspondientes
    GPIO_PinWrite(LED_A, !((seg >> 0) & 1));
    GPIO_PinWrite(LED_B, !((seg >> 1) & 1));
    GPIO_PinWrite(LED_C, !((seg >> 2) & 1));
    GPIO_PinWrite(LED_D, !((seg >> 3) & 1));
    GPIO_PinWrite(LED_E, !((seg >> 4) & 1));
    GPIO_PinWrite(LED_F, !((seg >> 5) & 1));
    GPIO_PinWrite(LED_G, !((seg >> 6) & 1));

    // Activa solo el dígito izquierdo (A1)
    GPIO_PinWrite(GPIO, 0, 8, 1); // A1 en bajo (activo)
    GPIO_PinWrite(GPIO, 0, 9, 1); // A2 en alto (apagado)
}

int main(void)
{
    // Encender los pines correspondientes   
    GPIO_PortInit(GPIO, 0);
    gpio_pin_config_t out_config = {.pinDirection = kGPIO_DigitalOutput, .outputLogic = 0};

    // Inicializo Pin como Salida
    GPIO_PinInit(GPIO, 0, 11, &out_config);
    GPIO_PinInit(GPIO, 0, 10, &out_config);
    GPIO_PinInit(GPIO, 0, 6, &out_config);
    GPIO_PinInit(GPIO, 0, 14, &out_config);
    GPIO_PinInit(GPIO, 0, 0, &out_config);
    GPIO_PinInit(GPIO, 0, 13, &out_config);
    GPIO_PinInit(GPIO, 0, 15, &out_config);

    // Inicializo Pin como Salida
    GPIO_PinInit(GPIO, 0, 8, &out_config); // A1
    GPIO_PinInit(GPIO, 0, 9, &out_config); // A2

    while(1)
    {
        for (int i = 0; i < 10; i++)
        {
            display_digit(i);
            for (volatile int d = 0; d < 300000; d++); // Retardo simple
        }
    }
}