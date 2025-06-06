/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_mult_q7.c
 * Description:  Q7 vector multiplication
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

#include "dsp/basic_math_functions.h"

/**
  @ingroup groupMath
 */

/**
  @addtogroup BasicMult
  @{
 */

/**
  @brief         Q7 vector multiplication
  @param[in]     pSrcA      points to the first input vector
  @param[in]     pSrcB      points to the second input vector
  @param[out]    pDst       points to the output vector
  @param[in]     blockSize  number of samples in each vector

  @par           Scaling and Overflow Behavior
                   The function uses saturating arithmetic.
                   Results outside of the allowable Q7 range [0x80 0x7F] are saturated.
 */
#if defined(ARM_MATH_MVEI) && !defined(ARM_MATH_AUTOVECTORIZE)

#include "arm_helium_utils.h"

ARM_DSP_ATTRIBUTE void arm_mult_q7(
    const q7_t * pSrcA,
    const q7_t * pSrcB,
    q7_t * pDst,
    uint32_t blockSize)
{
    uint32_t  blkCnt;           /* loop counters */
    q7x16_t vecA, vecB;

    /* Compute 16 outputs at a time */
    blkCnt = blockSize >> 4;
    while (blkCnt > 0U)
    {
        /*
         * C = A * B
         * Multiply the inputs and then store the results in the destination buffer.
         */
        vecA = vld1q(pSrcA);
        vecB = vld1q(pSrcB);
        vst1q(pDst, vqdmulhq(vecA, vecB));
        /*
         * Decrement the blockSize loop counter
         */
        blkCnt--;
        /*
         * advance vector source and destination pointers
         */
        pSrcA  += 16;
        pSrcB  += 16;
        pDst   += 16;
    }
    /*
     * tail
     */
    blkCnt = blockSize & 0xF;
    if (blkCnt > 0U)
    {
        mve_pred16_t p0 = vctp8q(blkCnt);
        vecA = vld1q(pSrcA);
        vecB = vld1q(pSrcB);
        vstrbq_p(pDst, vqdmulhq(vecA, vecB), p0);
    }
}

#else
ARM_DSP_ATTRIBUTE void arm_mult_q7(
  const q7_t * pSrcA,
  const q7_t * pSrcB,
        q7_t * pDst,
        uint32_t blockSize)
{
        uint32_t blkCnt;                               /* Loop counter */

#if defined (ARM_MATH_LOOPUNROLL)

#if defined (ARM_MATH_DSP)
  q7_t out1, out2, out3, out4;                   /* Temporary output variables */
#endif

  /* Loop unrolling: Compute 4 outputs at a time */
  blkCnt = blockSize >> 2U;

  while (blkCnt > 0U)
  {
    /* C = A * B */

#if defined (ARM_MATH_DSP)
    /* Multiply inputs and store results in temporary variables */
    out1 = (q7_t) __SSAT((((q15_t) (*pSrcA++) * (*pSrcB++)) >> 7), 8);
    out2 = (q7_t) __SSAT((((q15_t) (*pSrcA++) * (*pSrcB++)) >> 7), 8);
    out3 = (q7_t) __SSAT((((q15_t) (*pSrcA++) * (*pSrcB++)) >> 7), 8);
    out4 = (q7_t) __SSAT((((q15_t) (*pSrcA++) * (*pSrcB++)) >> 7), 8);

    /* Pack and store result in destination buffer (in single write) */
    write_q7x4_ia (&pDst, __PACKq7(out1, out2, out3, out4));
#else
    *pDst++ = (q7_t) __SSAT((((q15_t) (*pSrcA++) * (*pSrcB++)) >> 7), 8);
    *pDst++ = (q7_t) __SSAT((((q15_t) (*pSrcA++) * (*pSrcB++)) >> 7), 8);
    *pDst++ = (q7_t) __SSAT((((q15_t) (*pSrcA++) * (*pSrcB++)) >> 7), 8);
    *pDst++ = (q7_t) __SSAT((((q15_t) (*pSrcA++) * (*pSrcB++)) >> 7), 8);
#endif

    /* Decrement loop counter */
    blkCnt--;
  }

  /* Loop unrolling: Compute remaining outputs */
  blkCnt = blockSize % 0x4U;

#else

  /* Initialize blkCnt with number of samples */
  blkCnt = blockSize;

#endif /* #if defined (ARM_MATH_LOOPUNROLL) */

  while (blkCnt > 0U)
  {
    /* C = A * B */

    /* Multiply input and store result in destination buffer. */
    *pDst++ = (q7_t) __SSAT((((q15_t) (*pSrcA++) * (*pSrcB++)) >> 7), 8);

    /* Decrement loop counter */
    blkCnt--;
  }

}
#endif /* defined(ARM_MATH_MVEI) */

/**
  @} end of BasicMult group
 */
