/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_dct4_init_f32.c
 * Description:  Initialization function of DCT-4 & IDCT4 F32
 *
 * $Date:        23 April 2021
 * $Revision:    V1.9.0
 *
 * Target Processor: Cortex-M and Cortex-A cores
 * -------------------------------------------------------------------- */
/*
 * Copyright (C) 2010-2021 ARM Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "dsp/transform_functions.h"
#include "arm_common_tables.h"

/**
 * @defgroup DCT4F32 DCT4 F32
 */

/**
  @ingroup DCT4_IDCT4
 */

 /**
  @addtogroup DCT4F32
  @{
 */

/**
  @brief         Initialization function for the floating-point DCT4/IDCT4.
  @deprecated    Do not use this function. It is using a deprecated version of the RFFT.
  @param[in,out] S          points to an instance of floating-point DCT4/IDCT4 structure
  @param[in]     S_RFFT     points to an instance of floating-point RFFT/RIFFT structure
  @param[in]     S_CFFT     points to an instance of floating-point CFFT/CIFFT structure
  @param[in]     N			length of the DCT4
  @param[in]     Nby2       half of the length of the DCT4
  @param[in]     normalize  normalizing factor.
  @return        execution status
                   - \ref ARM_MATH_SUCCESS        : Operation successful
                   - \ref ARM_MATH_ARGUMENT_ERROR : <code>N</code> is not a supported transform length

  @par           Normalizing factor
                   The normalizing factor is <code>sqrt(2/N)</code>, which depends on the size of transform <code>N</code>.
                   Floating-point normalizing factors are mentioned in the table below for different DCT sizes:

 
| DCT Size  | Normalizing factor value  | 
| --------: | ------------------------: | 
| 2048      | 0.03125                   | 
| 512       | 0.0625                    | 
| 128       | 0.125                     | 

 */

ARM_DSP_ATTRIBUTE arm_status arm_dct4_init_f32(
  arm_dct4_instance_f32 * S,
  arm_rfft_instance_f32 * S_RFFT,
  arm_cfft_radix4_instance_f32 * S_CFFT,
  uint16_t N,
  uint16_t Nby2,
  float32_t normalize)
{
  /* Initialize the default arm status */
  arm_status status = ARM_MATH_SUCCESS;


  /* Initialize the DCT4 length */
  S->N = N;

  /* Initialize the half of DCT4 length */
  S->Nby2 = Nby2;

  /* Initialize the DCT4 Normalizing factor */
  S->normalize = normalize;

  /* Initialize Real FFT Instance */
  S->pRfft = S_RFFT;

  /* Initialize Complex FFT Instance */
  S->pCfft = S_CFFT;

  switch (N)
  {
    /* Initialize the table modifier values */
  case 8192U:
    S->pTwiddle = Weights_8192;
    S->pCosFactor = cos_factors_8192;
    break;

  case 2048U:
    S->pTwiddle = Weights_2048;
    S->pCosFactor = cos_factors_2048;
    break;

  case 512U:
    S->pTwiddle = Weights_512;
    S->pCosFactor = cos_factors_512;
    break;

  case 128U:
    S->pTwiddle = Weights_128;
    S->pCosFactor = cos_factors_128;
    break;
  default:
    status = ARM_MATH_ARGUMENT_ERROR;
  }

  /* Initialize the RFFT/RIFFT Function */
  arm_rfft_init_f32(S->pRfft, S->pCfft, S->N, 0U, 1U);

  /* return the status of DCT4 Init function */
  return (status);
}

/**
  @} end of DCT4F32 group
 */
