
ivp-y := ivp_cmd_hnd.o \
	 ivp_dbgfs.o \
	 ivp_driver.o \
	 ivp_err_hnd.o \
	 ivp_fw_interface.o \
	 ivp_ioctl.o \
	 ivp_opp_ctl.o \
	 ivp_queue.o \
	 ivp_utilization.o \
	 ivpbuf-core.o \
	 ivpbuf-dma-contig.o \
	 ivpbuf-dma-memcpy.o \
	 ivpbuf-heap.o \
	 ivpbuf-memops.o

ivp-y += ivp_kservice.o
ivp-y += ivp_profile.o
ivp-y += ivp_met_event.o
ivp-y += ivp_power_ctl_mt3612.o

ifneq ($(CONFIG_MTK_IVP),)
ivp-$(CONFIG_MTK_IVP_DVFS) += ivp_dvfs.o
endif

obj-$(CONFIG_MTK_IVP) += ivp.o
