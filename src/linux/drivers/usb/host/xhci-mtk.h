/*
 * Copyright (c) 2015 MediaTek Inc.
 * Author:
 *  Zhigang.Wei <zhigang.wei@mediatek.com>
 *  Chunfeng.Yun <chunfeng.yun@mediatek.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/**
 * @file xhci-mtk.h
 * MediaTek xHCI Host Controller Driver header file.
 */

/**
 * @defgroup IP_group_usbhost USB_HOST
 *   The dirver use for MTK USB Host.
 *
 *   @{
 *       @defgroup IP_group_usbhost_external EXTERNAL
 *         The external API document for MTK USB Host. \n
 *
 *         @{
 *           @defgroup IP_group_usbhost_external_function 1.function
 *               none. No extra function in MTK USB Host driver.
 *           @defgroup IP_group_usbhost_external_struct 2.Structure
 *               none. No extra Structure in MTK USB Host driver.
 *           @defgroup IP_group_usbhost_external_typedef 3.Typedef
 *               none. No extra Typedef in MTK USB Host driver.
 *           @defgroup IP_group_usbhost_external_enum 4.Enumeration
 *               none. No extra Enumeration in MTK USB Host driver.
 *           @defgroup IP_group_usbhost_external_def 5.Define
 *               none. No extra Define in MTK USB Host driver.
 *         @}
 *
 *       @defgroup IP_group_usbhost_internal INTERNAL
 *         The internal API document for MTK USB Host. \n
 *
 *         @{
 *           @defgroup IP_group_usbhost_internal_function 1.function
 *               Internal function for MTK USB Host initial.
 *           @defgroup IP_group_usbhost_internal_struct 2.Structure
 *               Internal structure used for MTK USB Host driver.
 *           @defgroup IP_group_usbhost_internal_typedef 3.Typedef
 *               none. No extra Typedef in MTK USB Host driver.
 *           @defgroup IP_group_usbhost_internal_enum 4.Enumeration
 *               Internal enumeration used for MTK USB Host driver.
 *           @defgroup IP_group_usbhost_internal_def 5.Define
 *               Internal define used for MTK USB Host driver.
 *         @}
 *    @}
 */

#ifndef _XHCI_MTK_H_
#define _XHCI_MTK_H_

#include "xhci.h"

/** @ingroup IP_group_usbhost_internal_def
 * @brief use for mtk software scheduler define.
 */
/**
 * To simplify scheduler algorithm, set a upper limit for ESIT,\n
 * if a synchromous ep's ESIT is larger than @XHCI_MTK_MAX_ESIT,\n
 * round down to the limit value, that means allocating more\n
 * bandwidth to it.
 */
#define XHCI_MTK_MAX_ESIT	64

/** @ingroup IP_group_usbhost_internal_typedef
 * @brief use for mtk HCD control bit.
 * @{
 */
/** ip_pw_ctrl0 register */
#define CTRL0_IP_SW_RST	BIT(0)

/** ip_pw_ctrl1 register */
#define CTRL1_IP_HOST_PDN	BIT(0)

/** ip_pw_ctrl2 register */
#define CTRL2_IP_DEV_PDN	BIT(0)

/** ip_pw_sts1 register */
#define STS1_IP_SLEEP_STS	BIT(30)
#define STS1_XHCI_RST		BIT(11)
#define STS1_SYS125_RST	BIT(10)
#define STS1_REF_RST		BIT(8)
#define STS1_SYSPLL_STABLE	BIT(0)

/** ip_xhci_cap register */
#define CAP_U3_PORT_NUM(p)	((p) & 0xff)
#define CAP_U2_PORT_NUM(p)	(((p) >> 8) & 0xff)

/** u3_ctrl_p register */
#define CTRL_U3_PORT_HOST_SEL	BIT(2)
#define CTRL_U3_PORT_PDN	BIT(1)
#define CTRL_U3_PORT_DIS	BIT(0)

/** u2_ctrl_p register */
#define CTRL_U2_PORT_HOST_SEL	BIT(2)
#define CTRL_U2_PORT_PDN	BIT(1)
#define CTRL_U2_PORT_DIS	BIT(0)

/** ref_ck_ctrl register */
#define REF_CK_GATE_EN		BIT(2)
#define REF_PHY_CK_GATE_EN	BIT(1)
#define REF_MAC_CK_GATE_EN	BIT(0)

#define PERI_WK_CTRL0		0x400
#define UWK_CTR0_0P_LS_PE	BIT(8)  /* posedge */
#define UWK_CTR0_0P_LS_NE	BIT(7)  /* negedge for 0p linestate*/
#define UWK_CTL1_1P_LS_C(x)	(((x) & 0xf) << 1)
#define UWK_CTL1_1P_LS_E	BIT(0)

#define PERI_WK_CTRL1		0x404
#define UWK_CTL1_IS_C(x)	(((x) & 0xf) << 26)
#define UWK_CTL1_IS_E		BIT(25)
#define UWK_CTL1_0P_LS_C(x)	(((x) & 0xf) << 21)
#define UWK_CTL1_0P_LS_E	BIT(20)
#define UWK_CTL1_IDDIG_C(x)	(((x) & 0xf) << 11)  /* cycle debounce */
#define UWK_CTL1_IDDIG_E	BIT(10) /* enable debounce */
#define UWK_CTL1_IDDIG_P	BIT(9)  /* polarity */
#define UWK_CTL1_0P_LS_P	BIT(7)
#define UWK_CTL1_IS_P		BIT(6)  /* polarity for ip sleep */

/** host usb3_mac_csr_p0 register */
#define SSUSB_USB3_MAC_CSR_P0	0x2400	/* usb3_mac_csr_p0 base address */

#define UX_LFPS_TIMING_PARAMETER	0x007c
#define UX_EXIT_T12_T11_MINUS_300NS	0x4b

#define U1_LFPS_TIMING_PARAMETER_2	0x0080
#define U1_EXIT_T12_T10			0x008a
#define U1_EXIT_T13_T11			0x8300

/** host usb3_sys_csr_p0 register */
#define LINK_HP_TIMER			0x0200
#define PHP_TIMEOUT_VALUE		0x18

#define LINK_PM_TIMER			0x0208
#define PM_ENTRY_TIMEOUT_VALUE		0x0900
#define PM_LC_TIMEOUT_VALUE		0x000a

/**
 * @}
 */

/** @ingroup IP_group_usbhost_internal_def
 * @brief use for mtk HW SUB ID number define.
 */
#define HOST_SSUSB_HW_SUB_ID	0x20180711

/** @ingroup IP_group_usbhost_internal_struct
* @brief schedule information for TT .
*/
/**
 * ss_bit_map: index bit map of XHCI_MTK_MAX_ESIT for ss (start-split)\n
 * cs_bit_map: index bit map of XHCI_MTK_MAX_ESIT for cs (complete-split)\n
 * ep_list: eps list using this TT\n
 * usb_tt: usb TT struct\n
 * tt_port: TT port number\n
 */
struct mu3h_sch_tt {
	DECLARE_BITMAP(ss_bit_map, XHCI_MTK_MAX_ESIT);
	DECLARE_BITMAP(cs_bit_map, XHCI_MTK_MAX_ESIT);
	struct list_head ep_list; /* Endpoints using this TT */
	struct usb_tt *usb_tt;
	int tt_port; /* TT port number */
};

/** @ingroup IP_group_usbhost_internal_struct
* @brief schedule information for bandwidth domain.
*/
/**
 * bus_bw: array to keep track of bandwidth already used at each uframes\n
 * bw_ep_list: eps in the bandwidth domain\n
 *\n
 * treat a HS root port as a bandwidth domain, but treat a SS root port as\n
 * two bandwidth domains, one for IN eps and another for OUT eps.
 */
struct mu3h_sch_bw_info {
	u32 bus_bw[XHCI_MTK_MAX_ESIT];
	struct list_head bw_ep_list;
};

/** @ingroup IP_group_usbhost_internal_struct
* @brief schedule information for endpoint.
*/
/**
 * struct mu3h_sch_ep_info: schedule information for endpoint
 * @verbatim
 * esit: unit is 125us, equal to 2 << Interval field in ep-context.
 * num_budget_microframes: number of continuous uframes
 *             (repeat==1) scheduled within the interval.
 * bw_cost_per_microframe: bandwidth cost per microframe.
 * bw_budget_table: bandwidth budget table.
 * endpoint: linked into bandwidth domain which it belongs to.
 * tt_endpoint: linked into mu3h_sch_tt list which it belongs to
 * ep: address of usb_host_endpoint struct.
 * ep_ctx: address of xhci_ep_ctx struct.
 * offset: which uframe of the interval that transfer should be
 *              scheduled first time within the interval.
 * repeat: the time gap between two uframes that transfers are
 *              scheduled within a interval. in the simple algorithm, only
 *              assign 0 or 1 to it; 0 means using only one uframe in a
 *              interval, and 1 means using @num_budget_microframes
 *              continuous uframes.
 * pkts: number of packets to be transferred in the scheduled uframes.
 * cs_count: number of CS that host will trigger.
 * burst_mode: burst mode for scheduling. 0: normal burst mode,
 *              distribute the bMaxBurst+1 packets for a single burst
 *              according to @pkts and @repeat, repeate the burst multiple
 *              times; 1: distribute the (bMaxBurst+1)*(Mult+1) packets
 *              according to @pkts and @repeat. normal mode is used by
 *              default.
 * @endverbatim
 */
struct mu3h_sch_ep_info {
	u32 esit;
	u32 num_budget_microframes;
	u32 bw_cost_per_microframe;
	u32 *bw_budget_table;
	struct list_head endpoint;
	struct list_head tt_endpoint;
	void *ep;
	struct xhci_ep_ctx *ep_ctx;
	/*
	 * mtk xHCI scheduling information put into reserved DWs
	 * in ep context
	 */
	u32 offset;
	u32 repeat;
	u32 pkts;
	u32 cs_count;
	u32 burst_mode;
};

 /** @ingroup IP_group_usbhost_internal_enum
 * @brief for MTK SW scheduler EP operation state
 *
 */
enum mu3h_sch_ep_operation {
	ADD_EP = 0,
	DROP_EP,
};

/** @ingroup IP_group_usbhost_internal_def
 * @brief define for MTK HCD support maximum port number.
 * @{
 */
/** MTK HCD support maximum U3 port */
#define MU3C_U3_PORT_MAX 4
/** MTK HCD support maximum U2 port */
#define MU3C_U2_PORT_MAX 5

/**
 * @}
 */

/** @ingroup IP_group_usbhost_internal_struct
* @brief MTK ssusb ip port control registers.
*/
/**
 * struct mu3c_ippc_regs: MTK ssusb ip port control registers\n
 * ip_pw_ctr0~3: ip power and clock control registers\n
 * ip_pw_sts1~2: ip power and clock status registers\n
 * ip_xhci_cap: ip xHCI capability register\n
 * u3_ctrl_p[x]: ip usb3 port x control register, only low 4bytes are used\n
 * u2_ctrl_p[x]: ip usb2 port x control register, only low 4bytes are used\n
 * u2_phy_pll: usb2 phy pll control register\n
 * ref_ck_ctrl: usb Host ref_clk control register\n
 */
struct mu3c_ippc_regs {
	__le32 ip_pw_ctr0;
	__le32 ip_pw_ctr1;
	__le32 ip_pw_ctr2;
	__le32 ip_pw_ctr3;
	__le32 ip_pw_sts1;
	__le32 ip_pw_sts2;
	__le32 reserved0[3];
	__le32 ip_xhci_cap;
	__le32 reserved1[2];
	__le64 u3_ctrl_p[MU3C_U3_PORT_MAX];
	__le64 u2_ctrl_p[MU3C_U2_PORT_MAX];
	__le32 reserved2;
	__le32 u2_phy_pll;		/* 0x7c */
	__le32 reserved3[3];
	__le32 ref_ck_ctrl;		/* 0x8c */
	__le32 reserved4[5];	/* 0x90 ~ 0xA0 */
	__le32 ssusb_hw_sub_id;	/* 0xA4 */
	__le32 reserved5[22];	/* 0xA8 ~ 0xFC */

};

/** @ingroup IP_group_usbhost_internal_struct
* @brief The information of MTK HCD controller.
*/
/**
 * device *dev: device struct of MTK HCD\n
 * usb_hcd *hcd: usb_hcd struct of MTK HCD\n
 * sch_array: mu3h_sch_bw_info struct of MTK HCD\n
 * ippc_regs: mu3c_ippc_regs struct of MTK HCD\n
 * has_ippc: bool value "true" index ippc register had got resource.\n
 * num_u2_ports: current MTK HCD support u2 ports.\n
 * num_u3_ports: current MTK HCD support u3 ports.\n
 * vusb33: regulator struct of MTK HCD vbus3.3\n
 * vbus:  regulator struct of MTK HCD vbus\n
 * sys_clk: clk struct of MTK HCD system and mac clock\n
 * ref_clk: clk struct of MTK HCD system reference clock\n
 * wk_deb_p0: clk struct of MTK HCD port0's wakeup debounce clock\n
 * wk_deb_p1: clk struct of MTK HCD port1's wakeup debounce clock\n
 * pericfg: regmap struct of MTK HCD\n
 * phys: phy struct of MTK HCD\n
 * num_phys: current PHY numbers of MTK HCD\n
 * wakeup_src: wakeup source of MTK HCD\n
 * lpm_support: index support lpm of MTK HCD\n
 */
struct xhci_hcd_mtk {
	struct device *dev;
	struct usb_hcd *hcd;
	struct mu3h_sch_bw_info *sch_array;
	struct mu3c_ippc_regs __iomem *ippc_regs;
	bool has_ippc;
	int num_u2_ports;
	int num_u3_ports;
	struct regulator *vusb33;
	struct regulator *vbus;
	struct clk *sys_clk;	/* sys and mac clock */
	struct clk *xhci_clk;
	struct clk *ref_clk;
	struct clk *mcu_clk;
	struct clk *dma_clk;
	struct clk *wk_deb_p0;	/* port0's wakeup debounce clock */
	struct clk *wk_deb_p1;
	struct regmap *pericfg;
	struct phy **phys;
	int num_phys;
	int wakeup_src;
	bool lpm_support;
	void  __iomem *mac_regs;
};

 /** @ingroup IP_group_usbhost_internal_function
 * @par Description
 *     Get xhci_hcd_mtk struct form usb_hcd struct.
 * @param[in]
 *     hcd: usb_hcd struct pointer.\n
 * @return
 *     return xhci_hcd_mtk struct pointer.
 * @par Boundary case and Limitation
 *     none.
 * @par Error case and Error handling
 *     none.
 * @par Call graph and Caller graph (refer to the graph below)
 * @par Refer to the source code
 */
static inline struct xhci_hcd_mtk *hcd_to_mtk(struct usb_hcd *hcd)
{
	return dev_get_drvdata(hcd->self.controller);
}

#if IS_ENABLED(CONFIG_USB_XHCI_MTK)
int xhci_mtk_sch_init(struct xhci_hcd_mtk *mtk);
void xhci_mtk_sch_exit(struct xhci_hcd_mtk *mtk);
int xhci_mtk_add_ep_quirk(struct usb_hcd *hcd, struct usb_device *udev,
		struct usb_host_endpoint *ep);
void xhci_mtk_drop_ep_quirk(struct usb_hcd *hcd, struct usb_device *udev,
		struct usb_host_endpoint *ep);

void xhci_mtk_retimer(struct usb_hcd *hcd);
#else
static inline int xhci_mtk_add_ep_quirk(struct usb_hcd *hcd,
	struct usb_device *udev, struct usb_host_endpoint *ep)
{
	return 0;
}

static inline void xhci_mtk_drop_ep_quirk(struct usb_hcd *hcd,
	struct usb_device *udev, struct usb_host_endpoint *ep)
{
}

static inline void xhci_mtk_retimer(struct usb_hcd *hcd)
{
}


#endif

#endif		/* _XHCI_MTK_H_ */
