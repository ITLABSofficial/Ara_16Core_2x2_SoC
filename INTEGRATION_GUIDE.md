# Ara_16Core_2x2_SoC: Hardware & Software Integration Guide
**16-Core GALS SoC, Memory Apertures, Linux Kernel Driver, and Power Delivery**

---

## 1. Top-Level Module Instantiation

The top-level SoC macro `ara_multicore_wse_hbm3_top` connects the host system bus to the 16-core 2x2 mesh, the GALS CDC bridges, and the OpenHBM3 PIM base die.

### SystemVerilog Instantiation Template

```systemverilog
ara_multicore_wse_hbm3_top #(
    .NUM_CLUSTERS    (4),      // 2x2 mesh = 4 clusters
    .CORES_PER_CLUST (4),      // 4 Ara VPUs per cluster = 16 cores
    .TCDM_SIZE_KB    (512),    // 512 KB per cluster = 2 MB total L1
    .HBM_CHANNELS    (16)      // 16 Pseudo-Channels
) u_16core_soc (
    // Primary Clock Domains
    .clk_compute_i   (clk_2000mhz),        // 2.0 GHz Ara Vector Core Clock
    .clk_noc_i       (clk_1250mhz),        // 1.25 GHz NoC Mesh Clock
    .clk_hbm_i       (clk_1000mhz),        // 1.0 GHz HBM3 PIM Base-Die Clock
    .rst_async_n     (sys_rst_n),          // Master asynchronous reset

    // Host Memory-Mapped Control Interface (AXI5-Lite / CSR)
    .s_axi_csr_awaddr (csr_awaddr[15:0]),
    .s_axi_csr_awvalid(csr_awvalid),
    .s_axi_csr_awready(csr_awready),
    .s_axi_csr_wdata  (csr_wdata[31:0]),
    .s_axi_csr_wvalid (csr_wvalid),
    .s_axi_csr_wready (csr_wready),
    .s_axi_csr_araddr (csr_araddr[15:0]),
    .s_axi_csr_arvalid(csr_arvalid),
    .s_axi_csr_arready(csr_arready),
    .s_axi_csr_rdata  (csr_rdata[31:0]),
    .s_axi_csr_rvalid (csr_rvalid),
    .s_axi_csr_rready (csr_rready),

    // Host Direct High-Speed AXI5 Master (Direct TCDM/HBM Access)
    .m_axi_dram_awaddr(dram_awaddr[31:0]),
    .m_axi_dram_awvalid(dram_awvalid),
    .m_axi_dram_awready(dram_awready),
    .m_axi_dram_wdata (dram_wdata[255:0]),
    .m_axi_dram_wvalid(dram_wvalid),
    .m_axi_dram_wready(dram_wready),

    // Telemetry & Interrupts
    .soc_irq_o       (host_msi_interrupt), // Task completion interrupt
    .soc_alert_o     (thermal_power_alert) // Emergency thermal/voltage alert
);
```

---

## 2. System Memory Map & Apertures

The SoC exposes a clean, non-overlapping memory aperture map to host software:

```
+─────────────────────────────────────────────────────────────────────────────+
|                         ARA 16-CORE SYSTEM MEMORY MAP                       |
|                                                                             |
|  Base Address   Size        Aperture Name       Access   Description        |
|  ────────────   ─────────   ─────────────────   ──────   ───────────        |
|  0x0000_0000    64 KB       CSR MMIO Registers  RW       Task descriptors   |
|  0x0010_0000    2048 KB     Shared L1 TCDM      RW       Local scratchpad   |
|  0x1000_0000    32 GB       HBM3 DRAM Window    RW       High-bandwidth mem |
|  0x8000_0000    --          PIM Control Space   RW       Base-die offload   |
+─────────────────────────────────────────────────────────────────────────────+
```

---

## 3. Voltage Regulator Specifications & Power Supply Network

The 16-core SoC dissipates up to $34.0\text{ W}$ under concurrent systolic compute and HBM3 streaming.

| Power Supply Rail | Nominal Voltage | Tolerance | Maximum Ripple | Transient Current Step | Function |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **`VDD_CORE`** | **0.75 V** | **$\pm 3.0\%$ ($\pm 22.5\text{ mV}$)** | **$< 15\text{ mV}$** | **$< 15\text{ ns}$ (2A to 45A)** | 16x Ara VPUs, systolic matrices. |
| **`VDD_NOC`** | **0.75 V** | **$\pm 3.5\%$ ($\pm 26.0\text{ mV}$)** | **$< 12\text{ mV}$** | **$< 20\text{ ns}$ (1A to 8A)** | 2x2 NoC mesh routers and FIFOs. |
| **`VDD_PIM`** | **0.80 V** | **$\pm 3.0\%$ ($\pm 24.0\text{ mV}$)** | **$< 12\text{ mV}$** | **$< 15\text{ ns}$ (0 to 12A)** | Base-Die PIM arithmetic units. |
| **`VDD_DRAM`** | **1.10 V** | **$\pm 3.0\%$ ($\pm 33.0\text{ mV}$)** | **$< 15\text{ mV}$** | **$< 50\text{ ns}$** | HBM3 DRAM core stacks. |

### 4-Zone Power Grid Sequencing
- The PDN controller implements 4 independent power zones corresponding to the 4 clusters:
  $$\text{Zone 0 (Cluster 0,0)} \xrightarrow{8\text{ cycles}} \text{Zone 1 (0,1)} \xrightarrow{8\text{ cycles}} \text{Zone 2 (1,0)} \xrightarrow{8\text{ cycles}} \text{Zone 3 (1,1)}$$
- This staggered activation prevents $L \cdot \frac{di}{dt}$ inductive ground bounce, keeping rail sag below $20\text{ mV}$.

---

## 4. Software Stack Integration

### 1. Linux Kernel Driver Installation
```bash
cd driver
make
sudo insmod ara_wse_driver.ko
# Verify character device node is created
ls -la /dev/ara_wse0
```

### 2. C User-Space Runtime Library (`libara_runtime`)
Applications interact with the hardware via `libara_runtime`:
```c
#include "libara_runtime.h"

int main() {
    ara_device_t dev;
    ara_init(&dev, 0); // Open /dev/ara_wse0

    // Submit 16x16 Systolic FP16 GEMM task
    ara_task_desc_t task = {
        .opcode   = ARA_OP_GEMM,
        .data_fmt = ARA_FMT_FP16,
        .k_len    = 16,
        .act_mode = ARA_ACT_NONE
    };
    ara_submit_task(&dev, &task);
    ara_wait_completion(&dev, 1000); // Wait with 1000 us timeout

    ara_close(&dev);
    return 0;
}
```
