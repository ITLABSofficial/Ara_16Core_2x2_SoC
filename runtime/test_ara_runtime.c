/*
 * Copyright (c) 2026 ETH Zurich and University of Bologna.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Test harness for libara_runtime user-space library
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "libara_runtime.h"

int main(void)
{
    printf("============================================================================\n");
    printf("  ARA WSE C RUNTIME LIBRARY (libara_runtime) UNIT TEST SUITE\n");
    printf("============================================================================\n\n");

    /* 1. Device Initialization */
    printf("[TEST 1] Initializing Ara WSE Device Context...\n");
    ara_device_t* dev = ara_init(0);
    assert(dev != NULL);
    printf("  [PASS] Device initialized successfully (Emulated Mode: %s)\n",
           dev->is_emulated ? "TRUE" : "FALSE");

    /* 2. Memory Apertures */
    printf("\n[TEST 2] Mapping Memory Apertures (CSR, TCDM, HBM)...\n");
    void* csr_ptr  = ara_mmap_csr(dev);
    void* tcdm_ptr = ara_mmap_tcdm(dev, 0);
    void* hbm_ptr  = ara_mmap_hbm(dev, 0);
    assert(csr_ptr != NULL);
    assert(tcdm_ptr != NULL);
    assert(hbm_ptr != NULL);
    printf("  [PASS] All 3 memory apertures mapped cleanly!\n");

    /* 3. GEMM Task Submission */
    printf("\n[TEST 3] Packaging & Submitting 16x16 GEMM Task...\n");
    int ret = ara_submit_gemm(
        dev,
        1001,       /* Task ID */
        0x000F,     /* 2x2 Grid all 4 clusters */
        16, 16, 16, /* M, K, N */
        0x0080000000ULL, /* Src A */
        0x0080010000ULL, /* Src B */
        0x0080020000ULL, /* Dst C */
        ARA_FLAG_ENABLE_IRQ
    );
    assert(ret == 0);
    printf("  [PASS] 16x16 GEMM task submitted without error!\n");

    /* 4. RMSNorm Task Submission */
    printf("\n[TEST 4] Submitting RMSNorm Token Normalization Task...\n");
    ret = ara_submit_rmsnorm(
        dev,
        1002,
        0x000F,
        16,
        0x0080000000ULL,
        0x0080000100ULL,
        0x0080000200ULL
    );
    assert(ret == 0);
    printf("  [PASS] RMSNorm task submitted cleanly!\n");

    /* 5. SwiGLU Activation Task Submission */
    printf("\n[TEST 5] Submitting Fused SwiGLU Feed-Forward Task...\n");
    ret = ara_submit_swiglu(
        dev,
        1003,
        0x000F,
        16,
        0x0080000300ULL,
        0x0080000400ULL,
        0x0080000500ULL
    );
    assert(ret == 0);
    {
        volatile uint32_t *csr = (volatile uint32_t *)csr_ptr;
        assert(csr[ARA_REG_DESC_3 >> 2] == 0x05C0010FU);
        assert(csr[ARA_REG_DOORBELL >> 2] == 1003U);
    }
    printf("  [PASS] Fused SwiGLU task submitted cleanly (DESC_3=0x05C0010F verified)!\n");

    /* 6. Wait Completion & Barrier */
    printf("\n[TEST 6] Testing Completion Polling & Hardware Barrier Tree...\n");
    ret = ara_wait_task(dev, 1003, 1000);
    assert(ret == 0);
    uint32_t barrier = ara_barrier_sync(dev);
    printf("  [PASS] Task completed; Barrier sync returned 0x%08X!\n", barrier);

    /* 7. Status & PDN Telemetry */
    printf("\n[TEST 7] Querying Device Status & Configuring PDN Inrush...\n");
    struct ara_status_info status;
    ret = ara_get_status(dev, &status);
    assert(ret == 0);
    printf("  Hardware Version: 0x%08X\n", status.hw_version);
    printf("  Active Tiles:     0x%04X\n", status.active_tiles);
    printf("  Completed Tasks:  %u\n", status.completed_tasks);
    assert(status.completed_tasks >= 3);

    ret = ara_config_pdn(dev, 16, 1);
    assert(ret == 0);
    printf("  [PASS] PDN Stagger delay configured to 16 cycles!\n");

    /* 8. Global Abort */
    printf("\n[TEST 8] Triggering Host Emergency Hardware Abort...\n");
    ret = ara_global_abort(dev);
    assert(ret == 0);
    printf("  [PASS] Emergency abort acknowledged!\n");

    /* Cleanup */
    ara_close(dev);
    printf("\n============================================================================\n");
    printf("  >>> ALL C RUNTIME TESTS PASSED (0 ERRORS)! <<<\n");
    printf("============================================================================\n");

    return 0;
}
