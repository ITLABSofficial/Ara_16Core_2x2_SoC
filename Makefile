# ==============================================================================
# ARA 16-CORE 2x2 SoC (DURUM A) MAKEFILE
# ==============================================================================

IVERILOG = iverilog -g2012
VVP = vvp
YOSYS = yosys
CC = gcc

PIM_ALL = openhbm3_soc/rtl/pim/ara_hbm3_pim_true_fma.sv \
          openhbm3_soc/rtl/pim/ara_hbm3_pim_decoupled_fsm.sv \
          openhbm3_soc/rtl/pim/ara_hbm3_interposer_p2p.sv \
          openhbm3_soc/rtl/pim/ara_openhbm3_soc_top.sv

RTL_ALL = rtl/ara_multicore_true_fma_6stage.sv \
          rtl/ara_systolic_tensor_unit.sv \
          rtl/ara_tcdm_bank_interconnect.sv \
          rtl/ara_multicore_cluster.sv \
          rtl/ara_noc_fifo.sv \
          rtl/ara_noc_cdc_fifo.sv \
          rtl/ara_noc_mesh_router.sv \
          rtl/ara_noc_ni.sv \
          rtl/ara_noc_defect_bypass.sv \
          rtl/ara_barrier_tree.sv \
          rtl/ara_pdn_stagger_ctrl.sv \
          rtl/ara_host_cmd_dispatcher.sv \
          rtl/ara_wse_multi_cluster_top.sv \
          rtl/ara_multicore_wse_hbm3_top.sv \
          $(PIM_ALL)

.PHONY: all sim_fma sim_top sim_gals test test_runtime test_all synth clean driver runtime

all: sim_fma sim_gals sim_top

sim_fma: rtl/ara_multicore_true_fma_6stage.sv tb/tb_ara_multicore_fma_6stage.sv
	$(IVERILOG) -o sim_fma rtl/ara_multicore_true_fma_6stage.sv tb/tb_ara_multicore_fma_6stage.sv

sim_gals: rtl/ara_noc_fifo.sv rtl/ara_noc_cdc_fifo.sv rtl/ara_pdn_stagger_ctrl.sv tb/tb_ara_noc_cdc_gals.sv
	$(IVERILOG) -o sim_gals rtl/ara_noc_fifo.sv rtl/ara_noc_cdc_fifo.sv rtl/ara_pdn_stagger_ctrl.sv tb/tb_ara_noc_cdc_gals.sv

sim_top: $(RTL_ALL) tb/tb_ara_multicore_wse_hbm3_top.sv
	$(IVERILOG) -o sim_top $(RTL_ALL) tb/tb_ara_multicore_wse_hbm3_top.sv

test: sim_fma sim_gals sim_top
	@echo "=== [1/3] RUNNING TRUE-FMA TEST ==="
	$(VVP) sim_fma
	@echo "=== [2/3] RUNNING GALS CDC TEST ==="
	$(VVP) sim_gals
	@echo "=== [3/3] RUNNING 16-CORE TOP SIMULATION ==="
	$(VVP) sim_top

test_runtime: runtime
	@echo "=== [4/4] RUNNING C RUNTIME DRIVER TESTS ==="
	./runtime/test_ara_runtime

test_all: test test_runtime
	@echo "========================================================"
	@echo "  ALL ARA 16-CORE SOC REGRESSION SUITES PASSED (0 ERRORS)! "
	@echo "========================================================"

synth:
	$(YOSYS) -p "read_verilog -sv rtl/ara_multicore_true_fma_6stage.sv rtl/ara_systolic_tensor_unit.sv rtl/ara_tcdm_bank_interconnect.sv rtl/ara_multicore_cluster.sv rtl/ara_noc_mesh_router.sv rtl/ara_noc_fifo.sv rtl/ara_noc_defect_bypass.sv rtl/ara_barrier_tree.sv rtl/ara_pdn_stagger_ctrl.sv; hierarchy -check -top ara_multicore_cluster; proc; opt; check -assert"

runtime:
	$(MAKE) -C runtime

clean:
	rm -f sim_fma sim_gals sim_top *.vcd
	$(MAKE) -C runtime clean
