#include "board.h"
#include "pin_mux.h"
#include "fsl_dac.h"
#include "fsl_power.h"
#include "fsl_swm.h"
#include "fsl_iocon.h"
#include "fsl_clock.h"

int main(void)
{
    // Configurar la salida del DAC al PO 29
    CLOCK_EnableClock(kCLOCK_Swm);
    SWM_SetFixedPinSelect(SWM0, kSWM_DAC_OUT1, true);
    CLOCK_DisableClock(kCLOCK_Swm);

    // Habilito la función del DAC en el PO 29
    CLOCK_EnableClock(kCLOCK_Iocon);
    IOCON_PinMuxSet(IOCON, 1, IOCON_PIO_DACMODE_MASK);
    CLOCK_DisableClock(kCLOCK_Iocon);

    // Prendo el DAC
    POWER_DisablePD(kPDRUNCFG_PD_DAC1);

    // Configuro el DAC con 1us de refresco
    dac_config_t dacConfig = {kDAC_SettlingTimeIs1us};
    DAC_Init(DAC1, &dacConfig);

    // Offset 1.65V
    DAC_SetBufferValue(DAC1, 512);

    // Configuro SysTick para 31.25us
    SysTick_Config(SystemCoreClock / 32000);

    while (1)
        ;
    return 0;
}

// Tabla 32 Valores de la señal senoidal ajustada
const uint32_t values[32] = {
    512, 571, 629, 685, 739, 790, 837, 879,
    895, 879, 837, 790, 739, 685, 629, 571,
    512, 453, 395, 339, 285, 234, 187, 145,
    129, 145, 187, 234, 285, 339, 395, 453};

// Variable volatil para cambiar generar valores de señal senoidal
volatile uint8_t Wave = 0;

void SysTick_Handler(void)
{
    DAC_SetBufferValue(DAC1, values[Wave]);
    Wave++;
    if (Wave >= 32)
    {
        Wave = 0; // Reinicio la senoidal
    }
}