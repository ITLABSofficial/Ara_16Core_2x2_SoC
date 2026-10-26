/*
 * Copyright (c) 2026 ETH Zurich and University of Bologna.
 * SPDX-License-Identifier: Apache-2.0
 *
 * User-Space C Runtime Library Implementation for Ara MultiCore WSE
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "libara_runtime.h"

ara_device_t* ara_init(int dev_idx)
{
    char dev_path[64];
    ara_device_t* dev;

    dev = (ara_device_t*)calloc(1, sizeof(ara_device_t));
    if (!dev) {
        fprintf(stderr, "libara: Failed to allocate device context\n");
        return NULL;
    }

    dev->dev_idx = dev_idx;
    snprintf(dev_path, sizeof(dev_path), "/dev/ara_wse%d", dev_idx);

    dev->fd = open(dev_path, O_RDWR | O_SYNC);
    if (dev->fd < 0) {
        /* Fallback to Software Emulation / Mock Hardware Mode */
        dev->is_emulated = true;
        dev->csr_base  = calloc(1, 64 * 1024);
        dev->tcdm_base = calloc(1, ARA_TCDM_REGION_SIZE);
        dev->hbm_base  = calloc(1, 16 * 1024 * 1024); /* 16 MB mock HBM window */
        printf("libara: Notice: %s not accessible (%s). Running in Host Software Emulation Mode.\n",
               dev_path, strerror(errno));
    } else {
        dev->is_emulated = false;
        printf("libara: Successfully attached to hardware device node %s\n", dev_path);
    }

    return dev;
}

void ara_close(ara_device_t* dev)
{
    if (!dev) return;

    if (dev->is_emulated) {
        free(dev->csr_base);
        free(dev->tcdm_base);
        free(dev->hbm_base);
    } else {
        if (dev->csr_base)  munmap(dev->csr_base, 64 * 1024);
        if (dev->tcdm_base) munmap(dev->tcdm_base, ARA_TCDM_REGION_SIZE);
        if (dev->hbm_base)  munmap(dev->hbm_base, ARA_HBM_REGION_SIZE);
        if (dev->fd >= 0)   close(dev->fd);
    }

    free(dev);
}

void* ara_mmap_csr(ara_device_t* dev)
{
    if (!dev) return NULL;
    if (dev->is_emulated) return dev->csr_base;

    if (!dev->csr_base) {
        dev->csr_base = mmap(NULL, 64 * 1024, PROT_READ | PROT_WRITE,
                             MAP_SHARED, dev->fd, ARA_MMAP_CSR_OFFSET);
        if (dev->csr_base == MAP_FAILED) {
            dev->csr_base = NULL;
            return NULL;
        }
    }
    return dev->csr_base;
}

void* ara_mmap_tcdm(ara_device_t* dev, uint32_t cluster_id)
{
    (void)cluster_id;
    if (!dev) return NULL;
    if (dev->is_emulated) return dev->tcdm_base;

    if (!dev->tcdm_base) {
        dev->tcdm_base = mmap(NULL, ARA_TCDM_REGION_SIZE, PROT_READ | PROT_WRITE,
                              MAP_SHARED, dev->fd, ARA_MMAP_TCDM_OFFSET);
        if (dev->tcdm_base == MAP_FAILED) {
            dev->tcdm_base = NULL;
            return NULL;
        }
    }
    return dev->tcdm_base;
}

void* ara_mmap_hbm(ara_device_t* dev, uint32_t stack_id)
{
    (void)stack_id;
    if (!dev) return NULL;
    if (dev->is_emulated) return dev->hbm_base;

    if (!dev->hbm_base) {
        dev->hbm_base = mmap(NULL, ARA_HBM_REGION_SIZE, PROT_READ | PROT_WRITE,
                             MAP_SHARED, dev->fd, ARA_MMAP_HBM_OFFSET);
        if (dev->hbm_base == MAP_FAILED) {
            dev->hbm_base = NULL;
            return NULL;
        }
    }
    return dev->hbm_base;
}

void ara_munmap(void* addr, size_t size)
{
    if (addr && addr != MAP_FAILED) {
        munmap(addr, size);
    }
}

int ara_submit_task(ara_device_t* dev, const struct ara_task_desc* task)
{
    if (!dev || !task) return -EINVAL;

    if (dev->is_emulated) {
        if (dev->csr_base) {
            volatile uint32_t *csr = (volatile uint32_t *)dev->csr_base;
            uint32_t desc_w3 = (((uint32_t)(task->op_code & 0xFF)) << 24) |
                               (((uint32_t)(task->flags & 0x03))   << 22) |
                               (((uint32_t)(task->matrix_k & 0xFFF)) << 4) |
                               ((uint32_t)(task->cluster_mask & 0x0F));
            csr[ARA_REG_DESC_0 >> 2]        = (uint32_t)(task->src_a_addr & 0xFFFFFFFFULL);
            csr[ARA_REG_DESC_1 >> 2]        = (uint32_t)(task->dst_c_addr & 0xFFFFFFFFULL);
            csr[ARA_REG_DESC_2 >> 2]        = (uint32_t)(task->src_b_addr & 0xFFFFFFFFULL);
            csr[ARA_REG_DESC_3 >> 2]        = desc_w3;
            csr[ARA_REG_DOORBELL >> 2]      = task->task_id;
            csr[ARA_REG_COMPLETED_TASK >> 2]= task->task_id;
        }
        dev->mock_task_counter++;
        return 0;
    }

    return ioctl(dev->fd, ARA_IOC_SUBMIT_TASK, task);
}

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
) {
    struct ara_task_desc desc;
    memset(&desc, 0, sizeof(desc));

    desc.task_id      = task_id;
    desc.op_code      = ARA_OP_GEMM;
    desc.cluster_mask = cluster_mask;
    desc.matrix_m     = m;
    desc.matrix_k     = k;
    desc.matrix_n     = n;
    desc.src_a_addr   = a_phys_addr;
    desc.src_b_addr   = b_phys_addr;
    desc.dst_c_addr   = c_phys_addr;
    desc.flags        = flags;

    return ara_submit_task(dev, &desc);
}

int ara_submit_rmsnorm(
    ara_device_t* dev,
    uint32_t      task_id,
    uint16_t      cluster_mask,
    uint32_t      len,
    uint64_t      src_addr,
    uint64_t      gamma_addr,
    uint64_t      dst_addr
) {
    struct ara_task_desc desc;
    memset(&desc, 0, sizeof(desc));

    desc.task_id      = task_id;
    desc.op_code      = ARA_OP_RMSNORM;
    desc.cluster_mask = cluster_mask;
    desc.matrix_m     = 1;
    desc.matrix_k     = len;
    desc.matrix_n     = len;
    desc.src_a_addr   = src_addr;
    desc.src_b_addr   = gamma_addr;
    desc.dst_c_addr   = dst_addr;
    desc.flags        = ARA_FLAG_ENABLE_IRQ;

    return ara_submit_task(dev, &desc);
}

int ara_submit_swiglu(
    ara_device_t* dev,
    uint32_t      task_id,
    uint16_t      cluster_mask,
    uint32_t      len,
    uint64_t      gate_addr,
    uint64_t      up_addr,
    uint64_t      dst_addr
) {
    struct ara_task_desc desc;
    memset(&desc, 0, sizeof(desc));

    desc.task_id      = task_id;
    desc.op_code      = ARA_OP_SWIGLU_FMA;
    desc.cluster_mask = cluster_mask;
    desc.matrix_m     = 1;
    desc.matrix_k     = len;
    desc.matrix_n     = len;
    desc.src_a_addr   = gate_addr;
    desc.src_b_addr   = up_addr;
    desc.dst_c_addr   = dst_addr;
    desc.flags        = ARA_FLAG_ACTIVATION_SILU | ARA_FLAG_ENABLE_IRQ;

    return ara_submit_task(dev, &desc);
}

int ara_wait_task(ara_device_t* dev, uint32_t task_id, uint32_t timeout_ms)
{
    (void)timeout_ms;
    if (!dev) return -EINVAL;

    if (dev->is_emulated) {
        return 0; /* Emulation immediately returns success */
    }

    return ioctl(dev->fd, ARA_IOC_WAIT_TASK, &task_id);
}

uint32_t ara_barrier_sync(ara_device_t* dev)
{
    uint32_t barrier_val = 0;
    if (!dev) return 0xFFFFFFFF;

    if (dev->is_emulated) {
        return 0x00000000;
    }

    if (ioctl(dev->fd, ARA_IOC_READ_BARRIER, &barrier_val) < 0) {
        return 0xFFFFFFFF;
    }
    return barrier_val;
}

int ara_global_abort(ara_device_t* dev)
{
    if (!dev) return -EINVAL;
    if (dev->is_emulated) {
        printf("libara: Mock hardware abort triggered.\n");
        return 0;
    }
    return ioctl(dev->fd, ARA_IOC_ABORT);
}

int ara_get_status(ara_device_t* dev, struct ara_status_info* status)
{
    if (!dev || !status) return -EINVAL;

    if (dev->is_emulated) {
        memset(status, 0, sizeof(*status));
        status->hw_version      = 0x20260201;
        status->sys_busy        = 0;
        status->active_tiles    = 0x000F;
        status->pdn_safe        = 1;
        status->completed_tasks = dev->mock_task_counter;
        return 0;
    }

    return ioctl(dev->fd, ARA_IOC_GET_STATUS, status);
}

int ara_config_pdn(ara_device_t* dev, uint32_t step_delay_cycles, int stagger_down)
{
    struct ara_pdn_config cfg;
    if (!dev) return -EINVAL;

    cfg.step_delay_cycles = step_delay_cycles;
    cfg.stagger_down_en   = stagger_down ? 1 : 0;

    if (dev->is_emulated) {
        return 0;
    }

    return ioctl(dev->fd, ARA_IOC_CONFIG_PDN, &cfg);
}
