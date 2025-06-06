/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2019, 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_debug_console.h"
#include "board.h"
#include "app.h"
#include "fsl_sctimer.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief Main function
 */
int main(void)
{
    sctimer_config_t sctimerInfo;
    sctimer_pwm_signal_param_t pwmParam;
    uint32_t event;
    uint32_t sctimerClock;

    /* Board pin, clock, debug console init */
    BOARD_InitHardware();

    sctimerClock = SCTIMER_CLK_FREQ;

    /* Print a note to terminal */
    PRINTF("\r\nSCTimer example to output 2 center-aligned PWM signals\r\n");
    PRINTF("\r\nProbe the signal using an oscilloscope");

    SCTIMER_GetDefaultConfig(&sctimerInfo);

    /* Initialize SCTimer module */
    SCTIMER_Init(SCT0, &sctimerInfo);

    /* Configure first PWM with frequency 24kHZ from first output */
    pwmParam.output           = DEMO_FIRST_SCTIMER_OUT;
    pwmParam.level            = kSCTIMER_HighTrue;
    pwmParam.dutyCyclePercent = DUTY_CYCLE_CH1;
    if (SCTIMER_SetupPwm(SCT0, &pwmParam, PWM_MODE, 24000U, sctimerClock, &event) == kStatus_Fail)
    {
        return -1;
    }

    /* Configure second PWM with different duty cycle but same frequency as before */
    pwmParam.output           = DEMO_SECOND_SCTIMER_OUT;
    pwmParam.level            = kSCTIMER_LowTrue;
    pwmParam.dutyCyclePercent = DUTY_CYCLE_CH2;
    if (SCTIMER_SetupPwm(SCT0, &pwmParam, PWM_MODE, 24000U, sctimerClock, &event) == kStatus_Fail)
    {
        return -1;
    }

    /* Start the 32-bit unify timer */
    SCTIMER_StartTimer(SCT0, kSCTIMER_Counter_U);

    while (1)
    {
    }
}
