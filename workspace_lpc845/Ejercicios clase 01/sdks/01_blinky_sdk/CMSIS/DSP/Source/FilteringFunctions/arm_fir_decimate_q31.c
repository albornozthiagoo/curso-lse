/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_fir_decimate_q31.c
 * Description:  Q31 FIR Decimator
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

#include "dsp/filtering_functions.h"

/**
  @ingroup groupFilters
 */

/**
  @addtogroup FIR_decimate
  @{
 */

/**
  @brief         Processing function for the Q31 FIR decimator.
  @param[in]     S          points to an instance of the Q31 FIR decimator structure
  @param[in]     pSrc       points to the block of input data
  @param[out]    pDst       points to the block of output data
  @param[in]     blockSize  number of input samples to process

  @par           Scaling and Overflow Behavior
                   The function is implemented using an internal 64-bit accumulator.
                   The accumulator has a 2.62 format and maintains full precision of the intermediate multiplication results but provides only a single guard bit.
                   Thus, if the accumulator result overflows it wraps around rather than clip.
                   In order to avoid overflows completely the input signal must be scaled down by log2(numTaps) bits (where log2 is read as log to the base 2).
                   After all multiply-accumulates are performed, the 2.62 accumulator is truncated to 1.32 format and then saturated to 1.31 format.

 @remark
                   Refer to \ref arm_fir_decimate_fast_q31() for a faster but less precise implementation of this function.
 */

#if defined(ARM_MATH_MVEI) && !defined(ARM_MATH_AUTOVECTORIZE)

#include "arm_helium_utils.h"

ARM_DSP_ATTRIBUTE void arm_fir_decimate_q31(
  const arm_fir_decimate_instance_q31 * S,
  const q31_t * pSrc,
        q31_t * pDst,
        uint32_t blockSize)
{
    q31_t    *pState = S->pState;   /* State pointer */
    const q31_t    *pCoeffs = S->pCoeffs; /* Coefficient pointer */
    q31_t    *pStateCurnt;      /* Points to the current sample of the state */
    const q31_t    *px, *pb;          /* Temporary pointers for state and coefficient buffers */
    uint32_t  numTaps = S->numTaps; /* Number of filter coefficients in the filter */
    uint32_t  i, tapCnt, blkCnt, outBlockSize = blockSize / S->M;   /* Loop counters */
    uint32_t  blkCntN4;
    const q31_t    *px0, *px1, *px2, *px3;
    q63_t     acc0v, acc1v, acc2v, acc3v;
    q31x4_t x0v, x1v, x2v, x3v;
    q31x4_t c0v;

    /*
     * S->pState buffer contains previous frame (numTaps - 1) samples
     * pStateCurnt points to the location where the new input data should be written
     */
    pStateCurnt = S->pState + (numTaps - 1U);
    /*
     * Total number of output samples to be computed
     */
    blkCnt = outBlockSize / 4;
    blkCntN4 = outBlockSize - (4 * blkCnt);

    while (blkCnt > 0U)
    {
        /*
         * Copy 4 * decimation factor number of new input samples into the state buffer
         */
        i = (4 * S->M) >> 2;
        do
        {
            vst1q(pStateCurnt, vldrwq_s32(pSrc));
            pSrc += 4;
            pStateCurnt += 4;
            i--;
        }
        while (i > 0U);

        /*
         * Clear all accumulators
         */
        acc0v = 0LL;
        acc1v = 0LL;
        acc2v = 0LL;
        acc3v = 0LL;
        /*
         * Initialize state pointer for all the samples
         */
        px0 = pState;
        px1 = pState + S->M;
        px2 = pState + 2 * S->M;
        px3 = pState + 3 * S->M;
        /*
         * Initialize coeff. pointer
         */
        pb = pCoeffs;
        /*
         * Loop unrolling.  Process 4 taps at a time.
         */
        tapCnt = numTaps >> 2;
        /*
         * Loop over the number of taps.  Unroll by a factor of 4.
         * Repeat until we've computed numTaps-4 coefficients.
         */
        while (tapCnt > 0U)
        {
            /*
             * Read the b[numTaps-1] coefficient
             */
            c0v = vldrwq_s32(pb);
            pb += 4;
            /*
             * Read x[n-numTaps-1] sample for acc0
             */
            x0v = vld1q(px0);
            x1v = vld1q(px1);
            x2v = vld1q(px2);
            x3v = vld1q(px3);
            px0 += 4;
            px1 += 4;
            px2 += 4;
            px3 += 4;

            acc0v = vrmlaldavhaq(acc0v, x0v, c0v);
            acc1v = vrmlaldavhaq(acc1v, x1v, c0v);
            acc2v = vrmlaldavhaq(acc2v, x2v, c0v);
            acc3v = vrmlaldavhaq(acc3v, x3v, c0v);
            /*
             * Decrement the loop counter
             */
            tapCnt--;
        }

        /*
         * If the filter length is not a multiple of 4, compute the remaining filter taps
         * should be tail predicated
         */
        tapCnt = numTaps % 0x4U;
        if (tapCnt > 0U)
        {
            mve_pred16_t p0 = vctp32q(tapCnt);
            /*
             * Read the b[numTaps-1] coefficient
             */
            c0v = vldrwq_z_s32(pb, p0);
            pb += 4;
            /*
             * Read x[n-numTaps-1] sample for acc0
             */
            x0v = vld1q(px0);
            x1v = vld1q(px1);
            x2v = vld1q(px2);
            x3v = vld1q(px3);
            px0 += 4;
            px1 += 4;
            px2 += 4;
            px3 += 4;

            acc0v = vrmlaldavhaq(acc0v, x0v, c0v);
            acc1v = vrmlaldavhaq(acc1v, x1v, c0v);
            acc2v = vrmlaldavhaq(acc2v, x2v, c0v);
            acc3v = vrmlaldavhaq(acc3v, x3v, c0v);
        }

        acc0v = asrl(acc0v, 31 - 8);
        acc1v = asrl(acc1v, 31 - 8);
        acc2v = asrl(acc2v, 31 - 8);
        acc3v = asrl(acc3v, 31 - 8);
        /*
         * store in the destination buffer.
         */
        *pDst++ = (q31_t) acc0v;
        *pDst++ = (q31_t) acc1v;
        *pDst++ = (q31_t) acc2v;
        *pDst++ = (q31_t) acc3v;

        /*
         * Advance the state pointer by the decimation factor
         * to process the next group of decimation factor number samples
         */
        pState = pState + 4 * S->M;
        /*
         * Decrement the loop counter
         */
        blkCnt--;
    }

    while (blkCntN4 > 0U)
    {
        /*
         * Copy decimation factor number of new input samples into the state buffer
         */
        i = S->M;
        do
        {
            *pStateCurnt++ = *pSrc++;
        }
        while (--i);
        /*
         * Set accumulator to zero
         */
        acc0v = 0LL;
        /*
         * Initialize state pointer
         */
        px = pState;
        /*
         * Initialize coeff. pointer
         */
        pb = pCoeffs;
        /*
         * Loop unrolling.  Process 4 taps at a time.
         */
        tapCnt = numTaps >> 2;
        /*
         * Loop over the number of taps.  Unroll by a factor of 4.
         * Repeat until we've computed numTaps-4 coefficients.
         */
        while (tapCnt > 0U)
        {
            c0v = vldrwq_s32(pb);
            x0v = vldrwq_s32(px);
            pb += 4;
            px += 4;
            acc0v = vrmlaldavhaq(acc0v, x0v, c0v);
            /*
             * Decrement the loop counter
             */
            tapCnt--;
        }
        tapCnt = numTaps % 0x4U;
        if (tapCnt > 0U)
        {
            mve_pred16_t p0 = vctp32q(tapCnt);
            c0v = vldrwq_z_s32(pb, p0);
            x0v = vldrwq_z_s32(px, p0);
            acc0v = vrmlaldavhaq_p(acc0v, x0v, c0v, p0);
        }
        acc0v = asrl(acc0v, 31 - 8);

        /*
         * Advance the state pointer by the decimation factor
         * * to process the next group of decimation factor number samples
         */
        pState = pState + S->M;
        /*
         * The result is in the accumulator, store in the destination buffer.
         */
        *pDst++ = (q31_t) acc0v;
        /*
         * Decrement the loop counter
         */
        blkCntN4--;
    }

    /*
     * Processing is complete.
     * Now copy the last numTaps - 1 samples to the start of the state buffer.
     * This prepares the state buffer for the next function call.
     */
    pStateCurnt = S->pState;
    blkCnt = (numTaps - 1) >> 2;
    while (blkCnt > 0U)
    {
        vst1q(pStateCurnt, vldrwq_s32(pState));
        pState += 4;
        pStateCurnt += 4;
        blkCnt--;
    }
    blkCnt = (numTaps - 1) & 3;
    if (blkCnt > 0U)
    {
        mve_pred16_t p0 = vctp32q(blkCnt);
        vstrwq_p_s32(pStateCurnt, vldrwq_s32(pState), p0);
    }
}
#else
ARM_DSP_ATTRIBUTE void arm_fir_decimate_q31(
  const arm_fir_decimate_instance_q31 * S,
  const q31_t * pSrc,
        q31_t * pDst,
        uint32_t blockSize)
{
        q31_t *pState = S->pState;                     /* State pointer */
  const q31_t *pCoeffs = S->pCoeffs;                   /* Coefficient pointer */
        q31_t *pStateCur;                              /* Points to the current sample of the state */
        q31_t *px0;                                    /* Temporary pointer for state buffer */
  const q31_t *pb;                                     /* Temporary pointer for coefficient buffer */
        q31_t x0, c0;                                  /* Temporary variables to hold state and coefficient values */
        q63_t acc0;                                    /* Accumulator */
        uint32_t numTaps = S->numTaps;                 /* Number of filter coefficients in the filter */
        uint32_t i, tapCnt, blkCnt, outBlockSize = blockSize / S->M;  /* Loop counters */

#if defined (ARM_MATH_LOOPUNROLL)
        q31_t *px1, *px2, *px3;
        q31_t x1, x2, x3;
        q63_t acc1, acc2, acc3;
#endif

  /* S->pState buffer contains previous frame (numTaps - 1) samples */
  /* pStateCur points to the location where the new input data should be written */
  pStateCur = S->pState + (numTaps - 1U);

#if defined (ARM_MATH_LOOPUNROLL)

    /* Loop unrolling: Compute 4 samples at a time */
  blkCnt = outBlockSize >> 2U;

  /* Samples loop unrolled by 4 */
  while (blkCnt > 0U)
  {
    /* Copy 4 * decimation factor number of new input samples into the state buffer */
    i = S->M * 4;

    do
    {
      *pStateCur++ = *pSrc++;

    } while (--i);

    /* Set accumulators to zero */
    acc0 = 0;
    acc1 = 0;
    acc2 = 0;
    acc3 = 0;

    /* Initialize state pointer for all the samples */
    px0 = pState;
    px1 = pState + S->M;
    px2 = pState + 2 * S->M;
    px3 = pState + 3 * S->M;

    /* Initialize coeff pointer */
    pb = pCoeffs;

    /* Loop unrolling: Compute 4 taps at a time */
    tapCnt = numTaps >> 2U;

    while (tapCnt > 0U)
    {
      /* Read the b[numTaps-1] coefficient */
      c0 = *(pb++);

      /* Read x[n-numTaps-1] sample for acc0 */
      x0 = *(px0++);
      /* Read x[n-numTaps-1] sample for acc1 */
      x1 = *(px1++);
      /* Read x[n-numTaps-1] sample for acc2 */
      x2 = *(px2++);
      /* Read x[n-numTaps-1] sample for acc3 */
      x3 = *(px3++);

      /* Perform the multiply-accumulate */
      acc0 += (q63_t) x0 * c0;
      acc1 += (q63_t) x1 * c0;
      acc2 += (q63_t) x2 * c0;
      acc3 += (q63_t) x3 * c0;

      /* Read the b[numTaps-2] coefficient */
      c0 = *(pb++);

      /* Read x[n-numTaps-2] sample for acc0, acc1, acc2, acc3 */
      x0 = *(px0++);
      x1 = *(px1++);
      x2 = *(px2++);
      x3 = *(px3++);

      /* Perform the multiply-accumulate */
      acc0 += (q63_t) x0 * c0;
      acc1 += (q63_t) x1 * c0;
      acc2 += (q63_t) x2 * c0;
      acc3 += (q63_t) x3 * c0;

      /* Read the b[numTaps-3] coefficient */
      c0 = *(pb++);

      /* Read x[n-numTaps-3] sample acc0, acc1, acc2, acc3 */
      x0 = *(px0++);
      x1 = *(px1++);
      x2 = *(px2++);
      x3 = *(px3++);

      /* Perform the multiply-accumulate */
      acc0 += (q63_t) x0 * c0;
      acc1 += (q63_t) x1 * c0;
      acc2 += (q63_t) x2 * c0;
      acc3 += (q63_t) x3 * c0;

      /* Read the b[numTaps-4] coefficient */
      c0 = *(pb++);

      /* Read x[n-numTaps-4] sample acc0, acc1, acc2, acc3 */
      x0 = *(px0++);
      x1 = *(px1++);
      x2 = *(px2++);
      x3 = *(px3++);

      /* Perform the multiply-accumulate */
      acc0 += (q63_t) x0 * c0;
      acc1 += (q63_t) x1 * c0;
      acc2 += (q63_t) x2 * c0;
      acc3 += (q63_t) x3 * c0;

      /* Decrement loop counter */
      tapCnt--;
    }

    /* Loop unrolling: Compute remaining taps */
    tapCnt = numTaps % 0x4U;

    while (tapCnt > 0U)
    {
      /* Read coefficients */
      c0 = *(pb++);

      /* Fetch state variables for acc0, acc1, acc2, acc3 */
      x0 = *(px0++);
      x1 = *(px1++);
      x2 = *(px2++);
      x3 = *(px3++);

      /* Perform the multiply-accumulate */
      acc0 += (q63_t) x0 * c0;
      acc1 += (q63_t) x1 * c0;
      acc2 += (q63_t) x2 * c0;
      acc3 += (q63_t) x3 * c0;

      /* Decrement loop counter */
      tapCnt--;
    }

    /* Advance the state pointer by the decimation factor
     * to process the next group of decimation factor number samples */
    pState = pState + S->M * 4;

    /* The result is in the accumulator, store in the destination buffer. */
    *pDst++ = (q31_t) (acc0 >> 31);
    *pDst++ = (q31_t) (acc1 >> 31);
    *pDst++ = (q31_t) (acc2 >> 31);
    *pDst++ = (q31_t) (acc3 >> 31);

    /* Decrement loop counter */
    blkCnt--;
  }

  /* Loop unrolling: Compute remaining samples */
  blkCnt = outBlockSize % 0x4U;

#else

  /* Initialize blkCnt with number of samples */
  blkCnt = outBlockSize;

#endif /* #if defined (ARM_MATH_LOOPUNROLL) */

  while (blkCnt > 0U)
  {
    /* Copy decimation factor number of new input samples into the state buffer */
    i = S->M;

    do
    {
      *pStateCur++ = *pSrc++;

    } while (--i);

    /* Set accumulator to zero */
    acc0 = 0;

    /* Initialize state pointer */
    px0 = pState;

    /* Initialize coeff pointer */
    pb = pCoeffs;

#if defined (ARM_MATH_LOOPUNROLL)

    /* Loop unrolling: Compute 4 taps at a time */
    tapCnt = numTaps >> 2U;

    while (tapCnt > 0U)
    {
      /* Read the b[numTaps-1] coefficient */
      c0 = *pb++;

      /* Read x[n-numTaps-1] sample */
      x0 = *px0++;

      /* Perform the multiply-accumulate */
      acc0 += (q63_t) x0 * c0;

      /* Read the b[numTaps-2] coefficient */
      c0 = *pb++;

      /* Read x[n-numTaps-2] sample */
      x0 = *px0++;

      /* Perform the multiply-accumulate */
      acc0 += (q63_t) x0 * c0;

      /* Read the b[numTaps-3] coefficient */
      c0 = *pb++;

      /* Read x[n-numTaps-3] sample */
      x0 = *px0++;

      /* Perform the multiply-accumulate */
      acc0 += (q63_t) x0 * c0;

      /* Read the b[numTaps-4] coefficient */
      c0 = *pb++;

      /* Read x[n-numTaps-4] sample */
      x0 = *px0++;

      /* Perform the multiply-accumulate */
      acc0 += (q63_t) x0 * c0;

      /* Decrement loop counter */
      tapCnt--;
    }

    /* Loop unrolling: Compute remaining taps */
    tapCnt = numTaps % 0x4U;

#else

    /* Initialize tapCnt with number of taps */
    tapCnt = numTaps;

#endif /* #if defined (ARM_MATH_LOOPUNROLL) */

    while (tapCnt > 0U)
    {
      /* Read coefficients */
      c0 = *pb++;

      /* Fetch 1 state variable */
      x0 = *px0++;

      /* Perform the multiply-accumulate */
      acc0 += (q63_t) x0 * c0;

      /* Decrement loop counter */
      tapCnt--;
    }

    /* Advance the state pointer by the decimation factor
     * to process the next group of decimation factor number samples */
    pState = pState + S->M;

    /* The result is in the accumulator, store in the destination buffer. */
    *pDst++ = (q31_t) (acc0 >> 31);

    /* Decrement loop counter */
    blkCnt--;
  }

  /* Processing is complete.
     Now copy the last numTaps - 1 samples to the satrt of the state buffer.
     This prepares the state buffer for the next function call. */

  /* Points to the start of the state buffer */
  pStateCur = S->pState;

#if defined (ARM_MATH_LOOPUNROLL)

  /* Loop unrolling: Compute 4 taps at a time */
  tapCnt = (numTaps - 1U) >> 2U;

  /* Copy data */
  while (tapCnt > 0U)
  {
    *pStateCur++ = *pState++;
    *pStateCur++ = *pState++;
    *pStateCur++ = *pState++;
    *pStateCur++ = *pState++;

    /* Decrement loop counter */
    tapCnt--;
  }

  /* Loop unrolling: Compute remaining taps */
  tapCnt = (numTaps - 1U) % 0x04U;

#else

  /* Initialize tapCnt with number of taps */
  tapCnt = (numTaps - 1U);

#endif /* #if defined (ARM_MATH_LOOPUNROLL) */

  /* Copy data */
  while (tapCnt > 0U)
  {
    *pStateCur++ = *pState++;

    /* Decrement loop counter */
    tapCnt--;
  }

}
#endif /* defined(ARM_MATH_MVEI) */
/**
  @} end of FIR_decimate group
 */
