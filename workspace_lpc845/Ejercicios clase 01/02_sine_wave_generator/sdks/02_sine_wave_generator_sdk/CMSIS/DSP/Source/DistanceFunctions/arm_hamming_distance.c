
/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_hamming_distance.c
 * Description:  Hamming distance between two vectors
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

#include "dsp/distance_functions.h"
#include <limits.h>
#include <math.h>


extern void arm_boolean_distance_TF_FT(const uint32_t *pA
       , const uint32_t *pB
       , uint32_t numberOfBools
       , uint32_t *cTF
       , uint32_t *cFT
       );

/**
  @addtogroup BoolDist
  @{
 */


/**
 * @brief        Hamming distance between two vectors
 *
 * @param[in]    pA              First vector of packed booleans
 * @param[in]    pB              Second vector of packed booleans
 * @param[in]    numberOfBools   Number of booleans
 * @return distance
 *
 */

ARM_DSP_ATTRIBUTE float32_t arm_hamming_distance(const uint32_t *pA, const uint32_t *pB, uint32_t numberOfBools)
{
    uint32_t ctf=0,cft=0;

    arm_boolean_distance_TF_FT(pA, pB, numberOfBools, &ctf, &cft);

    return(1.0*(ctf + cft) / numberOfBools);
}


/**
 * @} end of BoolDist group
 */
