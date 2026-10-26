# Ara_16Core_2x2_SoC: 16-Core GALS Multicore AI Processor & PIM
**16.384 TFLOPS FP16 Multicore SoC, 2x2 NoC Mesh, GALS CDC, 4-Zone PDN, and Base-Die PIM**

[![License](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](LICENSE)
[![Compute](https://img.shields.io/badge/Compute_Throughput-16.384_TFLOPS_FP16-brightgreen.svg)]()
[![NoC](https://img.shields.io/badge/NoC_Mesh-2x2_Homogeneous-blue.svg)]()
[![Clock](https://img.shields.io/badge/Clock_Architecture-GALS_CDC_(2.0G%2F1.25G%2F1.0G)-purple.svg)]()
[![Driver](https://img.shields.io/badge/Linux_Driver-Kernel_Module_+_C_Runtime-orange.svg)]()


> 🧭 **Silicon Stack Navigation:** [Architecture Home](https://github.com/ITLABSofficial) &nbsp;•&nbsp; [⬅️ Prev: Ara_Cluster_TCDM_NoC](https://github.com/ITLABSofficial/Ara_Cluster_TCDM_NoC) &nbsp;•&nbsp; **Layer 06: `Ara_16Core_2x2_SoC`** &nbsp;•&nbsp; [Next: Ara_PCIe_AI_Accelerator_64Core ➡️](https://github.com/ITLABSofficial/Ara_PCIe_AI_Accelerator_64Core)

**Ara_16Core_2x2_SoC** is an open-source, production-grade 16-core multicore AI SoC macro (Durum A architecture). It integrates **4 homogeneous compute clusters** (each containing 4 Ara VPUs and 512 KB L1 TCDM SRAM) arranged in a 2D-mesh Network-on-Chip, paired with the **OpenHBM3 Base-Die Processing-in-Memory (PIM)** subsystem, a **Globally Asynchronous Locally Synchronous (GALS)** clock architecture, and a **4-zone Power Delivery Network (PDN)** controller.

---

## 1. Architectural Highlights

```
+─────────────────────────────────────────────────────────────────────────────+
|                         ARA 16-CORE 2x2 SoC TOPOLOGY                        |
|                                                                             |
|      ┌─────────────────────────────┐   ┌─────────────────────────────┐      |
|      │ Cluster (0,0) - 4 Ara Cores │   │ Cluster (0,1) - 4 Ara Cores │      |
|      │ 512 KB TCDM | NoC Router    │◄─►│ 512 KB TCDM | NoC Router    │      |
|      └──────────────┬──────────────┘   └──────────────┬──────────────┘      |
|                     │                                 │                     |
|                     ▲ (2D-Mesh NoC Link)              ▲                     |
|                     │                                 │                     |
|      ┌──────────────┴──────────────┐   ┌──────────────┴──────────────┐      |
|      │ Cluster (1,0) - 4 Ara Cores │   │ Cluster (1,1) - 4 Ara Cores │      |
|      │ 512 KB TCDM | NoC Router    │◄─►│ 512 KB TCDM | NoC Router    │      |
|      └──────────────┬──────────────┘   └──────────────┬──────────────┘      |
|                     │                                 │                     |
|                     ▼ (GALS CDC Ingress / Egress)     ▼                     |
|      ═════════════════════════════════════════════════════════════════      |
|      OpenHBM3 SoC v2.1 Base-Die PIM Subsystem (3.2 TB/s Internal TSV)       |
|      - Decoupled True-FMA PIM Engine & Interposer Crossbar                  |
+─────────────────────────────────────────────────────────────────────────────+
```

### 1. 16.384 TFLOPS Vector Compute
- 16 parallel Ara VPU cores operating at **2.0 GHz**.
- Peak FP16 throughput: **16.384 TFLOPS** ($16\text{ cores} \times 512\text{ FLOPs/cycle} \times 2.0\text{ GHz}$).
- Peak FP8 throughput: **32.768 TFLOPS** ($1,024\text{ FLOPs/cycle}$ per core).
- Total L1 TCDM storage: **2.0 MB** across 128 conflict-free memory banks ($1.024\text{ TB/s}$ local bandwidth).

### 2. GALS Clock Domain Architecture
- **Compute Domain**: 2.0 GHz (500 ps period) for maximum systolic contraction.
- **NoC Routing Domain**: 1.25 GHz (800 ps period) for low-power packet traversal.
- **Memory / PIM Domain**: 1.0 GHz (1000 ps period) matching HBM3 base-die timing.
- Asynchronous bi-synchronous Gray-code pointer CDC FIFOs eliminate metastabilities with zero dropped beats.

### 3. 4-Zone PDN Power Controller
- Staggered power-up sequencing (8 cycles per cluster zone) reduces inrush di/dt current by $75\%$.
- 1-cycle emergency power abort safely shuts down all active arithmetic units during thermal panic.

### 4. Full Software Stack
- **Linux Kernel Driver (`driver/ara_wse_driver.c/h`)**: Provides character device `/dev/ara_wse0`, MMIO mmap, MSI-X interrupt handling, and DMA buffer allocation.
- **C User-Space Runtime (`runtime/libara_runtime.c/h`)**: Provides high-level C APIs for GEMM, RMSNorm, and SwiGLU task submission with hardware completion barriers.

---

## 2. Directory Structure

```
Ara_16Core_2x2_SoC/
├── rtl/                        # Synthesizable 16-core SoC RTL
│   ├── ara_multicore_wse_hbm3_top.sv   # Top-level SoC integration wrapper
│   ├── ara_wse_multi_cluster_top.sv    # 2x2 cluster mesh interconnection
│   ├── ara_multicore_cluster.sv        # 4-core cluster macro
│   ├── ara_noc_cdc_fifo.sv             # GALS bi-synchronous CDC FIFO
│   └── ara_pdn_stagger_ctrl.sv         # 4-zone staggered PDN controller
├── openhbm3_soc/               # Integrated Base-Die PIM IP core
├── driver/                     # Linux kernel driver module
│   ├── ara_wse_driver.c
│   └── ara_wse_driver.h
├── runtime/                    # C user-space runtime library & unit tests
│   ├── libara_runtime.c
│   ├── libara_runtime.h
│   └── test_ara_runtime.c
├── tb/                         # Hardware integration testbenches
├── scripts/                    # Simulation & timing scripts
├── INTEGRATION_GUIDE.md        # Memory map, driver installation, and PDN specs
├── BENCHMARKS.md               # Measured throughput, GALS latency, and thermal benchmarks
├── Makefile                    # Unified build automation
└── LICENSE                     # Apache 2.0 Open-Source License
```

---

## 3. Quickstart & Verification

```bash
# 1. Run all hardware simulations (FMA, GALS CDC, 16-Core Top - 100% PASS)
make test

# 2. Compile and run C user-space runtime test suite (8/8 PASS)
make runtime
./runtime/test_ara_runtime

# 3. Check ASIC synthesizability in Yosys
make synth
```

---

## 🗺️ Silicon Architecture Stack & Next-Page Navigation

This repository represents **Layer 06** of our 8-layer unified silicon supercomputing stack. You can navigate sequentially through the entire hardware hierarchy:

```
[01: PHY Layer] ➔ [02: Memory Controller] ➔ [03: Near-Memory PIM] ➔ [04: Ara Core True-FMA]
       ➔ [05: Cluster NoC] ➔ [06: 16-Core SoC] ➔ [07: 64-Core PCIe Card] ➔ [08: 16K WSE Wafer]
```

| Layer | Hardware Component | GitHub Repository | Key Architectural Specifications | Status |
|:---:|:---|:---|:---|:---:|
| **01** | **Physical Layer (PHY)** | [**`Open_HBM3_PHY`**](https://github.com/ITLABSofficial/Open_HBM3_PHY) | 1.18 TB/s HBM3E PHY, 8:1 SerDes, DFE/CTLE, 4.6 GHz WCK, DFI 5.0 | Active |
| **02** | **Memory Controller** | [**`Open_HBM3_Controller`**](https://github.com/ITLABSofficial/Open_HBM3_Controller) | 16-Pseudo-Channel DFI 5.0 Controller, SECDED ECC, Bank Interleaving | Active |
| **03** | **Near-Memory PIM** | [**`OpenHBM3_PIM_Subsystem`**](https://github.com/ITLABSofficial/OpenHBM3_PIM_Subsystem) | 3.2 TB/s Base-Die PIM Engine, FlashAttention, RMSNorm, True-FMA | Active |
| **04** | **Compute Core** | [**`Ara_Core_TrueFMA_Systolic`**](https://github.com/ITLABSofficial/Ara_Core_TrueFMA_Systolic) | 16x16 Systolic Array & 6-Stage 2.0 GHz True-FMA (1.024 TFLOPS FP16) | Active |
| **05** | **Cluster Interconnect** | [**`Ara_Cluster_TCDM_NoC`**](https://github.com/ITLABSofficial/Ara_Cluster_TCDM_NoC) | 4-Core Vector Cluster, 32-Bank TCDM SRAM (256 GB/s), 5-Port VC Router | Active |
| **06** | **Multicore SoC** | [**`Ara_16Core_2x2_SoC` (Current)**](https://github.com/ITLABSofficial/Ara_16Core_2x2_SoC) | 16-Core GALS NoC Multicore SoC, 4-Zone PDN Stagger, Linux C Driver | Active |
| **07** | **Board / Accelerator** | [**`Ara_PCIe_AI_Accelerator_64Core`**](https://github.com/ITLABSofficial/Ara_PCIe_AI_Accelerator_64Core) | 64-Core PCIe Gen5 x16 AI Card, 128 GB HBM3e Address Space (229W TDP) | Active |
| **08** | **Wafer-Scale Engine** | [**`WSE_16K_Wafer_Scale_Engine`**](https://github.com/ITLABSofficial/WSE_16K_Wafer_Scale_Engine) | 300mm Monolithic 16,384-Core Wafer Engine, 48x HBM3 (1.15 TB), 15 kW | Active |

<div align="center">

| ⬅️ **Previous Component** | 🏠 **Architecture Ecosystem** | ➡️ **Next Component** |
|:---:|:---:|:---:|
| [**⬅️ Ara_Cluster_TCDM_NoC**](https://github.com/ITLABSofficial/Ara_Cluster_TCDM_NoC) | [**ITLABSofficial Profile**](https://github.com/ITLABSofficial) | [**Ara_PCIe_AI_Accelerator_64Core ➔**](https://github.com/ITLABSofficial/Ara_PCIe_AI_Accelerator_64Core) |

</div>

---

## License

This project is licensed under the **Apache License 2.0** - see the [LICENSE](LICENSE) file for details.
