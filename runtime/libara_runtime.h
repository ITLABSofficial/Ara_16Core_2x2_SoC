/*
 * Copyright (c) 2026 ETH Zurich and University of Bologna.
 * SPDX-License-Identifier: Apache-2.0
 *
 * User-Space C Runtime Library for Ara MultiCore Wafer-Scale Engine (WSE)
 * Provides high-level tensor dispatch, memory mapping, and synchronization.
 */

#ifndef _LIBARA_RUNTIME_H_
#define _LIBARA_RUNTIME_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "../driver/ara_wse_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ara_device {
    int fd;
    bool is_emulated;
    void *csr_base;
    void *tcdm_base;
    void *hbm_base;
    uint32_t dev_idx;
    uint32_t mock_task_counter;
} ara_device_t;

/* Device Management */
ara_device_t* ara_init(int dev_idx);
void          ara_close(ara_device_t* dev);

/* Memory Mapping */
void* ara_mmap_csr(ara_device_t* dev);
void* ara_mmap_tcdm(ara_device_t* dev, uint32_t cluster_id);
void* ara_mmap_hbm(ara_device_t* dev, uint32_t stack_id);
void  ara_munmap(void* addr, size_t size);

/* Task Dispatch APIs */
int ara_submit_task(ara_device_t* dev, const struct ara_task_desc* task);

int ara_submit_gemm(
    ara_device_t* dev,
    uint32_t      task_id,
    uint16_t      cluster_mask,
    uint32_t      m,
    uint32_t      k,
    uint32_t      n,
    uint64_t      a_phys_addr,
    uint64_t      b_phys_addr,
    uint64_t      c_phys_addr,
    uint32_t      flags
);

int ara_submit_rmsnorm(
    ara_device_t* dev,
    uint32_t      task_id,
    uint16_t      cluster_mask,
    uint32_t      len,
    uint64_t      src_addr,
    uint64_t      gamma_addr,
    uint64_t      dst_addr
);

int ara_submit_swiglu(
    ara_device_t* dev,
    uint32_t      task_id,
    uint16_t      cluster_mask,
    uint32_t      len,
    uint64_t      gate_addr,
    uint64_t      up_addr,
    uint64_t      dst_addr
);

/* Synchronization & Hardware Barriers */
int      ara_wait_task(ara_device_t* dev, uint32_t task_id, uint32_t timeout_ms);
uint32_t ara_barrier_sync(ara_device_t* dev);
int      ara_global_abort(ara_device_t* dev);

/* Hardware Telemetry & PDN Control */
int ara_get_status(ara_device_t* dev, struct ara_status_info* status);
int ara_config_pdn(ara_device_t* dev, uint32_t step_delay_cycles, int stagger_down);

#ifdef __cplusplus
}
#endif

#endif /* _LIBARA_RUNTIME_H_ */
