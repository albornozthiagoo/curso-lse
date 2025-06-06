/*
 * Copyright 2020 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/***********************************************************************************************************************
 * This file was generated by the MCUXpresso Config Tools. Any manual edits made to this file
 * will be overwritten if the respective MCUXpresso Config Tools is used to update this file.
 **********************************************************************************************************************/

/* clang-format off */
/* TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
!!GlobalInfo
product: Peripherals v8.0
processor: LPC845
package_id: LPC845M301JBD48
mcu_data: ksdk2_0
processor_version: 0.0.2
board: LPC845BREAKOUT
functionalGroups:
- name: BOARD_InitPeripherals
  UUID: 5e519eed-bd94-4950-84de-0f29de85bcea
  called_from_default_init: true
  selectedCore: core0
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS **********/

/* TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
component:
- type: 'system'
- type_id: 'system_54b53072540eeeb8f8e9343e71f28176'
- global_system_definitions:
  - user_definitions: ''
  - user_includes: ''
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS **********/
/* clang-format on */

/***********************************************************************************************************************
 * Included files
 **********************************************************************************************************************/
#include "peripherals.h"

/***********************************************************************************************************************
 * BOARD_InitPeripherals functional group
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * FreeMASTER initialization code
 **********************************************************************************************************************/
/* clang-format off */
/* TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
instance:
- name: 'FreeMASTER'
- type: 'freemaster'
- mode: 'general'
- custom_name_enabled: 'false'
- type_id: 'freemaster_14894d49d87e02d0dca5f799e22554da'
- functional_group: 'BOARD_InitPeripherals'
- config_sets:
  - freemaster_config:
    - FMSTR_DISABLE: 'false'
    - initFreemaster: 'true'
    - generalConfig:
      - commInterface: 'serial'
      - interruptMode: 'polling'
      - initType: 'false'
      - serialConfig:
        - peripheralUART: 'USART0'
        - clockSource: 'genericUartClockSource'
        - clockSourceFreq: 'BOARD_BootClockRUN'
        - baudRate_Bps: '115200'
        - parityMode_usart: 'kUSART_ParityDisabled'
        - stopBitCount_usart: 'kUSART_OneStopBit'
        - bitCountPerChar: 'kUSART_8BitsPerChar'
        - enable_rx_tx_irq: 'true'
        - interrupt_rx_tx:
          - IRQn: 'USART0_IRQn'
          - enable_interrrupt: 'enabled'
          - enable_priority: 'false'
          - priority: '0'
          - useDefaultISR: 'true'
      - FMSTR_COMM_BUFFER_SIZE: 'autoSize'
      - FMSTR_COMM_RQUEUE_SIZE: '32'
      - FMSTR_USE_SCOPE: '2'
      - FMSTR_MAX_SCOPE_VARS: '8'
      - FMSTR_REC_FLOAT_TRIG: 'false'
      - recorders:
        - 0:
          - recID:
            - enableCustomID: 'false'
          - nameRec: 'Default Recorder'
          - recBuff:
            - customBuffer: 'false'
            - bufferSize: '1024'
          - basePeriod_ns: '0'
          - recInit: 'true'
        - 1:
          - recID:
            - enableCustomID: 'false'
          - nameRec: 'Alternate Recorder for test purposes'
          - recBuff:
            - customBuffer: 'false'
            - bufferSize: '512'
          - basePeriod_ns: '0'
          - recInit: 'true'
      - FMSTR_USE_TSA: 'true'
      - FMSTR_USE_TSA_INROM: 'true'
      - FMSTR_USE_TSA_SAFETY: 'true'
      - FMSTR_USE_TSA_DYNAMIC: 'false'
      - pipes:
        - 0:
          - pipeID:
            - enableCustomID: 'false'
          - namePipe: 'Simple echoing terminal pipe 1'
          - pipePort: '1'
          - enablePipeHandler: 'true'
          - callbackFunction: 'my_pipe_handler'
          - rxBuffer:
            - customBuffer: 'false'
            - bufferSize: '10'
          - txBuffer:
            - customBuffer: 'false'
            - bufferSize: '128'
          - pipeInit: 'true'
        - 1:
          - pipeID:
            - enableCustomID: 'false'
          - namePipe: 'Simple echoing terminal pipe 2'
          - pipePort: '2'
          - enablePipeHandler: 'true'
          - callbackFunction: 'my_pipe_handler'
          - rxBuffer:
            - customBuffer: 'false'
            - bufferSize: '10'
          - txBuffer:
            - customBuffer: 'false'
            - bufferSize: '128'
          - pipeInit: 'true'
        - 2:
          - pipeID:
            - enableCustomID: 'false'
          - namePipe: 'Binary multiplication pipe at port 33'
          - pipePort: '33'
          - enablePipeHandler: 'true'
          - callbackFunction: 'my_pipe_math'
          - rxBuffer:
            - customBuffer: 'false'
            - bufferSize: '8'
          - txBuffer:
            - customBuffer: 'false'
            - bufferSize: '4'
          - pipeInit: 'true'
      - FMSTR_USE_READMEM: 'true'
      - FMSTR_USE_WRITEMEM: 'true'
      - FMSTR_USE_WRITEMEMMASK: 'true'
      - passR: ''
      - passRW: ''
      - passRWF: ''
      - FMSTR_USE_HASHED_PASSWORDS: 'false'
      - FMSTR_USE_APPCMD: 'true'
      - FMSTR_APPCMD_BUFF_SIZE: '32'
      - FMSTR_MAX_APPCMD_CALLS: '4'
    - freemaster_codegenerator: []
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS **********/
/* clang-format on */
/* USART configuration structure */
usart_config_t FreeMASTER_serial_config;
/* Recorder0 configuration */
FMSTR_U8 FreeMASTER_RecBuffer0[FREEMASTER_REC_0_SIZE];
/* Recorder1 configuration */
FMSTR_U8 FreeMASTER_RecBuffer1[FREEMASTER_REC_1_SIZE];
/* Pipe0 handle */
FMSTR_HPIPE FreeMASTER_Pipe_handle_1;
/* Pipe0 Rx buffer */
FMSTR_U8 FreeMASTER_RxPipe1[FREEMASTER_RX_PIPE_1_SIZE];
/* Pipe0 Tx buffer */
FMSTR_U8 FreeMASTER_TxPipe1[FREEMASTER_TX_PIPE_1_SIZE];
/* Pipe1 handle */
FMSTR_HPIPE FreeMASTER_Pipe_handle_2;
/* Pipe1 Rx buffer */
FMSTR_U8 FreeMASTER_RxPipe2[FREEMASTER_RX_PIPE_2_SIZE];
/* Pipe1 Tx buffer */
FMSTR_U8 FreeMASTER_TxPipe2[FREEMASTER_TX_PIPE_2_SIZE];
/* Pipe2 handle */
FMSTR_HPIPE FreeMASTER_Pipe_handle_3;
/* Pipe2 Rx buffer */
FMSTR_U8 FreeMASTER_RxPipe3[FREEMASTER_RX_PIPE_3_SIZE];
/* Pipe2 Tx buffer */
FMSTR_U8 FreeMASTER_TxPipe3[FREEMASTER_TX_PIPE_3_SIZE];

FMSTR_REC_BUFF FreeMASTER_Recorder_0 = {
  .name = "Default Recorder",
  .addr = (FMSTR_ADDR)FreeMASTER_RecBuffer0,
  .size = FREEMASTER_REC_0_SIZE,
  .basePeriod_ns = 0UL
};
FMSTR_REC_BUFF FreeMASTER_Recorder_1 = {
  .name = "Alternate Recorder for test purposes",
  .addr = (FMSTR_ADDR)FreeMASTER_RecBuffer1,
  .size = FREEMASTER_REC_1_SIZE,
  .basePeriod_ns = 0UL
};

static void FreeMASTER_init(void) {
  /* Serial communication peripheral initialization */

  /* Get USART default configuration and modify it */
  USART_GetDefaultConfig(&FreeMASTER_serial_config);

  FreeMASTER_serial_config.baudRate_Bps = FREEMASTER_SERIAL_PERIPHERAL_BAUDRATE;
  FreeMASTER_serial_config.parityMode = kUSART_ParityDisabled;
  FreeMASTER_serial_config.stopBitCount = kUSART_OneStopBit;
  FreeMASTER_serial_config.bitCountPerChar = kUSART_8BitsPerChar;

  /* Initialize USART module. */
  USART_Init(FREEMASTER_SERIAL_PERIPHERAL, &FreeMASTER_serial_config, FREEMASTER_SERIAL_PERIPHERAL_CLK_FREQ);
  /* FreeMASTER middleware initialization */
  FMSTR_Init();
  /* FreeMASTER recorder 0 configuration initialization  */
  FMSTR_RecorderCreate(0, &FreeMASTER_Recorder_0);
  /* FreeMASTER recorder 1 configuration initialization  */
  FMSTR_RecorderCreate(1, &FreeMASTER_Recorder_1);
  /* FreeMASTER pipe 1 initialization  */
  FreeMASTER_Pipe_handle_1 = FMSTR_PipeOpen(1, &my_pipe_handler, (FMSTR_ADDR)FreeMASTER_RxPipe1, FREEMASTER_RX_PIPE_1_SIZE,
      (FMSTR_ADDR)FreeMASTER_TxPipe1, FREEMASTER_TX_PIPE_1_SIZE, 0, "Simple echoing terminal pipe 1");
  /* FreeMASTER pipe 2 initialization  */
  FreeMASTER_Pipe_handle_2 = FMSTR_PipeOpen(2, &my_pipe_handler, (FMSTR_ADDR)FreeMASTER_RxPipe2, FREEMASTER_RX_PIPE_2_SIZE,
      (FMSTR_ADDR)FreeMASTER_TxPipe2, FREEMASTER_TX_PIPE_2_SIZE, 0, "Simple echoing terminal pipe 2");
  /* FreeMASTER pipe 3 initialization  */
  FreeMASTER_Pipe_handle_3 = FMSTR_PipeOpen(33, &my_pipe_math, (FMSTR_ADDR)FreeMASTER_RxPipe3, FREEMASTER_RX_PIPE_3_SIZE,
      (FMSTR_ADDR)FreeMASTER_TxPipe3, FREEMASTER_TX_PIPE_3_SIZE, 0, "Binary multiplication pipe at port 33");
}

/***********************************************************************************************************************
 * Initialization functions
 **********************************************************************************************************************/
void BOARD_InitPeripherals(void)
{
  /* Initialize components */
  FreeMASTER_init();
}

/***********************************************************************************************************************
 * BOARD_InitBootPeripherals function
 **********************************************************************************************************************/
void BOARD_InitBootPeripherals(void)
{
  BOARD_InitPeripherals();
}
