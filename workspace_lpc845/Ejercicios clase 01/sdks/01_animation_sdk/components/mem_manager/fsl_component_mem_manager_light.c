/*! *********************************************************************************
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2022, 2023 NXP
 *
 * \file
 *
 * This is the source file for the Memory Manager.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ********************************************************************************** */

/*! *********************************************************************************
*************************************************************************************
* Include
*************************************************************************************
********************************************************************************** */

#include "fsl_common.h"
#if defined(MEM_STATISTICS_INTERNAL) || defined(MEM_MANAGER_BENCH)
#include "fsl_component_timer_manager.h"
#include "fsl_component_mem_manager_internal.h"
#endif /* MEM_STATISTICS_INTERNAL MEM_MANAGER_BENCH*/
#include "fsl_component_mem_manager.h"
#if defined(gDebugConsoleEnable_d) && (gDebugConsoleEnable_d == 1)
#include "fsl_debug_console.h"
#endif

#if defined(gMemManagerLight) && (gMemManagerLight == 1)

/*  Selects the allocation scheme that will be used by the MemManagerLight
    0: Allocates the first free block available in the heap, no matter its size
    1: Allocates the first free block whose size is at most the double of the requested size
    2: Allocates the first free block whose size is at most the 4/3 of the requested size     */
#ifndef cMemManagerLightReuseFreeBlocks
#define cMemManagerLightReuseFreeBlocks 1
#endif

#if defined(cMemManagerLightReuseFreeBlocks) && (cMemManagerLightReuseFreeBlocks > 0)
/* Because a more restrictive on the size of the free blocks when cMemManagerLightReuseFreeBlocks
 *  is set, we need to enable a garbage collector to clean up the free block when possible .
 * When set gMemManagerLightFreeBlocksCleanUp is used to select between 2 policies:
 *  1: on each bufffer free, the allocator parses the free list in the forward direction and
 *     attempts to merge the freeed buffer with the top unused remainder of the region.
 *  2: In addition to behaviour described in 1, allocator parses the list backwards to merge
 *     previous contiguous members of the free list (free blocks) if adjacent to the last block.
 *     In this case they meld in the top of the unused region.
 */
#ifndef gMemManagerLightFreeBlocksCleanUp
#define gMemManagerLightFreeBlocksCleanUp 2
#endif
#endif

#ifndef gMemManagerLightGuardsCheckEnable
#define gMemManagerLightGuardsCheckEnable 0
#endif

/*! Extend Heap usage beyond the size defined by MinimalHeapSize_c up to __HEAP_end__ symbol address
 *   to make full use of the remaining available SRAM for the dynamic allocator. Also, only the data up to the
 *   highest allocated block will be retained by calling the @function MEM_GetHeapUpperLimit() from the power
 *   manager.
 *   When this flag is turned to 1 :
 *     -  __HEAP_end__ linker symbol shall be defined in linker script to be the highest allowed address
 *   used by the fsl_component_memory_manager_light
 *     - .heap section shall be defined and placed after bss and zi sections to make sure no data is located
 *   up to __HEAP_end__ symbol.
 *   @Warning, no data shall be placed after memHeap symbol address up to __HEAP_end__ . If an other
 *   memory allocator uses a memory area between __HEAP_start__ and __HEAP_end__, area may overlap
 *   with fsl_component_memory_manager_light, so this flag shall be kept to 0
 */
#ifndef gMemManagerLightExtendHeapAreaUsage
#define gMemManagerLightExtendHeapAreaUsage 0
#endif

/*! *********************************************************************************
*************************************************************************************
* Private macros
*************************************************************************************
********************************************************************************** */
#ifndef MAX_UINT16
#define MAX_UINT16 0x00010000U
#endif

#define MEMMANAGER_BLOCK_INVALID (uint16_t)0x0    /* Used to remove a block in the heap - debug only */
#define MEMMANAGER_BLOCK_FREE    (uint16_t)0xBA00 /* Mark a previous allocated block as free         */
#define MEMMANAGER_BLOCK_USED    (uint16_t)0xBABE /* Mark the block as allocated                     */

#define BLOCK_HDR_SIZE (ROUNDUP_WORD(sizeof(blockHeader_t)))

#define ROUNDUP_WORD(__x) (((((__x)-1U) & ~0x3U) + 4U) & 0XFFFFFFFFU)

#define BLOCK_HDR_PREGUARD_SIZE     28U
#define BLOCK_HDR_PREGUARD_PATTERN  0x28U
#define BLOCK_HDR_POSTGUARD_SIZE    28U
#define BLOCK_HDR_POSTGUARD_PATTERN 0x39U

#if defined(__IAR_SYSTEMS_ICC__)
#define __mem_get_LR() __get_LR()
#elif defined(__GNUC__)
#define __mem_get_LR() __builtin_return_address(0U)
#elif defined(__CC_ARM) || defined(__ARMCC_VERSION)
#define __mem_get_LR() __return_address()
#endif

#if defined(gMemManagerLightGuardsCheckEnable) && (gMemManagerLightGuardsCheckEnable == 1)
#define gMemManagerLightAddPreGuard  1
#define gMemManagerLightAddPostGuard 1
#endif

#ifndef gMemManagerLightAddPreGuard
#define gMemManagerLightAddPreGuard 0
#endif

#ifndef gMemManagerLightAddPostGuard
#define gMemManagerLightAddPostGuard 0
#endif

#if defined(__IAR_SYSTEMS_ICC__) && (defined __CORTEX_M) && \
    ((__CORTEX_M == 4U) || (__CORTEX_M == 7U) || (__CORTEX_M == 33U))
#define D_BARRIER __asm("DSB"); /* __DSB() could not be used */
#else
#define D_BARRIER
#endif
#define ENABLE_GLOBAL_IRQ(reg) \
    D_BARRIER;                 \
    EnableGlobalIRQ(reg)
#define KB(x) ((x) << 10u)

/************************************************************************************
*************************************************************************************
* Private type definitions
*************************************************************************************
************************************************************************************/

typedef struct blockHeader_s
{
#if defined(gMemManagerLightAddPreGuard) && (gMemManagerLightAddPreGuard == 1)
    uint8_t preguard[BLOCK_HDR_PREGUARD_SIZE];
#endif
    uint16_t used;
    uint8_t area_id;
    uint8_t reserved;
#if defined(MEM_STATISTICS_INTERNAL)
    uint16_t buff_size;
#endif
    struct blockHeader_s *next;
    struct blockHeader_s *next_free;
    struct blockHeader_s *prev_free;
#ifdef MEM_TRACKING
    void *first_alloc_caller;
    void *second_alloc_caller;
#endif
#if defined(gMemManagerLightAddPostGuard) && (gMemManagerLightAddPostGuard == 1)
    uint8_t postguard[BLOCK_HDR_POSTGUARD_SIZE];
#endif
} blockHeader_t;

typedef struct freeBlockHeaderList_s
{
    struct blockHeader_s *head;
    struct blockHeader_s *tail;
} freeBlockHeaderList_t;

typedef union void_ptr_tag
{
    uint32_t raw_address;
    uint32_t *address_ptr;
    void *void_ptr;
    blockHeader_t *block_hdr_ptr;
} void_ptr_t;
typedef struct _memAreaPriv_s
{
    freeBlockHeaderList_t FreeBlockHdrList;
#ifdef MEM_STATISTICS_INTERNAL
    mem_statis_t statistics;
#endif
} memAreaPriv_t;

typedef struct _mem_area_priv_desc_s
{
    memAreaCfg_t *next;       /*< Next registered RAM area descriptor. */
    void_ptr_t start_address; /*< Start address of RAM area. */
    void_ptr_t end_address;   /*< End address of registered RAM area. */
    uint16_t flags;           /*< BIT(0) means not member of default pool, other bits RFFU */
    uint16_t reserved;        /*< alignment padding */
    uint32_t low_watermark;
    union
    {
        uint8_t internal_ctx[MML_INTERNAL_STRUCT_SZ]; /* Placeholder for internal allocator data */
        memAreaPriv_t ctx;
    };
} memAreaPrivDesc_t;

/*! *********************************************************************************
*************************************************************************************
* Private memory declarations
*************************************************************************************
********************************************************************************** */

#ifndef MEMORY_POOL_GLOBAL_VARIABLE_ALLOC
/* Allocate memHeap array in the .heap section to ensure the size of the .heap section is large enough
   for the application
   However, the real heap used at run time will cover all the .heap section so this area can be bigger
   than the requested MinimalHeapSize_c - see memHeapEnd */
#if defined(__IAR_SYSTEMS_ICC__)
#pragma location = ".heap"
static uint32_t memHeap[MinimalHeapSize_c / sizeof(uint32_t)];
#elif defined(__CC_ARM) || defined(__ARMCC_VERSION)
static uint32_t memHeap[MinimalHeapSize_c / sizeof(uint32_t)] __attribute__((section(".heap")));
#elif defined(__GNUC__)
static uint32_t memHeap[MinimalHeapSize_c / sizeof(uint32_t)] __attribute__((section(".heap, \"aw\", %nobits @")));
#else
#error "Compiler unknown!"
#endif

#if defined(gMemManagerLightExtendHeapAreaUsage) && (gMemManagerLightExtendHeapAreaUsage == 1)
#if defined(__ARMCC_VERSION)
extern uint32_t Image$$ARM_LIB_STACK$$Base[];
static const uint32_t memHeapEnd = (uint32_t)&Image$$ARM_LIB_STACK$$Base;
#else
extern uint32_t __HEAP_end__[];
static const uint32_t memHeapEnd = (uint32_t)&__HEAP_end__;
#endif
#else
static const uint32_t memHeapEnd = (uint32_t)(memHeap + MinimalHeapSize_c / sizeof(uint32_t));
#endif

#else
extern uint32_t *memHeap;
extern uint32_t memHeapEnd;
#endif /* MEMORY_POOL_GLOBAL_VARIABLE_ALLOC */

static memAreaPrivDesc_t heap_area_list;

#ifdef MEM_STATISTICS_INTERNAL
static mem_statis_t s_memStatis;
#endif /* MEM_STATISTICS_INTERNAL */

#if defined(gFSCI_MemAllocTest_Enabled_d) && (gFSCI_MemAllocTest_Enabled_d)
extern mem_alloc_test_status_t FSCI_MemAllocTestCanAllocate(void *pCaller);
#endif

/*! *********************************************************************************
*************************************************************************************
* Private functions
*************************************************************************************
********************************************************************************** */

#ifdef MEM_STATISTICS_INTERNAL
static void MEM_Inits_memStatis(mem_statis_t *s_memStatis_)
{
    (void)memset(s_memStatis_, 0U, sizeof(mem_statis_t));
    SystemCoreClockUpdate();
}

static void MEM_BufferAllocates_memStatis(void *buffer, uint32_t time, uint32_t requestedSize)
{
    void_ptr_t buffer_ptr;
    void_ptr_t blockHdr_ptr;
    blockHeader_t *BlockHdr;

    /* Using union to fix Misra */
    buffer_ptr.void_ptr      = buffer;
    blockHdr_ptr.raw_address = buffer_ptr.raw_address - BLOCK_HDR_SIZE;
    BlockHdr                 = blockHdr_ptr.block_hdr_ptr;

    /* existing block must have a BlockHdr and a next BlockHdr */
    assert((BlockHdr != NULL) && (BlockHdr->next != NULL));

    s_memStatis.nb_alloc++;
    /* Sort the buffers by size, based on defined thresholds */
    if (requestedSize <= SMALL_BUFFER_SIZE)
    {
        s_memStatis.nb_small_buffer++;
        UPDATE_PEAK(s_memStatis.nb_small_buffer, s_memStatis.peak_small_buffer);
    }
    else if (requestedSize <= LARGE_BUFFER_SIZE)
    {
        s_memStatis.nb_medium_buffer++;
        UPDATE_PEAK(s_memStatis.nb_medium_buffer, s_memStatis.peak_medium_buffer);
    }
    else
    {
        s_memStatis.nb_large_buffer++;
        UPDATE_PEAK(s_memStatis.nb_large_buffer, s_memStatis.peak_large_buffer);
    }
    /* the RAM allocated is the buffer size and the block header size*/
    s_memStatis.ram_allocated += (uint16_t)(requestedSize + BLOCK_HDR_SIZE);
    UPDATE_PEAK(s_memStatis.ram_allocated, s_memStatis.peak_ram_allocated);

    uint32_t block_size = 0U;
    block_size          = (uint32_t)BlockHdr->next - (uint32_t)BlockHdr - BLOCK_HDR_SIZE;

    assert(block_size >= requestedSize);
    /* ram lost is the difference between block size and buffer size */
    s_memStatis.ram_lost += (uint16_t)(block_size - requestedSize);
    UPDATE_PEAK(s_memStatis.ram_lost, s_memStatis.peak_ram_lost);

    UPDATE_PEAK(((uint32_t)FreeBlockHdrList.tail + BLOCK_HDR_SIZE), s_memStatis.peak_upper_addr);

#ifdef MEM_MANAGER_BENCH
    if (time != 0U)
    {
        /* update mem stats used for benchmarking */
        s_memStatis.last_alloc_block_size = (uint16_t)block_size;
        s_memStatis.last_alloc_buff_size  = (uint16_t)requestedSize;
        s_memStatis.last_alloc_time       = (uint16_t)time;
        s_memStatis.total_alloc_time += time;
        s_memStatis.average_alloc_time = (uint16_t)(s_memStatis.total_alloc_time / s_memStatis.nb_alloc);
        UPDATE_PEAK((uint16_t)time, s_memStatis.peak_alloc_time);
    }
    else /* alloc time is not correct, we bypass this allocation's data */
    {
        s_memStatis.nb_alloc--;
    }
#else
    (void)time;
#endif /* MEM_MANAGER_BENCH */
}

static void MEM_BufferFrees_memStatis(void *buffer)
{
    void_ptr_t buffer_ptr;
    void_ptr_t blockHdr_ptr;
    blockHeader_t *BlockHdr;

    /* Use union to fix Misra */
    buffer_ptr.void_ptr      = buffer;
    blockHdr_ptr.raw_address = buffer_ptr.raw_address - BLOCK_HDR_SIZE;
    BlockHdr                 = blockHdr_ptr.block_hdr_ptr;

    /* Existing block must have a next block hdr */
    assert((BlockHdr != NULL) && (BlockHdr->next != NULL));

    s_memStatis.ram_allocated -= (uint16_t)(BlockHdr->buff_size + BLOCK_HDR_SIZE);
    /* Sort the buffers by size, based on defined thresholds */
    if (BlockHdr->buff_size <= SMALL_BUFFER_SIZE)
    {
        s_memStatis.nb_small_buffer--;
    }
    else if (BlockHdr->buff_size <= LARGE_BUFFER_SIZE)
    {
        s_memStatis.nb_medium_buffer--;
    }
    else
    {
        s_memStatis.nb_large_buffer--;
    }

    uint16_t block_size = 0U;
    block_size          = (uint16_t)((uint32_t)BlockHdr->next - (uint32_t)BlockHdr - BLOCK_HDR_SIZE);

    assert(block_size >= BlockHdr->buff_size);
    assert(s_memStatis.ram_lost >= (block_size - BlockHdr->buff_size));

    /* as the buffer is free, the ram is not "lost" anymore */
    s_memStatis.ram_lost -= (block_size - BlockHdr->buff_size);
}

#endif /* MEM_STATISTICS_INTERNAL */

#if defined(gMemManagerLightFreeBlocksCleanUp) && (gMemManagerLightFreeBlocksCleanUp > 0)
static void MEM_BufferFreeBlocksCleanUp(memAreaPrivDesc_t *p_area, blockHeader_t *BlockHdr)
{
    blockHeader_t *NextBlockHdr     = BlockHdr->next;
    blockHeader_t *NextFreeBlockHdr = BlockHdr->next_free;
    /* This function shouldn't be called on the last free block */
    assert(BlockHdr < p_area->ctx.FreeBlockHdrList.tail);

    /* Step forward and append contiguous free blocks if they can be merged with the unused top of heap */
    while (NextBlockHdr == NextFreeBlockHdr)
    {
        if (NextBlockHdr == NULL)
        {
#if (gMemManagerLightFreeBlocksCleanUp == 2)
            /* Step backwards to merge all preceeding contiguous free blocks */
            blockHeader_t *PrevFreeBlockHdr = BlockHdr->prev_free;
            while (PrevFreeBlockHdr->next == BlockHdr)
            {
                assert(PrevFreeBlockHdr->next_free == BlockHdr);
                assert(PrevFreeBlockHdr->used == MEMMANAGER_BLOCK_FREE);
                PrevFreeBlockHdr->next_free = BlockHdr->next_free;
                PrevFreeBlockHdr->next      = BlockHdr->next;
                BlockHdr                    = PrevFreeBlockHdr;
                PrevFreeBlockHdr            = BlockHdr->prev_free;
            }
#endif
            assert(BlockHdr->next == BlockHdr->next_free);
            assert(BlockHdr->used == MEMMANAGER_BLOCK_FREE);
            /* pool is reached.  All buffers from BlockHdr to the pool are free
               remove all next buffers */
            BlockHdr->next                    = NULL;
            BlockHdr->next_free               = NULL;
            p_area->ctx.FreeBlockHdrList.tail = BlockHdr;
            break;
        }
        NextBlockHdr     = NextBlockHdr->next;
        NextFreeBlockHdr = NextFreeBlockHdr->next_free;
    }
}
#endif /* gMemManagerLightFreeBlocksCleanUp */

#if defined(gMemManagerLightGuardsCheckEnable) && (gMemManagerLightGuardsCheckEnable == 1)
static void MEM_BlockHeaderCheck(blockHeader_t *BlockHdr)
{
    int ret;
    uint8_t guardPrePattern[BLOCK_HDR_PREGUARD_SIZE];
    uint8_t guardPostPattern[BLOCK_HDR_POSTGUARD_SIZE];

    (void)memset((void *)guardPrePattern, BLOCK_HDR_PREGUARD_PATTERN, BLOCK_HDR_PREGUARD_SIZE);
    ret = memcmp((const void *)&BlockHdr->preguard, (const void *)guardPrePattern, BLOCK_HDR_PREGUARD_SIZE);
    if (ret != 0)
    {
        MEM_DBG_LOG("Preguard Block Header Corrupted %x", BlockHdr);
    }
    assert(ret == 0);

    (void)memset((void *)guardPostPattern, BLOCK_HDR_POSTGUARD_PATTERN, BLOCK_HDR_POSTGUARD_SIZE);
    ret = memcmp((const void *)&BlockHdr->postguard, (const void *)guardPostPattern, BLOCK_HDR_POSTGUARD_SIZE);
    if (ret != 0)
    {
        MEM_DBG_LOG("Postguard Block Header Corrupted %x", BlockHdr);
    }
    assert(ret == 0);
}

static void MEM_BlockHeaderSetGuards(blockHeader_t *BlockHdr)
{
    (void)memset((void *)&BlockHdr->preguard, BLOCK_HDR_PREGUARD_PATTERN, BLOCK_HDR_PREGUARD_SIZE);
    (void)memset((void *)&BlockHdr->postguard, BLOCK_HDR_POSTGUARD_PATTERN, BLOCK_HDR_POSTGUARD_SIZE);
}

#endif

static memAreaPrivDesc_t *MEM_GetAreaByAreaId(uint8_t area_id)
{
    memAreaPrivDesc_t *p_area = &heap_area_list;
    for (uint8_t i = 0u; i < area_id; i++)
    {
        p_area = (memAreaPrivDesc_t *)(void *)p_area->next;
    }
    return p_area;
}

/*! *********************************************************************************
*************************************************************************************
* Public functions
*************************************************************************************
********************************************************************************** */

#if defined(MEM_STATISTICS_INTERNAL)
static void MEM_Reports_memStatis(void)
{
    MEM_DBG_LOG("**************** MEM STATS REPORT **************");
    MEM_DBG_LOG("Nb Alloc:                  %d\r\n", s_memStatis.nb_alloc);
    MEM_DBG_LOG("Small buffers:             %d\r\n", s_memStatis.nb_small_buffer);
    MEM_DBG_LOG("Medium buffers:            %d\r\n", s_memStatis.nb_medium_buffer);
    MEM_DBG_LOG("Large buffers:             %d\r\n", s_memStatis.nb_large_buffer);
    MEM_DBG_LOG("Peak small:                %d\r\n", s_memStatis.peak_small_buffer);
    MEM_DBG_LOG("Peak medium:               %d\r\n", s_memStatis.peak_medium_buffer);
    MEM_DBG_LOG("Peak large:                %d\r\n", s_memStatis.peak_large_buffer);
    MEM_DBG_LOG("Current RAM allocated:     %d bytes\r\n", s_memStatis.ram_allocated);
    MEM_DBG_LOG("Peak RAM allocated:        %d bytes\r\n", s_memStatis.peak_ram_allocated);
    MEM_DBG_LOG("Current RAM lost:          %d bytes\r\n", s_memStatis.ram_lost);
    MEM_DBG_LOG("Peak RAM lost:             %d bytes\r\n", s_memStatis.peak_ram_lost);
    MEM_DBG_LOG("Peak Upper Address:        %x\r\n", s_memStatis.peak_upper_addr);
#ifdef MEM_MANAGER_BENCH
    MEM_DBG_LOG("************************************************\r\n");
    MEM_DBG_LOG("********* MEM MANAGER BENCHMARK REPORT *********\r\n");
    MEM_DBG_LOG("Last Alloc Time:           %d us\r\n", s_memStatis.last_alloc_time);
    MEM_DBG_LOG("Last Alloc Block Size:     %d bytes\r\n", s_memStatis.last_alloc_block_size);
    MEM_DBG_LOG("Last Alloc Buffer Size:    %d bytes\r\n", s_memStatis.last_alloc_buff_size);
    MEM_DBG_LOG("Average Alloc Time:        %d us\r\n", s_memStatis.average_alloc_time);
    MEM_DBG_LOG("Peak Alloc Time:           %d us\r\n", s_memStatis.peak_alloc_time);
#endif /* MEM_MANAGER_BENCH */
    MEM_DBG_LOG("************************************************");
}
#endif /* MEM_STATISTICS_INTERNAL */

static bool initialized = false;

mem_status_t MEM_RegisterExtendedArea(memAreaCfg_t *area_desc, uint8_t *p_area_id, uint16_t flags)
{
    mem_status_t st = kStatus_MemSuccess;
    memAreaPrivDesc_t *p_area;
    uint32_t regPrimask = DisableGlobalIRQ();
    assert(offsetof(memAreaCfg_t, internal_ctx) == offsetof(memAreaPrivDesc_t, ctx));
    assert(sizeof(memAreaCfg_t) >= sizeof(memAreaPrivDesc_t));
    do
    {
        void_ptr_t ptr;
        blockHeader_t *firstBlockHdr;
        uint32_t initial_level;

        if (area_desc == NULL)
        {
            assert(flags == 0U);
            p_area = &heap_area_list;
            /* Area_desc can only be NULL in the case of the implicit default memHeap registration */
            if ((p_area->start_address.address_ptr != NULL) || (p_area->end_address.address_ptr != NULL))
            {
                st = kStatus_MemInitError;
                break;
            }
            /* The head of the area des list is necessarily the main heap */
            p_area->start_address.address_ptr = &memHeap[0];
            p_area->end_address.raw_address   = memHeapEnd;
            assert(p_area->end_address.raw_address > p_area->start_address.raw_address);
            p_area->next = NULL;
            if (p_area_id != NULL)
            {
                *p_area_id = 0u;
            }
        }
        else
        {
            uint32_t area_sz;

            memAreaPrivDesc_t *new_area_desc = (memAreaPrivDesc_t *)(void *)area_desc;
            assert((flags & AREA_FLAGS_RFFU) == 0U);
            /* Registering an additional area : memHeap nust have been registered beforehand */
            uint8_t id = 0;
            if (area_desc->start_address == NULL)
            {
                st = kStatus_MemInitError;
                break;
            }
            if (heap_area_list.start_address.address_ptr == NULL)
            {
                /* memHeap must have been registered before */
                st = kStatus_MemInitError;
                break;
            }
            area_sz = new_area_desc->end_address.raw_address - new_area_desc->start_address.raw_address;
            if (area_sz <= (uint32_t)KB((uint32_t)1U))
            {
                /* doesn't make sense to register an area smaller than 1024 bytes */
                st = kStatus_MemInitError;
                break;
            }

            id = 1;
            for (p_area = &heap_area_list; p_area->next != NULL; p_area = (memAreaPrivDesc_t *)(void *)p_area->next)
            {
                if (p_area == new_area_desc)
                {
                    st = kStatus_MemInitError;
                    break;
                }
                id++;
            }
            if (st != kStatus_MemSuccess)
            {
                break;
            }
            if (p_area_id != NULL)
            {
                /* Determine the rank of the area in the list and return it as area_id */
                *p_area_id = id;
            }
            p_area->next  = area_desc;     /* p_area still points to previous area desc */
            p_area        = new_area_desc; /* let p_area point to new element */
            p_area->flags = flags;
        }
        /* Here p_area points either to the implicit memHeap when invoked from MEM_Init or to the
         * newly appended area configuration descriptor
         */
        p_area->next    = NULL;
        ptr.address_ptr = p_area->start_address.address_ptr;
        firstBlockHdr   = ptr.block_hdr_ptr;

        /* MEM_DBG_LOG("%x %d\r\n", memHeap, heapSize_c/sizeof(uint32_t)); */

        /* Init firstBlockHdr as a free block */
        firstBlockHdr->next      = NULL;
        firstBlockHdr->used      = MEMMANAGER_BLOCK_FREE;
        firstBlockHdr->next_free = NULL;
        firstBlockHdr->prev_free = NULL;

#if defined(MEM_STATISTICS_INTERNAL)
        firstBlockHdr->buff_size = 0U;
#endif

        /* Init FreeBlockHdrList with firstBlockHdr */
        p_area->ctx.FreeBlockHdrList.head = firstBlockHdr;
        p_area->ctx.FreeBlockHdrList.tail = firstBlockHdr;
        initial_level = p_area->end_address.raw_address - ((uint32_t)firstBlockHdr + BLOCK_HDR_SIZE - 1U);

        p_area->low_watermark = initial_level;

#if defined(gMemManagerLightGuardsCheckEnable) && (gMemManagerLightGuardsCheckEnable == 1)
        MEM_BlockHeaderSetGuards(firstBlockHdr);
#endif

#if defined(MEM_STATISTICS_INTERNAL)
        /* Init memory statistics */
        MEM_Inits_memStatis(&p_area->ctx.statistics);
#endif

        st = kStatus_MemSuccess;
    } while (false);
    ENABLE_GLOBAL_IRQ(regPrimask);
    return st;
}

static bool MEM_AreaIsEmpty(memAreaPrivDesc_t *p_area)
{
    bool res = false;

    blockHeader_t *FreeBlockHdr     = p_area->ctx.FreeBlockHdrList.head;
    blockHeader_t *NextFreeBlockHdr = FreeBlockHdr->next_free;
    if ((FreeBlockHdr == (blockHeader_t *)p_area->start_address.raw_address) && (NextFreeBlockHdr == NULL))
    {
        res = true;
    }

    return res;
}

mem_status_t MEM_UnRegisterExtendedArea(uint8_t area_id)
{
    mem_status_t st = kStatus_MemUnknownError;
    memAreaPrivDesc_t *prev_area;
    memAreaPrivDesc_t *p_area_to_remove = NULL;
    uint32_t regPrimask                 = DisableGlobalIRQ();

    do
    {
        /* Cannot unregister main heap */
        if (area_id == 0U)
        {
            st = kStatus_MemFreeError;
            break;
        }
        prev_area = MEM_GetAreaByAreaId(area_id - 1U); /* Get previous area in list */
        if (prev_area == NULL)
        {
            st = kStatus_MemFreeError;
            break;
        }

        p_area_to_remove = (memAreaPrivDesc_t *)(void *)prev_area->next;
        if (p_area_to_remove == NULL)
        {
            st = kStatus_MemFreeError;
            break;
        }
        if (!MEM_AreaIsEmpty(p_area_to_remove))
        {
            st = kStatus_MemFreeError;
            break;
        }

        /* Only unchain if no remaining allocated buffers */
        prev_area->next        = p_area_to_remove->next;
        p_area_to_remove->next = NULL;

        st = kStatus_MemSuccess;
    } while (false);

    ENABLE_GLOBAL_IRQ(regPrimask);

    return st;
}

mem_status_t MEM_Init(void)
{
    mem_status_t st = kStatus_MemSuccess;
    uint8_t memHeap_id;
    if (initialized == false)
    {
        initialized = true;
        st          = MEM_RegisterExtendedArea(NULL, &memHeap_id, 0U); /* initialized default heap area */
    }
    return st;
}

static void *MEM_BufferAllocateFromArea(memAreaPrivDesc_t *p_area, uint8_t area_id, uint32_t numBytes)
{
    uint32_t regPrimask = DisableGlobalIRQ();

    blockHeader_t *FreeBlockHdr     = p_area->ctx.FreeBlockHdrList.head;
    blockHeader_t *NextFreeBlockHdr = FreeBlockHdr->next_free;
    blockHeader_t *PrevFreeBlockHdr = FreeBlockHdr->prev_free;
    blockHeader_t *BlockHdrFound    = NULL;

#if defined(cMemManagerLightReuseFreeBlocks) && (cMemManagerLightReuseFreeBlocks > 0)
    blockHeader_t *UsableBlockHdr = NULL;
#endif
    void *buffer = NULL;

#ifdef MEM_MANAGER_BENCH
    uint32_t START_TIME = 0U, STOP_TIME = 0U, ALLOC_TIME = 0U;
    START_TIME = TM_GetTimestamp();
#endif /* MEM_MANAGER_BENCH */

    do
    {
        assert(FreeBlockHdr->used == MEMMANAGER_BLOCK_FREE);
        if (FreeBlockHdr->next != NULL)
        {
            uint32_t available_size;
            available_size = (uint32_t)FreeBlockHdr->next - (uint32_t)FreeBlockHdr - BLOCK_HDR_SIZE;
            /* if a next block hdr exists, it means (by design) that a next free block exists too
               Because the last block header at the end of the heap will always be free
               So, the current block header can't be the tail, and the next free can't be NULL */
            assert(FreeBlockHdr < p_area->ctx.FreeBlockHdrList.tail);
            assert(FreeBlockHdr->next_free != NULL);

            if (available_size >= numBytes) /* enough space in this free buffer */
            {
#if defined(cMemManagerLightReuseFreeBlocks) && (cMemManagerLightReuseFreeBlocks > 0)
                /* this block could be used if the memory pool if full, so we memorize it */
                if (UsableBlockHdr == NULL)
                {
                    UsableBlockHdr = FreeBlockHdr;
                }
                /* To avoid waste of large blocks with small blocks, make sure the required size is big enough for the
                  available block otherwise, try an other block !
                  Do not check if available block size is 4 bytes, take the block anyway ! */
                if ((available_size <= 4u) ||
                    ((available_size - numBytes) < (available_size >> cMemManagerLightReuseFreeBlocks)))
#endif
                {
                    /* Found a matching free block */
                    FreeBlockHdr->used    = MEMMANAGER_BLOCK_USED;
                    FreeBlockHdr->area_id = area_id;
#if defined(MEM_STATISTICS_INTERNAL)
                    FreeBlockHdr->buff_size = (uint16_t)numBytes;
#endif
                    NextFreeBlockHdr = FreeBlockHdr->next_free;
                    PrevFreeBlockHdr = FreeBlockHdr->prev_free;

                    /* In the current state, the current block header can be anywhere
                       from list head to previous block of list tail */
                    if (p_area->ctx.FreeBlockHdrList.head == FreeBlockHdr)
                    {
                        p_area->ctx.FreeBlockHdrList.head = NextFreeBlockHdr;
                        NextFreeBlockHdr->prev_free       = NULL;
                    }
                    else
                    {
                        assert(p_area->ctx.FreeBlockHdrList.head->next_free <= FreeBlockHdr);

                        NextFreeBlockHdr->prev_free = PrevFreeBlockHdr;
                        PrevFreeBlockHdr->next_free = NextFreeBlockHdr;
                    }

                    BlockHdrFound = FreeBlockHdr;
                    break;
                }
            }
        }
        else
        {
            /* last block in the heap, check if available space to allocate the block */
            int32_t available_size;
            uint32_t total_size;
            uint32_t current_footprint = (uint32_t)FreeBlockHdr + BLOCK_HDR_SIZE - 1U;
            int32_t remaining_bytes;

            /* Current allocation should never be greater than heap end */
            available_size = (int32_t)p_area->end_address.raw_address - (int32_t)current_footprint;
            assert(available_size >= 0);

            assert(FreeBlockHdr == p_area->ctx.FreeBlockHdrList.tail);
            total_size      = (numBytes + BLOCK_HDR_SIZE);
            remaining_bytes = available_size - (int32_t)total_size;
            if (remaining_bytes >= 0) /* need to keep the room for the next BlockHeader */
            {
                if (p_area->low_watermark > (uint32_t)remaining_bytes)
                {
                    p_area->low_watermark = (uint32_t)remaining_bytes;
                }
                /* Depending on the platform, some RAM banks could need some reinitialization after a low power
                 * period, such as ECC RAM banks */
                MEM_ReinitRamBank((uint32_t)FreeBlockHdr + BLOCK_HDR_SIZE,
                                  ROUNDUP_WORD(((uint32_t)FreeBlockHdr + total_size + BLOCK_HDR_SIZE)));

                FreeBlockHdr->used    = MEMMANAGER_BLOCK_USED;
                FreeBlockHdr->area_id = area_id;
#if defined(MEM_STATISTICS_INTERNAL)
                FreeBlockHdr->buff_size = (uint16_t)numBytes;
#endif
                FreeBlockHdr->next      = (blockHeader_t *)ROUNDUP_WORD(((uint32_t)FreeBlockHdr + total_size));
                FreeBlockHdr->next_free = FreeBlockHdr->next;

                PrevFreeBlockHdr = FreeBlockHdr->prev_free;

                NextFreeBlockHdr       = FreeBlockHdr->next_free;
                NextFreeBlockHdr->used = MEMMANAGER_BLOCK_FREE;
#if defined(MEM_STATISTICS_INTERNAL)
                NextFreeBlockHdr->buff_size = 0U;
#endif
                NextFreeBlockHdr->next      = NULL;
                NextFreeBlockHdr->next_free = NULL;
                NextFreeBlockHdr->prev_free = PrevFreeBlockHdr;

                if (p_area->ctx.FreeBlockHdrList.head == FreeBlockHdr)
                {
                    assert(p_area->ctx.FreeBlockHdrList.head == p_area->ctx.FreeBlockHdrList.tail);
                    assert(PrevFreeBlockHdr == NULL);
                    /* last free block in heap was the only free block available
                       so now the first free block in the heap is the next one */
                    p_area->ctx.FreeBlockHdrList.head = FreeBlockHdr->next_free;
                }
                else
                {
                    /* update previous free block header to point its next
                       to the new free block */
                    PrevFreeBlockHdr->next_free = NextFreeBlockHdr;
                }

                /* new free block is now the tail of the free block list */
                p_area->ctx.FreeBlockHdrList.tail = NextFreeBlockHdr;

#if defined(gMemManagerLightGuardsCheckEnable) && (gMemManagerLightGuardsCheckEnable == 1)
                MEM_BlockHeaderSetGuards(NextFreeBlockHdr);
#endif

                BlockHdrFound = FreeBlockHdr;
            }
#if defined(cMemManagerLightReuseFreeBlocks) && (cMemManagerLightReuseFreeBlocks > 0)
            else if (UsableBlockHdr != NULL)
            {
                /* we found a free block that can be used */
                UsableBlockHdr->used    = MEMMANAGER_BLOCK_USED;
                UsableBlockHdr->area_id = area_id;
#if defined(MEM_STATISTICS_INTERNAL)
                UsableBlockHdr->buff_size = (uint16_t)numBytes;
#endif
                NextFreeBlockHdr = UsableBlockHdr->next_free;
                PrevFreeBlockHdr = UsableBlockHdr->prev_free;

                /* In the current state, the current block header can be anywhere
                   from list head to previous block of list tail */
                if (p_area->ctx.FreeBlockHdrList.head == UsableBlockHdr)
                {
                    p_area->ctx.FreeBlockHdrList.head = NextFreeBlockHdr;
                    NextFreeBlockHdr->prev_free       = NULL;
                }
                else
                {
                    assert(p_area->ctx.FreeBlockHdrList.head->next_free <= UsableBlockHdr);

                    NextFreeBlockHdr->prev_free = PrevFreeBlockHdr;
                    PrevFreeBlockHdr->next_free = NextFreeBlockHdr;
                }
                BlockHdrFound = UsableBlockHdr;
            }
#endif
            else
            {
                BlockHdrFound = NULL;
            }
            break;
        }
#if defined(gMemManagerLightGuardsCheckEnable) && (gMemManagerLightGuardsCheckEnable == 1)
        MEM_BlockHeaderCheck(FreeBlockHdr->next_free);
#endif
        FreeBlockHdr = FreeBlockHdr->next_free;
        /* avoid looping */
        assert(FreeBlockHdr != FreeBlockHdr->next_free);
    } while (true);
    /* MEM_DBG_LOG("BlockHdrFound: %x", BlockHdrFound); */

#ifdef MEM_DEBUG_OUT_OF_MEMORY
    assert(BlockHdrFound);
#endif

#ifdef MEM_MANAGER_BENCH
    STOP_TIME  = TM_GetTimestamp();
    ALLOC_TIME = STOP_TIME - START_TIME;
#endif /* MEM_MANAGER_BENCH */

    if (BlockHdrFound != NULL)
    {
        void_ptr_t buffer_ptr;
#ifdef MEM_TRACKING
        void_ptr_t lr;
        lr.raw_address                    = (uint32_t)__mem_get_LR();
        BlockHdrFound->first_alloc_caller = lr.void_ptr;
#endif
        buffer_ptr.raw_address = (uint32_t)BlockHdrFound + BLOCK_HDR_SIZE;
        buffer                 = buffer_ptr.void_ptr;
#ifdef MEM_STATISTICS_INTERNAL
#ifdef MEM_MANAGER_BENCH
        MEM_BufferAllocates_memStatis(buffer, ALLOC_TIME, numBytes);
#else
        MEM_BufferAllocates_memStatis(buffer, 0, numBytes);
#endif
        if ((p_area->ctx.statistics.nb_alloc % NB_ALLOC_REPORT_THRESHOLD) == 0U)
        {
            MEM_Reports_memStatis();
        }
#endif /* MEM_STATISTICS_INTERNAL */
    }
    else
    {
        /* TODO: Allocation failure try to merge free blocks together  */
    }

    EnableGlobalIRQ(regPrimask);

    return buffer;
}

static void *MEM_BufferAllocate(uint32_t numBytes, uint8_t poolId)
{
    memAreaPrivDesc_t *p_area;
    void *buffer    = NULL;
    uint8_t area_id = 0U;

    if (initialized == false)
    {
        (void)MEM_Init();
    }
    if (poolId == 0U)
    {
        area_id = 0U;
        for (p_area = &heap_area_list; p_area != NULL; p_area = (memAreaPrivDesc_t *)(void *)p_area->next)
        {
            if ((p_area->flags & AREA_FLAGS_POOL_NOT_SHARED) == 0U)
            {
                buffer = MEM_BufferAllocateFromArea(p_area, area_id, numBytes);
                if (buffer != NULL)
                {
                    break;
                }
            }
            area_id++;
        }
    }
    else
    {
        p_area = MEM_GetAreaByAreaId(poolId); /* Exclusively allocate from targeted pool */
        if (p_area != NULL)
        {
            buffer = MEM_BufferAllocateFromArea(p_area, poolId, numBytes);
        }
    }
    return buffer;
}

void *MEM_BufferAllocWithId(uint32_t numBytes, uint8_t poolId)
{
#ifdef MEM_TRACKING
    void_ptr_t BlockHdr_ptr;
#endif
    void_ptr_t buffer_ptr;

#if defined(gFSCI_MemAllocTest_Enabled_d) && (gFSCI_MemAllocTest_Enabled_d)
    void *pCaller = (void *)((uint32_t *)__mem_get_LR());
    /* Verify if the caller is part of any FSCI memory allocation test. If so, return NULL. */
    if (FSCI_MemAllocTestCanAllocate(pCaller) == kStatus_AllocBlock)
    {
        buffer_ptr.void_ptr = NULL;
        return buffer_ptr.void_ptr;
    }
#endif

    /* Alloc a buffer */
    buffer_ptr.void_ptr = MEM_BufferAllocate(numBytes, poolId);

#ifdef MEM_TRACKING
    if (buffer_ptr.void_ptr != NULL)
    {
        BlockHdr_ptr.raw_address = buffer_ptr.raw_address - BLOCK_HDR_SIZE;
        /* store caller */
        BlockHdr_ptr.block_hdr_ptr->second_alloc_caller = (void *)((uint32_t *)__mem_get_LR());
        ;
    }
#endif

    return buffer_ptr.void_ptr;
}

static mem_status_t MEM_BufferFreeBackToArea(memAreaPrivDesc_t *p_area, void *buffer)
{
    void_ptr_t buffer_ptr;
    buffer_ptr.void_ptr = buffer;
    blockHeader_t *BlockHdr;
    BlockHdr = (blockHeader_t *)(buffer_ptr.raw_address - BLOCK_HDR_SIZE);

    mem_status_t ret = kStatus_MemSuccess;
    /* when allocating a buffer, we always create a FreeBlockHdr at
       the end of the buffer, so the FreeBlockHdrList.tail should always
       be at a higher address than current BlockHdr */
    assert((uint32_t)BlockHdr < (uint32_t)p_area->ctx.FreeBlockHdrList.tail);

#if defined(gMemManagerLightGuardsCheckEnable) && (gMemManagerLightGuardsCheckEnable == 1)
    MEM_BlockHeaderCheck(BlockHdr->next);
#endif

    /* MEM_DBG_LOG("%x %d", BlockHdr, BlockHdr->buff_size); */

#if defined(MEM_STATISTICS_INTERNAL)
    MEM_BufferFrees_memStatis(buffer);
#endif /* MEM_STATISTICS_INTERNAL */

    if ((uint32_t)BlockHdr < (uint32_t)p_area->ctx.FreeBlockHdrList.head)
    {
        /* BlockHdr is placed before FreeBlockHdrList.head so we can set it as
           the new head of the list */
        BlockHdr->next_free                          = p_area->ctx.FreeBlockHdrList.head;
        BlockHdr->prev_free                          = NULL;
        p_area->ctx.FreeBlockHdrList.head->prev_free = BlockHdr;
        p_area->ctx.FreeBlockHdrList.head            = BlockHdr;
    }
    else
    {
        /* we want to find the previous free block header
           here, we cannot use prev_free as this information could be outdated
           so we need to run through the whole list to be sure to catch the
           correct previous free block header */
        blockHeader_t *PrevFreeBlockHdr = p_area->ctx.FreeBlockHdrList.head;
        while ((uint32_t)PrevFreeBlockHdr->next_free < (uint32_t)BlockHdr)
        {
            PrevFreeBlockHdr = PrevFreeBlockHdr->next_free;
        }
        /* insert the new free block in the list */
        BlockHdr->next_free            = PrevFreeBlockHdr->next_free;
        BlockHdr->prev_free            = PrevFreeBlockHdr;
        BlockHdr->next_free->prev_free = BlockHdr;
        PrevFreeBlockHdr->next_free    = BlockHdr;
    }

    BlockHdr->used = MEMMANAGER_BLOCK_FREE;
#if defined(MEM_STATISTICS_INTERNAL)
    BlockHdr->buff_size = 0U;
#endif

#if defined(gMemManagerLightFreeBlocksCleanUp) && (gMemManagerLightFreeBlocksCleanUp != 0)
    MEM_BufferFreeBlocksCleanUp(p_area, BlockHdr);
#endif
    return ret;
}

mem_status_t MEM_BufferFree(void *buffer /* IN: Block of memory to free*/)
{
    mem_status_t ret = kStatus_MemSuccess;
    void_ptr_t buffer_ptr;
    buffer_ptr.void_ptr = buffer;

    if (buffer == NULL)
    {
        ret = kStatus_MemFreeError;
    }
    else
    {
        uint32_t regPrimask = DisableGlobalIRQ();

        blockHeader_t *BlockHdr;
        BlockHdr = (blockHeader_t *)(buffer_ptr.raw_address - BLOCK_HDR_SIZE);

        /* assert checks */
        assert(BlockHdr->used == MEMMANAGER_BLOCK_USED);
        assert(BlockHdr->next != NULL);
        memAreaPrivDesc_t *p_area = MEM_GetAreaByAreaId(BlockHdr->area_id);

        if (p_area != NULL)
        {
            ret = MEM_BufferFreeBackToArea(p_area, buffer);
        }
        else
        {
            assert(false);
            ret = kStatus_MemFreeError;
        }

        EnableGlobalIRQ(regPrimask);
    }

    return ret;
}

mem_status_t MEM_BufferCheck(void *buffer, uint32_t size)
{
    mem_status_t ret = kStatus_MemSuccess;
    void_ptr_t buffer_ptr;
    buffer_ptr.void_ptr = buffer;

    if (buffer == NULL)
    {
        ret = kStatus_MemUnknownError;
    }
    else
    {
        uint32_t regPrimask = DisableGlobalIRQ();

        blockHeader_t *BlockHdr;
        BlockHdr = (blockHeader_t *)(buffer_ptr.raw_address - BLOCK_HDR_SIZE);
        /* checks buffer is valid */
        if ((BlockHdr->used == MEMMANAGER_BLOCK_USED) && (BlockHdr->next != NULL))
        {
            memAreaPrivDesc_t *p_area = MEM_GetAreaByAreaId(BlockHdr->area_id);
            if (p_area != NULL)
            {
                /* Not memory manager buffer, do not care */
                if (((uint8_t*)buffer < (uint8_t*)p_area->start_address.raw_address) || 
                    ((uint8_t*)buffer > (uint8_t*)p_area->end_address.raw_address))
                {
                    ret = kStatus_MemUnknownError;
                }
                else
                {
                    if( size > MEM_BufferGetSize(buffer) )
                    {
                      assert(false);
                      ret = kStatus_MemOverFlowError;
                    }
                }
            }
            else 
            {
                ret = kStatus_MemUnknownError;
            }
        }

        EnableGlobalIRQ(regPrimask);
    }

    return ret;
}

mem_status_t MEM_BufferFreeAllWithId(uint8_t poolId)
{
    mem_status_t status = kStatus_MemSuccess;
#if (defined(MEM_TRACK_ALLOC_SOURCE) && (MEM_TRACK_ALLOC_SOURCE == 1))
#ifdef MEMMANAGER_NOT_IMPLEMENTED_YET

#endif /* MEMMANAGER_NOT_IMPLEMENTED_YET */
#else  /* (defined(MEM_TRACK_ALLOC_SOURCE) && (MEM_TRACK_ALLOC_SOURCE == 1)) */
    status = kStatus_MemFreeError;
#endif /* (defined(MEM_TRACK_ALLOC_SOURCE) && (MEM_TRACK_ALLOC_SOURCE == 1)) */
    return status;
}

uint32_t MEM_GetHeapUpperLimitByAreaId(uint8_t area_id)
{
    /* There is always a free block at the end of the heap
        and this free block is the tail of the list */
    uint32_t upper_limit = 0U;
    do
    {
        memAreaPrivDesc_t *p_area;
        p_area = MEM_GetAreaByAreaId(area_id);
        if (p_area == NULL)
        {
            break;
        }
        upper_limit = ((uint32_t)p_area->ctx.FreeBlockHdrList.tail + BLOCK_HDR_SIZE);

    } while (false);

    return upper_limit;
}

uint32_t MEM_GetHeapUpperLimit(void)
{
    return MEM_GetHeapUpperLimitByAreaId(0u);
}

uint32_t MEM_GetFreeHeapSizeLowWaterMarkByAreaId(uint8_t area_id)
{
    uint32_t low_watermark = 0U;
    do
    {
        memAreaPrivDesc_t *p_area;
        p_area = MEM_GetAreaByAreaId(area_id);
        if (p_area == NULL)
        {
            break;
        }
        low_watermark = p_area->low_watermark;

    } while (false);
    return low_watermark;
}

uint32_t MEM_GetFreeHeapSizeLowWaterMark(void)
{
    return MEM_GetFreeHeapSizeLowWaterMarkByAreaId(0u);
}

uint32_t MEM_ResetFreeHeapSizeLowWaterMarkByAreaId(uint8_t area_id)
{
    uint32_t current_level = 0U;
    do
    {
        memAreaPrivDesc_t *p_area;
        blockHeader_t *FreeBlockHdr;
        uint32_t current_footprint;
        p_area = MEM_GetAreaByAreaId(area_id);
        if (p_area == NULL)
        {
            break;
        }
        FreeBlockHdr      = p_area->ctx.FreeBlockHdrList.head;
        current_footprint = (uint32_t)FreeBlockHdr + BLOCK_HDR_SIZE - 1U;

        /* Current allocation should never be greater than heap end */
        current_level         = p_area->end_address.raw_address - current_footprint;
        p_area->low_watermark = current_level;

    } while (false);
    return current_level;
}

uint32_t MEM_ResetFreeHeapSizeLowWaterMark(void)
{
    return MEM_ResetFreeHeapSizeLowWaterMarkByAreaId(0u);
}

uint16_t MEM_BufferGetSize(void *buffer)
{
    blockHeader_t *BlockHdr = NULL;
    uint16_t size;
    /* union used to fix Misra */
    void_ptr_t buffer_ptr;
    buffer_ptr.void_ptr = buffer;

    if (buffer != NULL)
    {
        BlockHdr = (blockHeader_t *)(buffer_ptr.raw_address - BLOCK_HDR_SIZE);
        /* block size is the space between current BlockHdr and next BlockHdr */
        size = (uint16_t)((uint32_t)BlockHdr->next - (uint32_t)BlockHdr - BLOCK_HDR_SIZE);
    }
    else
    {
        /* is case of a NULL buffer, we return 0U */
        size = 0U;
    }

    return size;
}

void *MEM_BufferRealloc(void *buffer, uint32_t new_size)
{
    void *realloc_buffer = NULL;
    uint16_t block_size  = 0U;
    do
    {
        if (new_size >= MAX_UINT16)
        {
            realloc_buffer = NULL;
            /* Bypass he whole procedure so keep original buffer that cannot be reallocated */
            break;
        }
        if (new_size == 0U)
        {
            /* new requested size is 0, free old buffer */
            (void)MEM_BufferFree(buffer);
            realloc_buffer = NULL;
            break;
        }
        if (buffer == NULL)
        {
            /* input buffer is NULL simply allocate a new buffer and return it */
            realloc_buffer = MEM_BufferAllocate(new_size, 0U);
            break;
        }
        /* Current buffer needs to be reallocated */
        block_size = MEM_BufferGetSize(buffer);

        if ((uint16_t)new_size <= block_size)
        {
            /* current buffer is large enough for the new requested size
               we can still use it */
            realloc_buffer = buffer;
        }
        else
        {
            /* not enough space in the current block, creating a new one */
            realloc_buffer = MEM_BufferAllocate(new_size, 0U);

            if (realloc_buffer != NULL)
            {
                /* copy input buffer data to new buffer */
                (void)memcpy(realloc_buffer, buffer, (uint32_t)block_size);

                /* free old buffer */
                (void)MEM_BufferFree(buffer);
            }
        }
    } while (false);
    return realloc_buffer;
}
static uint32_t MEM_GetFreeHeapSpaceInArea(memAreaPrivDesc_t *p_area)
{
    uint32_t free_sz = 0U;
    /* skip unshared areas  */
    blockHeader_t *freeBlockHdr = p_area->ctx.FreeBlockHdrList.head;

    /* Count every free block in the free space */
    while (freeBlockHdr != p_area->ctx.FreeBlockHdrList.tail)
    {
        free_sz += ((uint32_t)freeBlockHdr->next - (uint32_t)freeBlockHdr - BLOCK_HDR_SIZE);
        freeBlockHdr = freeBlockHdr->next_free;
    }

    /* Add remaining free space in the heap */
    free_sz += p_area->end_address.raw_address - (uint32_t)p_area->ctx.FreeBlockHdrList.tail - BLOCK_HDR_SIZE + (uint32_t)1U;
    return free_sz;
}

uint32_t MEM_GetFreeHeapSizeByAreaId(uint8_t area_id)
{
    memAreaPrivDesc_t *p_area;
    uint32_t free_size = 0U;

    if (area_id == 0U)
    {
        /* Iterate through all registered areas */
        for (p_area = &heap_area_list; p_area != NULL; p_area = (memAreaPrivDesc_t *)(void *)p_area->next)
        {
            if ((p_area->flags & AREA_FLAGS_POOL_NOT_SHARED) == 0U)
            {
                free_size += MEM_GetFreeHeapSpaceInArea(p_area);
            }
        }
    }
    else
    {
        p_area = MEM_GetAreaByAreaId(area_id);
        if (p_area != NULL)
        {
            free_size = MEM_GetFreeHeapSpaceInArea(p_area);
        }
    }
    return free_size;
}

uint32_t MEM_GetFreeHeapSize(void)
{
    return MEM_GetFreeHeapSizeByAreaId(0U);
}

__attribute__((weak)) void MEM_ReinitRamBank(uint32_t startAddress, uint32_t endAddress)
{
    /* To be implemented by the platform */
    (void)startAddress;
    (void)endAddress;
}

#if 0 /* MISRA C-2012 Rule 8.4 */
uint32_t MEM_GetAvailableBlocks(uint32_t size)
{
    /* Function not implemented yet */
    assert(0);

    return 0U;
}
#endif

void *MEM_CallocAlt(size_t len, size_t val)
{
    size_t blk_size;

    blk_size = len * val;

    void *pData = MEM_BufferAllocate(blk_size, 0U);
    if (NULL != pData)
    {
        (void)memset(pData, 0, blk_size);
    }

    return pData;
}

#if 0 /* MISRA C-2012 Rule 8.4 */
void MEM_FreeAlt(void *pData)
{
    /* Function not implemented yet */
    assert(0);
}
#endif

#endif
