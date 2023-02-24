/*
 * Copyright (c) 2015 Pengutronix, Sascha Hauer <kernel@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/clk.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/mfd/syscon.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/pm_domain.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>
#include <linux/soc/mediatek/infracfg.h>
#include <linux/delay.h>

#include <dt-bindings/power/mt3612-power.h>
#include <pwr_ctrl_reg.h>
#include <linux/soc/mediatek/infracfg_ao_reg.h>

#define PWR_RST_B_BIT			BIT(0)
#define PWR_ISO_BIT				BIT(1)
#define PWR_ON_BIT				BIT(2)
#define PWR_ON_2ND_BIT			BIT(3)
#define PWR_CLK_DIS_BIT			BIT(4)
#define SRAM_CKISO_BIT			BIT(5)
#define SRAM_ISOINT_BIT			BIT(6)

#define MAX_CLKS		9

enum clk_id {
	CLK_NONE,
	CLK_MM_CORE,
	CLK_MM_CMM,
	CLK_CAM_SIDE0,
	CLK_CAM_SIDE1,
	CLK_IMG_SIDE0,
	CLK_IMG_SIDE1,
	CLK_CAM_IMG_GZ0,
	CLK_CAM_IMG_GZ1,
	CLK_MM_SYSRAM2,
	CLK_MM_SYSRAM1,
	CLK_MM_SYSRAM0,
	CLK_DP,
	CLK_IPU_IF,
	CLK_DSP,
	CLK_DSP1,
	CLK_DSP2,
	CLK_DSP3,
	CLK_VPU_VCORE_AHB,
	CLK_VPU_VCORE_AXI,
	CLK_VPU_CONN_AHB,
	CLK_VPU_CONN_AXI,
	CLK_VPU_CONN_VPU,
	CLK_VPU_CORE0_VPU,
	CLK_VPU_CORE0_AXI,
	CLK_VPU_CORE1_VPU,
	CLK_VPU_CORE1_AXI,
	CLK_VPU_CORE2_VPU,
	CLK_VPU_CORE2_AXI,
	CLK_MFG,
	CLK_MM_CORE_LARB0,
	CLK_MM_CORE_LARB1,
	CLK_MM_CORE_LARB15,
	CLK_MM_CORE_LARB16,
	CLK_MM_CORE_SMI0,
	CLK_MM_CORE_SMI1,
	CLK_MM_CORE_SMI15,
	CLK_MM_CORE_SMI16,
	CLK_MM_CMM_SMI_COM,
	CLK_MM_CMM_SMI3,
	CLK_MM_CMM_SMI21,
	CLK_MM_CMM_SMI2,
	CLK_MM_CMM_SMI20,
	CLK_MM_GAZE_SMI4,
	CLK_MM_GAZE_SMI22,
	CLK_CAM_S0_SYSRAM_LARB,
	CLK_CAM_S0_LARB,
	CLK_CAM_S1_SYSRAM_LARB,
	CLK_CAM_S1_LARB,
	CLK_IMG_S0_SYSRAM_LARB_1,
	CLK_IMG_S0_SYSRAM_LARB,
	CLK_IMG_S0_LARB,
	CLK_IMG_S1_SYSRAM_LARB_1,
	CLK_IMG_S1_SYSRAM_LARB,
	CLK_IMG_S1_LARB,
	CLK_CAM_GZ0_SYSRAM_LARB,
	CLK_CAM_GZ0_LARB,
	CLK_CAM_GZ1_SYSRAM_LARB,
	CLK_CAM_GZ1_LARB,
	CLK_MAX,
};

static const char * const clk_names[] = {
	NULL,
	"mm_core",
	"mm_cmm",
	"cam_side0",
	"cam_side1",
	"img_side0",
	"img_side1",
	"cam_img_gz0",
	"cam_img_gz1",
	"mm_sysram2",
	"mm_sysram1",
	"mm_sysram0",
	"dp",
	"ipu_if",
	"dsp",
	"dsp1",
	"dsp2",
	"dsp3",
	"vpu-vcore-ahb",
	"vpu-vcore-axi",
	"vpu-conn-ahb",
	"vpu-conn-axi",
	"vpu-conn-vpu",
	"vpu-core0-vpu",
	"vpu-core0-axi",
	"vpu-core1-vpu",
	"vpu-core1-axi",
	"vpu-core2-vpu",
	"vpu-core2-axi",
	"mfg",
	"mm-core-larb0",
	"mm-core-larb1",
	"mm-core-larb15",
	"mm-core-larb16",
	"mm-core-smi0",
	"mm-core-smi1",
	"mm-core-smi15",
	"mm-core-smi16",
	"mm-cmm-smi-com",
	"mm-cmm-smi3",
	"mm-cmm-smi21",
	"mm-cmm-smi2",
	"mm-cmm-smi20",
	"mm-gaze-smi4",
	"mm-gaze-smi22",
	"cam-s0-sysram-larb",
	"cam-s0-larb",
	"cam-s1-sysram-larb",
	"cam-s1-larb",
	"img-s0-sysram-larb-1",
	"img-s0-sysram-larb",
	"img-s0-larb",
	"img-s1-sysram-larb-1",
	"img-s1-sysram-larb",
	"img-s1-larb",
	"cam-gz0-sysram-larb",
	"cam-gz0-larb",
	"cam-gz1-sysram-larb",
	"cam-gz1-larb",
	NULL,
};

struct scp_domain_data {
	const char *name;
	u32 sta_mask;
	int ctl_offs;
	int sram_ctl_offs;
	u32 sram_pdn_bits;
	u32 sram_pdn_ack_bits;
	u32 sram_pdn1_bits;
	u32 sram_pdn1_ack_bits;
	u32 sram_pdn2_bits;
	u32 sram_pdn2_ack_bits;
	u32 sram_sleep_bits;
	u32 si0_ctl_offs;
	u32 si0_ctl_mask;
	u32 bus_prot_offs;
	u32 bus_protsta_offs;
	u32 bus_prot_mask;
	u32 bus_prot1_offs;
	u32 bus_protsta1_offs;
	u32 bus_prot1_mask;
	u32 bus_prot2_offs;
	u32 bus_protsta2_offs;
	u32 bus_prot2_mask;
	u32 bus_prot3_offs;
	u32 bus_protsta3_offs;
	u32 bus_prot3_mask;
	int ext_buck_offs;
	u32 ext_buck_bits;
	spinlock_t *ext_buck_lock;
	int els_iso_offs;
	int dp_phy_offs;
	u32 dp_phy_bits;
	u32 els_iso_bits;
	u32 ini_clk_num;
	u32 max_clk_num;
	enum clk_id clk_id[MAX_CLKS];
	bool active_wakeup;
	bool is_pwr_sts_vpu;
	bool is_pwr_sts_vpu_012;
	bool is_pwr_mfg1;
	bool flag_pwr_seq;
	bool flag_pwr_buck_seq;
	bool flag_pwr_els_seq;
	bool flag_rst_seq;
	bool flag_rst_dp_phy_seq;
	bool flag_sram_seq;
	bool flag_sram_dp_sleep_seq;
	bool flag_bus_seq;
};

struct scp;

struct scp_domain {
	struct generic_pm_domain genpd;
	struct scp *scp;
	struct clk *clk[MAX_CLKS];
	const struct scp_domain_data *data;
	struct regulator *supply;
};

struct scp_ctrl_reg {
	uint32_t pwr_sta_offs;
	uint32_t pwr_sta2nd_offs;
	uint32_t cpu_pwr_sta_offs;
	uint32_t cpu_pwr_sta2nd_offs;
};

struct scp {
	struct scp_domain *domains;
	struct genpd_onecell_data pd_data;
	struct device *dev;
	void __iomem *base;
	struct regmap *infracfg;
	struct scp_ctrl_reg ctrl_reg;
};

struct scp_subdomain {
	int origin;
	int subdomain;
};

struct scp_soc_data {
	const struct scp_domain_data *domains;
	int num_domains;
	const struct scp_subdomain *subdomains;
	int num_subdomains;
	const struct scp_ctrl_reg reg;
};

static int scpsys_domain_is_on(struct scp_domain *scpd)
{
	struct scp *scp = scpd->scp;
	uint32_t status, status2;

	status = readl(scp->base + scp->ctrl_reg.pwr_sta_offs) &
		      scpd->data->sta_mask;
	status2 = readl(scp->base + scp->ctrl_reg.pwr_sta2nd_offs) &
		       scpd->data->sta_mask;
	/*
	 * A domain is on when both status bits are set. If only one is set
	 * return an error. This happens while powering up a domain
	 */

	if (status && status2)
		return true;
	if (!status && !status2)
		return false;

	return -EINVAL;
}

static int scpsys_power_on(struct generic_pm_domain *genpd)
{
	struct scp_domain *scpd = container_of(genpd, struct scp_domain,
					      genpd);
	struct scp *scp = scpd->scp;
	unsigned long timeout;
	bool expired;
	void __iomem *ctl_addr = scp->base + scpd->data->ctl_offs;
	void __iomem *sram_ctl_addr;
	void __iomem *tmp_addr;
	u32 sram_pdn1_ack, sram_pdn2_ack, sram_pdn_ack =
	    scpd->data->sram_pdn_ack_bits;
	u32 val = 0, val1;
	int ret = 0;
	int i;

	if (scpd->supply) {
		ret = regulator_enable(scpd->supply);
		if (ret)
			return ret;
	}

	for (i = 0; i < scpd->data->ini_clk_num; i++) {
		if (!scpd->clk[i])
			break;
		ret = clk_prepare_enable(scpd->clk[i]);
		if (ret) {
			for (--i; i >= 0; i--)
				clk_disable_unprepare(scpd->clk[i]);

			goto err_clk;
		}
	}

	/* Set PROJECT_CODE = 0x0b160000 and BCLK_CG_EN = 1 */
	tmp_addr = scp->base + POWERON_CONFIG_EN;
	val1 = PROJECT_CODE | BCLK_CG_EN;
	writel(val1, tmp_addr);

	if (scpd->data->flag_pwr_seq) {
		if (scpd->data->flag_pwr_buck_seq) {
			/* Set EXT_BUCK_ISO = 0 */
			tmp_addr = scp->base + scpd->data->ext_buck_offs;
			if (scpd->data->ext_buck_lock)
				spin_lock(scpd->data->ext_buck_lock);
			val1 = readl(tmp_addr);
			val1 &= ~scpd->data->ext_buck_bits;
			writel(val1, tmp_addr);
			if (scpd->data->ext_buck_lock)
				spin_unlock(scpd->data->ext_buck_lock);
		}

		/* Set PWR_ON = 1 */
		val = readl(ctl_addr);
		val |= PWR_ON_BIT;
		writel(val, ctl_addr);
		/* Set PWR_ON_2ND = 1 */
		val |= PWR_ON_2ND_BIT;
		writel(val, ctl_addr);

		if (scpd->data->flag_pwr_els_seq) {
			/* Set ELS_ISO_EN_CON = 0 */
			tmp_addr = scp->base + scpd->data->els_iso_offs;
			val1 = readl(tmp_addr);
			val1 &= ~scpd->data->els_iso_bits;
			writel(val1, tmp_addr);
		}

		/* wait until PWR_ACK = 1 and PWR_ACK_2ND = 1 */
		timeout = jiffies + HZ;
		expired = false;
		while (1) {
			ret = scpsys_domain_is_on(scpd);
			if (ret > 0)
				break;

			if (expired) {
				ret = -ETIMEDOUT;
				goto err_pwr_ack;
			}

			cpu_relax();

			if (time_after(jiffies, timeout))
				expired = true;
		}
	}

	if (scpd->data->flag_rst_seq) {
		/* Set PWR_CLK_DIS = 0 */
		val &= ~PWR_CLK_DIS_BIT;
		writel(val, ctl_addr);
		/* Set PWR_ISO = 0 */
		val &= ~PWR_ISO_BIT;
		writel(val, ctl_addr);
		/* Set PWR_RST_B = 1 */
		val |= PWR_RST_B_BIT;
		writel(val, ctl_addr);
		if (scpd->data->flag_sram_dp_sleep_seq) {
			/* Set SRAM_SLEEP_B = 1 */
			val = readl(ctl_addr);
			if (!(val & scpd->data->sram_sleep_bits)) {
				val = readl(ctl_addr);
				val |= BIT(12);
				writel(val, ctl_addr);
				val = readl(ctl_addr);
				val |= BIT(13);
				writel(val, ctl_addr);
				val = readl(ctl_addr);
				val |= BIT(14);
				writel(val, ctl_addr);
				val = readl(ctl_addr);
				val |= BIT(15);
				writel(val, ctl_addr);
				udelay(1);

				/* Set SRAM_ISOINT_BIT = 1 */
				val |= SRAM_ISOINT_BIT;
				writel(val, ctl_addr);
				udelay(1);
				/* Set SRAM_CKISO_BIT = 0 */
				val &= ~SRAM_CKISO_BIT;
				writel(val, ctl_addr);
			}
		}
	}

	if (scpd->data->flag_sram_seq) {
		/* wait until SRAM_PDN_ACK all 0 */
		if (sram_pdn_ack) {
			if (scpd->data->is_pwr_sts_vpu) {
				sram_ctl_addr = scp->base +
						scpd->data->sram_ctl_offs;
				sram_pdn1_ack = scpd->data->sram_pdn1_ack_bits;
				sram_pdn2_ack = scpd->data->sram_pdn2_ack_bits;
				val = readl(sram_ctl_addr);
				/* Set SRAM_PDN = 0 */
				val &= ~scpd->data->sram_pdn_bits;
				writel(val, sram_ctl_addr);
				timeout = jiffies + HZ;
				expired = false;
				while (readl(sram_ctl_addr) & sram_pdn_ack) {
					if (expired) {
						ret = -ETIMEDOUT;
						goto err_pwr_ack;
					}
					cpu_relax();
					if (time_after(jiffies, timeout))
						expired = true;
				}
				val = readl(sram_ctl_addr);
				/* Set SRAM_PDN = 0 */
				val &= ~scpd->data->sram_pdn1_bits;
				writel(val, sram_ctl_addr);
				timeout = jiffies + HZ;
				expired = false;
				while (readl(sram_ctl_addr) & sram_pdn1_ack) {
					if (expired) {
						ret = -ETIMEDOUT;
						goto err_pwr_ack;
					}
					cpu_relax();
					if (time_after(jiffies, timeout))
						expired = true;
				}
				val = readl(sram_ctl_addr);
				/* Set SRAM_PDN = 0 */
				val &= ~scpd->data->sram_pdn2_bits;
				writel(val, sram_ctl_addr);
				timeout = jiffies + HZ;
				expired = false;
				while (readl(sram_ctl_addr) & sram_pdn2_ack) {
					if (expired) {
						ret = -ETIMEDOUT;
						goto err_pwr_ack;
					}
					cpu_relax();
					if (time_after(jiffies, timeout))
						expired = true;
				}
				if (scpd->data->is_pwr_sts_vpu_012) {
					for (i = 0; i < 5; i++) {
						val = readl(sram_ctl_addr);
						val &= ~(1 << (i+15));
						writel(val, sram_ctl_addr);
					}
				}
			} else {
				val = readl(ctl_addr);
				/* Set SRAM_PDN = 0 */
				val &= ~scpd->data->sram_pdn_bits;
				writel(val, ctl_addr);
				timeout = jiffies + HZ;
				expired = false;
				while (readl(ctl_addr) & sram_pdn_ack) {
					if (expired) {
						ret = -ETIMEDOUT;
						goto err_pwr_ack;
					}
					cpu_relax();
					if (time_after(jiffies, timeout))
						expired = true;
				}
			}
		}
	}

	if (scpd->data->flag_rst_dp_phy_seq) {
		/* Set DP_PHY_PWR_RST_B = 1 */
		tmp_addr = scp->base + scpd->data->dp_phy_offs;
		val1 = readl(tmp_addr);
		val1 |= scpd->data->dp_phy_bits;
		writel(val1, tmp_addr);
	}

	if (scpd->data->flag_bus_seq) {
		if (scpd->data->bus_prot3_mask) {
			ret = mtk_infracfg_clear_bus_protection(scp->infracfg,
				scpd->data->bus_prot3_offs,
				scpd->data->bus_protsta3_offs,
				scpd->data->bus_prot3_mask);
			if (ret)
				goto err_pwr_ack;
		}
		if (scpd->data->bus_prot2_mask) {
			ret = mtk_infracfg_clear_bus_protection(scp->infracfg,
				scpd->data->bus_prot2_offs,
				scpd->data->bus_protsta2_offs,
				scpd->data->bus_prot2_mask);
			if (ret)
				goto err_pwr_ack;
		}
		if (scpd->data->bus_prot1_mask) {
			ret = mtk_infracfg_clear_bus_protection(scp->infracfg,
				scpd->data->bus_prot1_offs,
				scpd->data->bus_protsta1_offs,
				scpd->data->bus_prot1_mask);
			if (ret)
				goto err_pwr_ack;
		}
		if (scpd->data->bus_prot_mask) {
			ret = mtk_infracfg_clear_bus_protection(scp->infracfg,
				scpd->data->bus_prot_offs,
				scpd->data->bus_protsta_offs,
				scpd->data->bus_prot_mask);
			if (ret)
				goto err_pwr_ack;
		}
		if (scpd->data->is_pwr_mfg1) {
			mtk_infracfg_set_topaxi_si0(scp->infracfg,
				scpd->data->si0_ctl_offs,
				scpd->data->si0_ctl_mask,
				scpd->data->si0_ctl_mask);
		}
	}

	for (i = scpd->data->ini_clk_num; i < scpd->data->max_clk_num; i++) {
		if (!scpd->clk[i])
			break;
		ret = clk_prepare_enable(scpd->clk[i]);
		if (ret) {
			for (--i; i >= scpd->data->ini_clk_num; i--)
				clk_disable_unprepare(scpd->clk[i]);
			dev_err(scp->dev, "Clock enable fail(%s)\n",
			       genpd->name);
		}
	}
	return 0;

err_pwr_ack:
	for (i = scpd->data->ini_clk_num - 1; i >= 0; i--) {
		if (scpd->clk[i])
			clk_disable_unprepare(scpd->clk[i]);
	}
err_clk:
	if (scpd->supply)
		regulator_disable(scpd->supply);
	dev_err(scp->dev, "Failed to power on domain %s\n", genpd->name);

	return ret;
}


static int pdn_ack_timeout_check(void __iomem *ctl_addr, u32 sram_pdn_ack,
				 unsigned long timeout)
{
	bool expired = false;
	int ret = 0;

	while ((readl(ctl_addr) & sram_pdn_ack) != sram_pdn_ack) {
		if (expired) {
			ret = -ETIMEDOUT;
			break;
		}

		cpu_relax();

		if (time_after(jiffies, timeout))
			expired = true;
	}
	return ret;
}

static int scpsys_power_off(struct generic_pm_domain *genpd)
{
	struct scp_domain *scpd = container_of(genpd, struct scp_domain,
					      genpd);
	struct scp *scp = scpd->scp;
	unsigned long timeout;
	bool expired;
	void __iomem *ctl_addr = scp->base + scpd->data->ctl_offs;
	void __iomem *sram_ctl_addr;
	void __iomem *tmp_addr;
	u32 sram_pdn1_ack, sram_pdn2_ack, sram_pdn_ack =
	    scpd->data->sram_pdn_ack_bits;
	u32 val = 0, val1;
	int ret = 0;
	int i;


	if (scpd->data->flag_bus_seq) {
		if (scpd->data->is_pwr_mfg1) {
			mtk_infracfg_set_topaxi_si0(scp->infracfg,
				scpd->data->si0_ctl_offs,
				scpd->data->si0_ctl_mask, 0);
		}
		if (scpd->data->bus_prot_mask) {
			ret = mtk_infracfg_set_bus_protection(scp->infracfg,
				scpd->data->bus_prot_offs,
				scpd->data->bus_protsta_offs,
				scpd->data->bus_prot_mask);
			if (ret)
				goto out;
		}
		if (scpd->data->bus_prot1_mask) {
			ret = mtk_infracfg_set_bus_protection(scp->infracfg,
				scpd->data->bus_prot1_offs,
				scpd->data->bus_protsta1_offs,
				scpd->data->bus_prot1_mask);
			if (ret)
				goto out;
		}
		if (scpd->data->bus_prot2_mask) {
			ret = mtk_infracfg_set_bus_protection(scp->infracfg,
				scpd->data->bus_prot2_offs,
				scpd->data->bus_protsta2_offs,
				scpd->data->bus_prot2_mask);
			if (ret)
				goto out;
		}
		if (scpd->data->bus_prot3_mask) {
			ret = mtk_infracfg_set_bus_protection(scp->infracfg,
				scpd->data->bus_prot3_offs,
				scpd->data->bus_protsta3_offs,
				scpd->data->bus_prot3_mask);
			if (ret)
				goto out;
		}
	}

	if ((scpd->data->is_pwr_sts_vpu) && (scpd->data->ini_clk_num > 0)) {
		for (i = scpd->data->max_clk_num - 1;
			i > (scpd->data->ini_clk_num - 1); i--) {
			if (!scpd->clk[i])
				break;
			clk_disable_unprepare(scpd->clk[i]);
		}
	}

	if (scpd->data->flag_rst_dp_phy_seq) {
		/* Set DP_PHY_PWR_RST_B = 0 */
		tmp_addr = scp->base + scpd->data->dp_phy_offs;
		val = readl(tmp_addr);
		val &= ~scpd->data->dp_phy_bits;
		writel(val1, tmp_addr);
	}

	if (scpd->data->flag_sram_seq) {
		/* Set SRAM_PDN = 1 */
		if (sram_pdn_ack) {
			if (scpd->data->is_pwr_sts_vpu) {
				sram_ctl_addr = scp->base +
					scpd->data->sram_ctl_offs;
				sram_pdn1_ack = scpd->data->sram_pdn1_ack_bits;
				sram_pdn2_ack = scpd->data->sram_pdn2_ack_bits;
				val = readl(sram_ctl_addr);
				val |= scpd->data->sram_pdn_bits;
				writel(val, sram_ctl_addr);
				/* wait until SRAM_PDN_ACK all 1 */
				timeout = jiffies + HZ;
				ret = pdn_ack_timeout_check(sram_ctl_addr,
					sram_pdn_ack, timeout);
				if (ret)
					goto out;

				val = readl(sram_ctl_addr);
				val |= scpd->data->sram_pdn1_bits;
				writel(val, sram_ctl_addr);
				/* wait until SRAM_PDN_ACK all 1 */
				timeout = jiffies + HZ;
				ret = pdn_ack_timeout_check(sram_ctl_addr,
					sram_pdn1_ack, timeout);
				if (ret)
					goto out;

				val = readl(sram_ctl_addr);
				val |= scpd->data->sram_pdn2_bits;
				writel(val, sram_ctl_addr);
				/* wait until SRAM_PDN_ACK all 1 */
				timeout = jiffies + HZ;
				ret = pdn_ack_timeout_check(sram_ctl_addr,
					sram_pdn2_ack, timeout);
				if (ret)
					goto out;
				if (scpd->data->is_pwr_sts_vpu_012) {
					for (i = 0; i < 5; i++) {
						val = readl(sram_ctl_addr);
						val |= (1 << (i+15));
						writel(val, sram_ctl_addr);
					}
				}
			} else {
				if (!scpd->data->flag_sram_dp_sleep_seq) {
					val = readl(ctl_addr);
					val |= scpd->data->sram_pdn_bits;
					writel(val, ctl_addr);

					/* wait until SRAM_PDN_ACK all 1 */
					timeout = jiffies + HZ;
					ret = pdn_ack_timeout_check(ctl_addr,
						sram_pdn_ack, timeout);
					if (ret)
						goto out;
				}
			}
		}
	}

	if (scpd->data->flag_sram_dp_sleep_seq) {
		val = readl(ctl_addr);
		/* Set SRAM_CKISO_BIT = 1 */
		val |= SRAM_CKISO_BIT;
		writel(val, ctl_addr);
		udelay(1);
		/* Set SRAM_ISOINT_BIT = 0 */
		val &= ~SRAM_ISOINT_BIT;
		writel(val, ctl_addr);
		/* Set SRAM_SLEEP = 0 */
		val = readl(ctl_addr);
		val &= ~BIT(12);
		writel(val, ctl_addr);
		val = readl(ctl_addr);
		val &= ~BIT(13);
		writel(val, ctl_addr);
		val = readl(ctl_addr);
		val &= ~BIT(14);
		writel(val, ctl_addr);
		val = readl(ctl_addr);
		val &= ~BIT(15);
		writel(val, ctl_addr);
		udelay(1);
}

	if (scpd->data->flag_rst_seq) {
		val = readl(ctl_addr);
		/* Set PWR_ISO = 1 */
		val |= PWR_ISO_BIT;
		writel(val, ctl_addr);
		/* Set PWR_CLK_DIS = 1 */
		val |= PWR_CLK_DIS_BIT;
		writel(val, ctl_addr);
		/* Set PWR_RST_B = 0 */
		if (!scpd->data->flag_sram_dp_sleep_seq) {
			val &= ~PWR_RST_B_BIT;
			writel(val, ctl_addr);
		}
	}

	if (scpd->data->flag_pwr_seq) {
		if (scpd->data->flag_pwr_els_seq) {
			/* Set ELS_ISO_EN_CON = 1 */
			tmp_addr = scp->base + scpd->data->els_iso_offs;
			val1 = readl(tmp_addr);
			val1 |= scpd->data->els_iso_bits;
			writel(val1, tmp_addr);
		}
		val = readl(ctl_addr);
		/* Set PWR_ON = 0 */
		val &= ~PWR_ON_BIT;
		writel(val, ctl_addr);
		/* Set PWR_ON_2ND = 0 */
		val &= ~PWR_ON_2ND_BIT;
		writel(val, ctl_addr);

		/* wait until PWR_ACK = 0 */
		timeout = jiffies + HZ;
		expired = false;
		while (1) {
			ret = scpsys_domain_is_on(scpd);
			if (ret == 0)
				break;

			if (expired) {
				ret = -ETIMEDOUT;
				goto out;
			}

			cpu_relax();

			if (time_after(jiffies, timeout))
				expired = true;
		}
		if (scpd->data->flag_pwr_buck_seq) {
			/* Set EXT_BUCK_ISO = 1 */
			tmp_addr = scp->base + scpd->data->ext_buck_offs;
			if (scpd->data->ext_buck_lock)
				spin_lock(scpd->data->ext_buck_lock);
			val1 = readl(tmp_addr);
			val1 |= scpd->data->ext_buck_bits;
			writel(val1, tmp_addr);
			if (scpd->data->ext_buck_lock)
				spin_unlock(scpd->data->ext_buck_lock);
		}
	}

	if ((scpd->data->is_pwr_sts_vpu) && (scpd->data->ini_clk_num > 0)) {
		for (i = scpd->data->ini_clk_num - 1; i >= 0; i--) {
			if (!scpd->clk[i])
				break;
			clk_disable_unprepare(scpd->clk[i]);
		}
	} else {
		for (i = 0; i < scpd->data->max_clk_num; i++) {
			if (!scpd->clk[i])
				break;
			clk_disable_unprepare(scpd->clk[i]);
		}
	}

	if (scpd->supply) {
		pr_err("disable regulator-2 %s\n", genpd->name);
		regulator_disable(scpd->supply);
	}

	return 0;

out:
	for (i = 0; i < scpd->data->max_clk_num; i++) {
		if (!scpd->clk[i])
			break;
		clk_disable_unprepare(scpd->clk[i]);
	}

	dev_err(scp->dev, "Failed to power off domain %s\n", genpd->name);

	return ret;
}

static bool scpsys_active_wakeup(struct device *dev)
{
	struct generic_pm_domain *genpd;
	struct scp_domain *scpd;

	genpd = pd_to_genpd(dev->pm_domain);
	scpd = container_of(genpd, struct scp_domain, genpd);

	return scpd->data->active_wakeup;
}

static void init_clks(struct platform_device *pdev, struct clk **clk)
{
	int i;

	for (i = CLK_NONE + 1; i < CLK_MAX; i++)
		clk[i] = devm_clk_get(&pdev->dev, clk_names[i]);
}

static struct scp *init_scp(struct platform_device *pdev,
			   const struct scp_domain_data *scp_domain_data,
			   int num,
			   const struct scp_ctrl_reg *scp_ctrl_reg)
{
	struct genpd_onecell_data *pd_data;
	struct resource *res;
	int i, j;
	struct scp *scp;
	struct clk *clk[CLK_MAX];

	scp = devm_kzalloc(&pdev->dev, sizeof(*scp), GFP_KERNEL);
	if (!scp)
		return ERR_PTR(-ENOMEM);
	scp->ctrl_reg.pwr_sta_offs = scp_ctrl_reg->pwr_sta_offs;
	scp->ctrl_reg.pwr_sta2nd_offs = scp_ctrl_reg->pwr_sta2nd_offs;
	scp->ctrl_reg.cpu_pwr_sta_offs = scp_ctrl_reg->cpu_pwr_sta_offs;
	scp->ctrl_reg.cpu_pwr_sta2nd_offs = scp_ctrl_reg->cpu_pwr_sta2nd_offs;

	scp->dev = &pdev->dev;
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	scp->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(scp->base))
		return ERR_CAST(scp->base);
	scp->domains = devm_kzalloc(&pdev->dev,
				sizeof(*scp->domains) * num, GFP_KERNEL);
	if (!scp->domains)
		return ERR_PTR(-ENOMEM);
	pd_data = &scp->pd_data;

	pd_data->domains = devm_kzalloc(&pdev->dev,
			sizeof(*pd_data->domains) * num, GFP_KERNEL);
	if (!pd_data->domains)
		return ERR_PTR(-ENOMEM);

	scp->infracfg = syscon_regmap_lookup_by_phandle(pdev->dev.of_node,
			"infracfg");
	if (IS_ERR(scp->infracfg)) {
		dev_err(&pdev->dev, "Cannot find infracfg controller: %ld\n",
				PTR_ERR(scp->infracfg));
		return ERR_CAST(scp->infracfg);
	}

	for (i = 0; i < num; i++) {
		struct scp_domain *scpd = &scp->domains[i];
		const struct scp_domain_data *data = &scp_domain_data[i];

		scpd->supply = devm_regulator_get_optional(&pdev->dev,
							  data->name);
		if (IS_ERR(scpd->supply)) {
			if (PTR_ERR(scpd->supply) == -ENODEV) {
				scpd->supply = NULL;
			} else {
				pr_err("%s supply error!\n", data->name);
				return ERR_CAST(scpd->supply);
			}
		}
	}

	pd_data->num_domains = num;

	init_clks(pdev, clk);

	for (i = 0; i < num; i++) {
		struct scp_domain *scpd = &scp->domains[i];
		struct generic_pm_domain *genpd = &scpd->genpd;
		const struct scp_domain_data *data = &scp_domain_data[i];

		pd_data->domains[i] = genpd;
		scpd->scp = scp;

		scpd->data = data;

		for (j = 0; j < MAX_CLKS && data->clk_id[j]; j++) {
			struct clk *c = clk[data->clk_id[j]];

			if (IS_ERR(c)) {
				dev_err(&pdev->dev, "%s: clk unavailable\n",
					data->name);
				return ERR_CAST(c);
			}
			scpd->clk[j] = c;
		}

		genpd->name = data->name;
		genpd->power_off = scpsys_power_off;
		genpd->power_on = scpsys_power_on;
		genpd->dev_ops.active_wakeup = scpsys_active_wakeup;
	}
	return scp;
}

static void mtk_register_power_domains(struct platform_device *pdev,
				struct scp *scp, int num)
{
	struct genpd_onecell_data *pd_data;
	int i, ret;

	for (i = 0; i < num; i++) {
		struct scp_domain *scpd = &scp->domains[i];
		struct generic_pm_domain *genpd = &scpd->genpd;

		/*
		 * Initially turn on all domains to make the domains usable
		 * with !CONFIG_PM and to get the hardware in sync with the
		 * software.  The unused domains will be switched off during
		 * late_init time.
		 */
		genpd->power_on(genpd);

		pm_genpd_init(genpd, NULL, false);
	}

	/*
	 * We are not allowed to fail here since there is no way to unregister
	 * a power domain. Once registered above we have to keep the domains
	 * valid.
	 */

	pd_data = &scp->pd_data;

	ret = of_genpd_add_provider_onecell(pdev->dev.of_node, pd_data);
	if (ret)
		dev_err(&pdev->dev, "Failed to add OF provider: %d\n", ret);
}

static spinlock_t vpu_ext_buck_lock = __SPIN_LOCK_UNLOCKED(vpu_ext_buck_lock);

/*
 * MT3612 power domain support
 */
static const struct scp_domain_data scp_domain_data_mt3612[] = {
	[MT3612_POWER_DOMAIN_SYSRAM_STBUF] = {
		.name = "sysram_stbuf",
		.sta_mask = PWR_STATUS_SYSRAM_STBUR,
		.ctl_offs = SYSRAM_STBUF_PWR_CON,
		.sram_pdn_bits = SYSRAM_STBUF_SRAM_PDN,
		.sram_pdn_ack_bits = SYSRAM_STBUF_SRAM_PDN_ACK,
		.bus_prot_mask = 0,
		.ini_clk_num = 1,
		.max_clk_num = 1,
		.clk_id = {CLK_MM_SYSRAM2},
		.is_pwr_sts_vpu = 0,
		.is_pwr_sts_vpu_012 = 0,
		.is_pwr_mfg1 = 0,
		.flag_pwr_seq = 1,
		.flag_pwr_buck_seq = 0,
		.flag_pwr_els_seq = 0,
		.flag_rst_seq = 1,
		.flag_sram_seq = 1,
		.flag_sram_dp_sleep_seq = 0,
		.flag_bus_seq = 0,
	},
	[MT3612_POWER_DOMAIN_SYSRAM_GAZE] = {
		.name = "sysram_gaze",
		.sta_mask = PWR_STATUS_SYSRAM_GAZE,
		.ctl_offs = SYSRAM_GAZE_PWR_CON,
		.sram_pdn_bits = SYSRAM_GAZE_SRAM_PDN,
		.sram_pdn_ack_bits = SYSRAM_GAZE_SRAM_PDN_ACK,
		.bus_prot_mask = 0,
		.ini_clk_num = 1,
		.max_clk_num = 1,
		.clk_id = {CLK_MM_SYSRAM1},
		.is_pwr_sts_vpu = 0,
		.is_pwr_sts_vpu_012 = 0,
		.is_pwr_mfg1 = 0,
		.flag_pwr_seq = 1,
		.flag_pwr_buck_seq = 0,
		.flag_pwr_els_seq = 0,
		.flag_rst_seq = 1,
		.flag_sram_seq = 1,
		.flag_sram_dp_sleep_seq = 0,
		.flag_bus_seq = 0,
	},
	[MT3612_POWER_DOMAIN_SYSRAM_VR_TRK] = {
		.name = "sysram_vr_trk",
		.sta_mask = PWR_STATUS_SYSRAM_VR_TRK,
		.ctl_offs = SYSRAM_VR_TRK_PWR_CON,
		.sram_pdn_bits = SYSRAM_VR_TRK_SRAM_PDN,
		.sram_pdn_ack_bits = SYSRAM_VR_TRK_SRAM_PDN_ACK,
		.bus_prot_mask = 0,
		.ini_clk_num = 1,
		.max_clk_num = 1,
		.clk_id = {CLK_MM_SYSRAM0},
		.is_pwr_sts_vpu = 0,
		.is_pwr_sts_vpu_012 = 0,
		.is_pwr_mfg1 = 0,
		.flag_pwr_seq = 1,
		.flag_pwr_buck_seq = 0,
		.flag_pwr_els_seq = 0,
		.flag_rst_seq = 1,
		.flag_sram_seq = 1,
		.flag_sram_dp_sleep_seq = 0,
		.flag_bus_seq = 0,
	},
	[MT3612_POWER_DOMAIN_MM_CORE] = {
		.name = "mm_core",
		.sta_mask = PWR_STATUS_MM_CORE,
		.ctl_offs = MM_CORE_PWR_CON,
		.sram_pdn_bits = MM_CORE_SRAM_PDN,
		.sram_pdn_ack_bits = SC_MM_CORE_SRAM_PDN_ACK,
		.bus_prot_offs = INFRA_TOPAXI_PROTECTEN_3,
		.bus_protsta_offs = INFRA_TOPAXI_PROTECTSTA1_3,
		.bus_prot_mask = GENMASK(1, 0) | GENMASK(22, 21),
		.bus_prot1_offs = SMI_PROTECTEN,
		.bus_protsta1_offs = SMI_PROTECTSTA1,
		.bus_prot1_mask = GENMASK(1, 0),
		.bus_prot2_offs = SMI_PROTECTEN,
		.bus_protsta2_offs = SMI_PROTECTSTA1,
		.bus_prot2_mask = GENMASK(22, 21),
		.bus_prot3_mask = 0,
		.ini_clk_num = 1,
		.max_clk_num = 9,
		.clk_id = {CLK_MM_CORE, CLK_MM_CORE_LARB0,
				CLK_MM_CORE_LARB1, CLK_MM_CORE_LARB15,
				CLK_MM_CORE_LARB16, CLK_MM_CORE_SMI0,
				CLK_MM_CORE_SMI1, CLK_MM_CORE_SMI15,
				CLK_MM_CORE_SMI16},
		.is_pwr_sts_vpu = 0,
		.is_pwr_sts_vpu_012 = 0,
		.is_pwr_mfg1 = 0,
		.flag_pwr_seq = 1,
		.flag_pwr_buck_seq = 0,
		.flag_pwr_els_seq = 0,
		.flag_rst_seq = 1,
		.flag_rst_dp_phy_seq = 0,
		.flag_sram_seq = 1,
		.flag_sram_dp_sleep_seq = 0,
		.flag_bus_seq = 1,
	},
	[MT3612_POWER_DOMAIN_MM_COM] = {
		.name = "mm_com",
		.sta_mask = PWR_STATUS_MM_COM,
		.ctl_offs = MM_COM_PWR_CON,
		.sram_pdn_bits = MM_COM_SRAM_PDN,
		.sram_pdn_ack_bits = SC_MM_COM_SRAM_PDN_ACK,
		.bus_prot_offs = INFRA_TOPAXI_PROTECTEN_3,
		.bus_protsta_offs = INFRA_TOPAXI_PROTECTSTA1_3,
		.bus_prot_mask = BIT(2) | GENMASK(24, 23),
		.bus_prot1_offs = SMI_PROTECTEN,
		.bus_protsta1_offs = SMI_PROTECTSTA1,
		.bus_prot1_mask = BIT(2),
		.bus_prot2_offs = SMI_PROTECTEN,
		.bus_protsta2_offs = SMI_PROTECTSTA1,
		.bus_prot2_mask = GENMASK(24, 23),
		.bus_prot3_mask = 0,
		.ini_clk_num = 1,
		.max_clk_num = 6,
		.clk_id = {CLK_MM_CMM, CLK_MM_CMM_SMI_COM,
				CLK_MM_CMM_SMI3, CLK_MM_CMM_SMI21,
				CLK_MM_CMM_SMI2, CLK_MM_CMM_SMI20},
		.is_pwr_sts_vpu = 0,
		.is_pwr_sts_vpu_012 = 0,
		.is_pwr_mfg1 = 0,
		.flag_pwr_seq = 1,
		.flag_pwr_buck_seq = 0,
		.flag_pwr_els_seq = 0,
		.flag_rst_seq = 1,
		.flag_rst_dp_phy_seq = 0,
		.flag_sram_seq = 1,
		.flag_sram_dp_sleep_seq = 0,
		.flag_bus_seq = 1,
	},
	[MT3612_POWER_DOMAIN_MM_GAZE] = {
		.name = "mm_gaze",
		.sta_mask = PWR_STATUS_MM_GAZE,
		.ctl_offs = MM_GAZE_PWR_CON,
		.sram_pdn_bits = MM_GAZE_SRAM_PDN,
		.sram_pdn_ack_bits = SC_MM_GAZE_SRAM_PDN_ACK,
		.bus_prot_offs = INFRA_TOPAXI_PROTECTEN_3,
		.bus_protsta_offs = INFRA_TOPAXI_PROTECTSTA1_3,
		.bus_prot_mask = BIT(3) | BIT(16),
		.bus_prot1_offs = SMI_PROTECTEN,
		.bus_protsta1_offs = SMI_PROTECTSTA1,
		.bus_prot1_mask = BIT(3),
		.bus_prot2_offs = SMI_PROTECTEN,
		.bus_protsta2_offs = SMI_PROTECTSTA1,
		.bus_prot2_mask = BIT(16),
		.bus_prot3_mask = 0,
		.ini_clk_num = 1,
		.max_clk_num = 3,
		.clk_id = {CLK_MM_CMM, CLK_MM_GAZE_SMI4, CLK_MM_GAZE_SMI22},
		.is_pwr_sts_vpu = 0,
		.is_pwr_sts_vpu_012 = 0,
		.is_pwr_mfg1 = 0,
		.flag_pwr_seq = 1,
		.flag_pwr_buck_seq = 0,
		.flag_pwr_els_seq = 0,
		.flag_rst_seq = 1,
		.flag_rst_dp_phy_seq = 0,
		.flag_sram_seq = 1,
		.flag_sram_dp_sleep_seq = 0,
		.flag_bus_seq = 1,
	},
	[MT3612_POWER_DOMAIN_CAM_SIDE0] = {
		.name = "cam_side0",
		.sta_mask = PWR_STATUS_CAM_SIDE0,
		.ctl_offs = CAM_SIDE0_PWR_CON,
		.sram_pdn_bits = CAM_SIDE0_SRAM_PDN,
		.sram_pdn_ack_bits = CAM_SIDE0_SRAM_PDN_ACK,
		.bus_prot_offs = INFRA_TOPAXI_PROTECTEN_3,
		.bus_protsta_offs = INFRA_TOPAXI_PROTECTSTA1_3,
		.bus_prot_mask = BIT(6) | BIT(12),
		.bus_prot1_offs = SMI_PROTECTEN,
		.bus_protsta1_offs = SMI_PROTECTSTA1,
		.bus_prot1_mask = BIT(6),
		.bus_prot2_offs = SMI_PROTECTEN,
		.bus_protsta2_offs = SMI_PROTECTSTA1,
		.bus_prot2_mask = BIT(12),
		.bus_prot3_mask = 0,
		.ini_clk_num = 1,
		.max_clk_num = 3,
		.clk_id = {CLK_CAM_SIDE0, CLK_CAM_S0_SYSRAM_LARB,
				CLK_CAM_S0_LARB},
		.is_pwr_sts_vpu = 0,
		.is_pwr_sts_vpu_012 = 0,
		.is_pwr_mfg1 = 0,
		.flag_pwr_seq = 1,
		.flag_pwr_buck_seq = 0,
		.flag_pwr_els_seq = 0,
		.flag_rst_seq = 1,
		.flag_rst_dp_phy_seq = 0,
		.flag_sram_seq = 1,
		.flag_sram_dp_sleep_seq = 0,
		.flag_bus_seq = 1,
	},
	[MT3612_POWER_DOMAIN_CAM_SIDE1] = {
		.name = "cam_side1",
		.sta_mask = PWR_STATUS_CAM_SIDE1,
		.ctl_offs = CAM_SIDE1_PWR_CON,
		.sram_pdn_bits = CAM_SIDE1_SRAM_PDN,
		.sram_pdn_ack_bits = CAM_SIDE1_SRAM_PDN_ACK,
		.bus_prot_offs = INFRA_TOPAXI_PROTECTEN_3,
		.bus_protsta_offs = INFRA_TOPAXI_PROTECTSTA1_3,
		.bus_prot_mask = BIT(8) | BIT(13),
		.bus_prot1_offs = SMI_PROTECTEN,
		.bus_protsta1_offs = SMI_PROTECTSTA1,
		.bus_prot1_mask = BIT(8),
		.bus_prot2_offs = SMI_PROTECTEN,
		.bus_protsta2_offs = SMI_PROTECTSTA1,
		.bus_prot2_mask = BIT(13),
		.ini_clk_num = 1,
		.max_clk_num = 3,
		.clk_id = {CLK_CAM_SIDE1, CLK_CAM_S1_SYSRAM_LARB,
				CLK_CAM_S1_LARB},
		.is_pwr_sts_vpu = 0,
		.is_pwr_sts_vpu_012 = 0,
		.is_pwr_mfg1 = 0,
		.flag_pwr_seq = 1,
		.flag_pwr_buck_seq = 0,
		.flag_pwr_els_seq = 0,
		.flag_rst_seq = 1,
		.flag_rst_dp_phy_seq = 0,
		.flag_sram_seq = 1,
		.flag_sram_dp_sleep_seq = 0,
		.flag_bus_seq = 1,
	},
	[MT3612_POWER_DOMAIN_IMG_SIDE0] = {
		.name = "img_side0",
		.sta_mask = PWR_STATUS_IMG_SIDE0,
		.ctl_offs = IMG_SIDE0_PWR_CON,
		.sram_pdn_bits = IMG_SIDE0_SRAM_PDN,
		.sram_pdn_ack_bits = IMG_SIDE0_SRAM_PDN_ACK,
		.bus_prot_offs = INFRA_TOPAXI_PROTECTEN_3,
		.bus_protsta_offs = INFRA_TOPAXI_PROTECTSTA1_3,
		.bus_prot_mask = BIT(7) | BIT(14) | BIT(19),
		.bus_prot1_offs = SMI_PROTECTEN,
		.bus_protsta1_offs = SMI_PROTECTSTA1,
		.bus_prot1_mask = BIT(7),
		.bus_prot2_offs = SMI_PROTECTEN,
		.bus_protsta2_offs = SMI_PROTECTSTA1,
		.bus_prot2_mask = BIT(14),
		.bus_prot3_offs = SMI_PROTECTEN,
		.bus_protsta3_offs = SMI_PROTECTSTA1,
		.bus_prot3_mask = BIT(19),
		.ini_clk_num = 1,
		.max_clk_num = 4,
		.clk_id = {CLK_IMG_SIDE0, CLK_IMG_S0_SYSRAM_LARB_1,
				CLK_IMG_S0_SYSRAM_LARB, CLK_IMG_S0_LARB},
		.is_pwr_sts_vpu = 0,
		.is_pwr_sts_vpu_012 = 0,
		.is_pwr_mfg1 = 0,
		.flag_pwr_seq = 1,
		.flag_pwr_buck_seq = 0,
		.flag_pwr_els_seq = 0,
		.flag_rst_seq = 1,
		.flag_rst_dp_phy_seq = 0,
		.flag_sram_seq = 1,
		.flag_sram_dp_sleep_seq = 0,
		.flag_bus_seq = 1,
	},
	[MT3612_POWER_DOMAIN_IMG_SIDE1] = {
		.name = "img_side1",
		.sta_mask = PWR_STATUS_IMG_SIDE1,
		.ctl_offs = IMG_SIDE1_PWR_CON,
		.sram_pdn_bits = IMG_SIDE1_SRAM_PDN,
		.sram_pdn_ack_bits = IMG_SIDE1_SRAM_PDN_ACK,
		.bus_prot_offs = INFRA_TOPAXI_PROTECTEN_3,
		.bus_protsta_offs = INFRA_TOPAXI_PROTECTSTA1_3,
		.bus_prot_mask = BIT(9) | BIT(15) | BIT(20),
		.bus_prot1_offs = SMI_PROTECTEN,
		.bus_protsta1_offs = SMI_PROTECTSTA1,
		.bus_prot1_mask = BIT(9),
		.bus_prot2_offs = SMI_PROTECTEN,
		.bus_protsta2_offs = SMI_PROTECTSTA1,
		.bus_prot2_mask = BIT(15),
		.bus_prot3_offs = SMI_PROTECTEN,
		.bus_protsta3_offs = SMI_PROTECTSTA1,
		.bus_prot3_mask = BIT(20),
		.ini_clk_num = 2,
		.max_clk_num = 5,
		.clk_id = {CLK_IMG_SIDE0, CLK_IMG_SIDE1,
				CLK_IMG_S1_SYSRAM_LARB_1,
				CLK_IMG_S1_SYSRAM_LARB,	CLK_IMG_S1_LARB},
		.is_pwr_sts_vpu = 0,
		.is_pwr_sts_vpu_012 = 0,
		.is_pwr_mfg1 = 0,
		.flag_pwr_seq = 1,
		.flag_pwr_buck_seq = 0,
		.flag_pwr_els_seq = 0,
		.flag_rst_seq = 1,
		.flag_rst_dp_phy_seq = 0,
		.flag_sram_seq = 1,
		.flag_sram_dp_sleep_seq = 0,
		.flag_bus_seq = 1,
	},
	[MT3612_POWER_DOMAIN_CAM_IMG_GZ0] = {
		.name = "cam_img_gz0",
		.sta_mask = PWR_STATUS_CAM_IMG_GZ0,
		.ctl_offs = CAM_IMG_GZ0_PWR_CON,
		.sram_pdn_bits = CAM_IMG_GZ0_SRAM_PDN,
		.sram_pdn_ack_bits = CAM_IMG_GZ0_SRAM_PDN_ACK,
		.bus_prot_offs = INFRA_TOPAXI_PROTECTEN_3,
		.bus_protsta_offs = INFRA_TOPAXI_PROTECTSTA1_3,
		.bus_prot_mask = BIT(10) | BIT(17),
		.bus_prot1_offs = SMI_PROTECTEN,
		.bus_protsta1_offs = SMI_PROTECTSTA1,
		.bus_prot1_mask = BIT(10),
		.bus_prot2_offs = SMI_PROTECTEN,
		.bus_protsta2_offs = SMI_PROTECTSTA1,
		.bus_prot2_mask = BIT(17),
		.bus_prot3_mask = 0,
		.ini_clk_num = 1,
		.max_clk_num = 3,
		.clk_id = {CLK_CAM_IMG_GZ0, CLK_CAM_GZ0_SYSRAM_LARB,
				CLK_CAM_GZ0_LARB},
		.is_pwr_sts_vpu = 0,
		.is_pwr_sts_vpu_012 = 0,
		.is_pwr_mfg1 = 0,
		.flag_pwr_seq = 1,
		.flag_pwr_buck_seq = 0,
		.flag_pwr_els_seq = 0,
		.flag_rst_seq = 1,
		.flag_rst_dp_phy_seq = 0,
		.flag_sram_seq = 1,
		.flag_sram_dp_sleep_seq = 0,
		.flag_bus_seq = 1,
	},
	[MT3612_POWER_DOMAIN_CAM_IMG_GZ1] = {
		.name = "cam_img_gz1",
		.sta_mask = PWR_STATUS_CAM_IMG_GZ1,
		.ctl_offs = CAM_IMG_GZ1_PWR_CON,
		.sram_pdn_bits = CAM_IMG_GZ1_SRAM_PDN,
		.sram_pdn_ack_bits = CAM_IMG_GZ1_SRAM_PDN_ACK,
		.bus_prot_offs = INFRA_TOPAXI_PROTECTEN_3,
		.bus_protsta_offs = INFRA_TOPAXI_PROTECTSTA1_3,
		.bus_prot_mask = BIT(11) | BIT(18),
		.bus_prot1_offs = SMI_PROTECTEN,
		.bus_protsta1_offs = SMI_PROTECTSTA1,
		.bus_prot1_mask = BIT(11),
		.bus_prot2_offs = SMI_PROTECTEN,
		.bus_protsta2_offs = SMI_PROTECTSTA1,
		.bus_prot2_mask = BIT(18),
		.bus_prot3_mask = 0,
		.ini_clk_num = 1,
		.max_clk_num = 3,
		.clk_id = {CLK_CAM_IMG_GZ1, CLK_CAM_GZ1_SYSRAM_LARB,
				CLK_CAM_GZ1_LARB},
		.is_pwr_sts_vpu = 0,
		.is_pwr_sts_vpu_012 = 0,
		.is_pwr_mfg1 = 0,
		.flag_pwr_seq = 1,
		.flag_pwr_buck_seq = 0,
		.flag_pwr_els_seq = 0,
		.flag_rst_seq = 1,
		.flag_rst_dp_phy_seq = 0,
		.flag_sram_seq = 1,
		.flag_sram_dp_sleep_seq = 0,
		.flag_bus_seq = 1,
	},
	[MT3612_POWER_DOMAIN_DP] = {
		.name = "dp",
		.sta_mask = PWR_STATUS_DP,
		.ctl_offs = DP_PWR_CON,
		.sram_pdn_bits = DP_SRAM_PDN,
		.sram_pdn_ack_bits = SC_DP_SRAM_PDN_ACK,
		.sram_sleep_bits = DP_SRAM_SLEEP_B,
		.bus_prot_mask = 0,
		.els_iso_offs = ELS_ISO_EN_CON,
		.els_iso_bits = DP_ELS_ISO,
		.ini_clk_num = 1,
		.max_clk_num = 1,
		.dp_phy_offs = DP_PHY_PWR_CON,
		.dp_phy_bits = DP_PHY_PWR_RST_B,
		.clk_id = {CLK_DP},
		.is_pwr_sts_vpu = 0,
		.is_pwr_sts_vpu_012 = 0,
		.is_pwr_mfg1 = 0,
		.flag_pwr_seq = 1,
		.flag_pwr_buck_seq = 0,
		.flag_pwr_els_seq = 1,
		.flag_rst_seq = 1,
		.flag_rst_dp_phy_seq = 1,
		.flag_sram_seq = 1,
		.flag_sram_dp_sleep_seq = 1,
		.flag_bus_seq = 0,
	},
	[MT3612_POWER_DOMAIN_VPU_VCORE] = {
		.name = "vpu_vcore",
		.sta_mask = PWR_STATUS_VPU_VCORE,
		.ctl_offs = VPU_CORE_PWR_CON,
		.sram_ctl_offs = VPU_CORE_PWR_CON,
		.sram_pdn_bits = BIT(8),
		.sram_pdn_ack_bits = BIT(24),
		.sram_pdn1_bits = BIT(9),
		.sram_pdn1_ack_bits = BIT(25),
		.sram_pdn2_bits = BIT(10),
		.sram_pdn2_ack_bits = BIT(26),
		.ini_clk_num = 1,
		.max_clk_num = 1,
		.clk_id = {CLK_IPU_IF},
		.is_pwr_sts_vpu = 1,
		.is_pwr_sts_vpu_012 = 0,
		.is_pwr_mfg1 = 0,
		.flag_pwr_seq = 1,
		.flag_pwr_buck_seq = 0,
		.flag_pwr_els_seq = 0,
		.flag_rst_seq = 1,
		.flag_rst_dp_phy_seq = 0,
		.flag_sram_seq = 0,
		.flag_sram_dp_sleep_seq = 0,
		.flag_bus_seq = 0,
	},
	[MT3612_POWER_DOMAIN_VPU_CONN] = {
		.name = "vpu_conn",
		.sta_mask = PWR_STATUS_VPU_CONN,
		.ctl_offs = VPU_CONN_PWR_CON,
		.sram_ctl_offs = VPU_CONN_PWR_CON,
		.sram_pdn_bits = BIT(8),
		.sram_pdn_ack_bits = BIT(24),
		.sram_pdn1_bits = BIT(9),
		.sram_pdn1_ack_bits = BIT(25),
		.sram_pdn2_bits = BIT(10),
		.sram_pdn2_ack_bits = BIT(26),
		.bus_prot_offs = INFRA_TOPAXI_PROTECTEN_2,
		.bus_protsta_offs = INFRA_TOPAXI_PROTECTSTA1_2,
		.bus_prot_mask = BIT(16),
		.bus_prot1_offs = INFRA_TOPAXI_PROTECTEN_3,
		.bus_protsta1_offs = INFRA_TOPAXI_PROTECTSTA1_3,
		.bus_prot1_mask = GENMASK(5, 4) | GENMASK(26, 25),
		.bus_prot2_offs = INFRA_TOPAXI_PROTECTEN,
		.bus_protsta2_offs = INFRA_TOPAXI_PROTECTSTA1,
		.bus_prot2_mask = BIT(25),
		.bus_prot3_offs = SMI_PROTECTEN,
		.bus_protsta3_offs = SMI_PROTECTSTA1,
		.bus_prot3_mask = GENMASK(5, 4) | GENMASK(26, 25),
		.ext_buck_offs = EXT_BUCK_ISO,
		.ext_buck_bits = VPU_CONN_EXT_BUCK_ISO,
		.ini_clk_num = 1,
		.max_clk_num = 3,
		.clk_id = {CLK_DSP, CLK_VPU_VCORE_AHB, CLK_VPU_VCORE_AXI},
		.is_pwr_sts_vpu = 1,
		.is_pwr_sts_vpu_012 = 0,
		.is_pwr_mfg1 = 0,
		.flag_pwr_seq = 1,
		.flag_pwr_buck_seq = 1,
		.flag_pwr_els_seq = 0,
		.flag_rst_seq = 1,
		.flag_rst_dp_phy_seq = 0,
		.flag_sram_seq = 0,
		.flag_sram_dp_sleep_seq = 0,
		.flag_bus_seq = 1,
	},
	[MT3612_POWER_DOMAIN_VPU0] = {
		.name = "vpu0",
		.sta_mask = PWR_STATUS_VPU0,
		.ctl_offs = VPU0_PWR_CON,
		.sram_ctl_offs = VPU0_SRAM_CON,
		.sram_pdn_bits = BIT(12),
		.sram_pdn_ack_bits = BIT(24),
		.sram_pdn1_bits = BIT(13),
		.sram_pdn1_ack_bits = BIT(25),
		.sram_pdn2_bits = BIT(14),
		.sram_pdn2_ack_bits = BIT(26),
		.bus_prot_offs = INFRA_TOPAXI_PROTECTEN_2,
		.bus_protsta_offs = INFRA_TOPAXI_PROTECTSTA1_2,
		.bus_prot_mask = BIT(29),
		.bus_prot1_offs = INFRA_TOPAXI_PROTECTEN_2,
		.bus_protsta1_offs = INFRA_TOPAXI_PROTECTSTA1_2,
		.bus_prot1_mask = BIT(12) | BIT(15) | BIT(19) | BIT(26),
		.bus_prot2_mask = 0,
		.bus_prot3_mask = 0,
		.ext_buck_offs = EXT_BUCK_ISO,
		.ext_buck_bits = VPU0_EXT_BUCK_ISO,
		.ext_buck_lock = &vpu_ext_buck_lock,
		.ini_clk_num = 6,
		.max_clk_num = 8,
		.clk_id = {CLK_DSP1,
			CLK_VPU_CONN_AHB, CLK_VPU_CONN_AXI, CLK_VPU_CONN_VPU,
			CLK_VPU_VCORE_AHB, CLK_VPU_VCORE_AXI,
			CLK_VPU_CORE0_VPU, CLK_VPU_CORE0_AXI},
		.is_pwr_sts_vpu = 1,
		.is_pwr_sts_vpu_012 = 1,
		.is_pwr_mfg1 = 0,
		.flag_pwr_seq = 1,
		.flag_pwr_buck_seq = 1,
		.flag_pwr_els_seq = 0,
		.flag_rst_seq = 1,
		.flag_rst_dp_phy_seq = 0,
		.flag_sram_seq = 1,
		.flag_sram_dp_sleep_seq = 0,
		.flag_bus_seq = 1,
	},
	[MT3612_POWER_DOMAIN_VPU1] = {
		.name = "vpu1",
		.sta_mask = PWR_STATUS_VPU1,
		.ctl_offs = VPU1_PWR_CON,
		.sram_ctl_offs = VPU1_SRAM_CON,
		.sram_pdn_bits = BIT(12),
		.sram_pdn_ack_bits = BIT(24),
		.sram_pdn1_bits = BIT(13),
		.sram_pdn1_ack_bits = BIT(25),
		.sram_pdn2_bits = BIT(14),
		.sram_pdn2_ack_bits = BIT(26),
		.bus_prot_offs = INFRA_TOPAXI_PROTECTEN_2,
		.bus_protsta_offs = INFRA_TOPAXI_PROTECTSTA1_2,
		.bus_prot_mask = BIT(30),
		.bus_prot1_offs = INFRA_TOPAXI_PROTECTEN_2,
		.bus_protsta1_offs = INFRA_TOPAXI_PROTECTSTA1_2,
		.bus_prot1_mask = BIT(13) | BIT(17) | BIT(20) | BIT(27),
		.bus_prot2_mask = 0,
		.bus_prot3_mask = 0,
		.ext_buck_offs = EXT_BUCK_ISO,
		.ext_buck_bits = VPU1_EXT_BUCK_ISO,
		.ext_buck_lock = &vpu_ext_buck_lock,
		.ini_clk_num = 6,
		.max_clk_num = 8,
		.clk_id = {CLK_DSP2,
			CLK_VPU_CONN_AHB, CLK_VPU_CONN_AXI, CLK_VPU_CONN_VPU,
			CLK_VPU_VCORE_AHB, CLK_VPU_VCORE_AXI,
			CLK_VPU_CORE1_VPU, CLK_VPU_CORE1_AXI},
		.is_pwr_sts_vpu = 1,
		.is_pwr_sts_vpu_012 = 1,
		.is_pwr_mfg1 = 0,
		.flag_pwr_seq = 1,
		.flag_pwr_buck_seq = 1,
		.flag_pwr_els_seq = 0,
		.flag_rst_seq = 1,
		.flag_rst_dp_phy_seq = 0,
		.flag_sram_seq = 1,
		.flag_sram_dp_sleep_seq = 0,
		.flag_bus_seq = 1,
	},
	[MT3612_POWER_DOMAIN_VPU2] = {
		.name = "vpu2",
		.sta_mask = PWR_STATUS_VPU2,
		.ctl_offs = VPU2_PWR_CON,
		.sram_ctl_offs = VPU2_SRAM_CON,
		.sram_pdn_bits = BIT(12),
		.sram_pdn_ack_bits = BIT(24),
		.sram_pdn1_bits = BIT(13),
		.sram_pdn1_ack_bits = BIT(25),
		.sram_pdn2_bits = BIT(14),
		.sram_pdn2_ack_bits = BIT(26),
		.bus_prot_offs = INFRA_TOPAXI_PROTECTEN_2,
		.bus_protsta_offs = INFRA_TOPAXI_PROTECTSTA1_2,
		.bus_prot_mask = BIT(31),
		.bus_prot1_offs = INFRA_TOPAXI_PROTECTEN_2,
		.bus_protsta1_offs = INFRA_TOPAXI_PROTECTSTA1_2,
		.bus_prot1_mask = BIT(14) | BIT(18) | BIT(21) | BIT(28),
		.bus_prot2_mask = 0,
		.bus_prot3_mask = 0,
		.ext_buck_offs = EXT_BUCK_ISO,
		.ext_buck_bits = VPU2_EXT_BUCK_ISO,
		.ext_buck_lock = &vpu_ext_buck_lock,
		.ini_clk_num = 6,
		.max_clk_num = 8,
		.clk_id = {CLK_DSP3,
			CLK_VPU_CONN_AHB, CLK_VPU_CONN_AXI, CLK_VPU_CONN_VPU,
			CLK_VPU_VCORE_AHB, CLK_VPU_VCORE_AXI,
			CLK_VPU_CORE2_VPU, CLK_VPU_CORE2_AXI},
		.is_pwr_sts_vpu = 1,
		.is_pwr_sts_vpu_012 = 1,
		.is_pwr_mfg1 = 0,
		.flag_pwr_seq = 1,
		.flag_pwr_buck_seq = 1,
		.flag_pwr_els_seq = 0,
		.flag_rst_seq = 1,
		.flag_rst_dp_phy_seq = 0,
		.flag_sram_seq = 1,
		.flag_sram_dp_sleep_seq = 0,
		.flag_bus_seq = 1,
	},
	[MT3612_POWER_DOMAIN_MFG0] = {
		.name = "mfg0",
		.sta_mask = PWR_STATUS_MFG0,
		.ctl_offs = MFG0_PWR_CON,
		.sram_pdn_bits = 0,
		.sram_pdn_ack_bits = 0,
		.bus_prot_mask = 0,
		.ini_clk_num = 1,
		.max_clk_num = 1,
		.clk_id = {CLK_MFG},
		.is_pwr_sts_vpu = 0,
		.is_pwr_sts_vpu_012 = 0,
		.is_pwr_mfg1 = 0,
		.flag_pwr_seq = 1,
		.flag_pwr_buck_seq = 0,
		.flag_pwr_els_seq = 0,
		.flag_rst_seq = 1,
		.flag_rst_dp_phy_seq = 0,
		.flag_sram_seq = 0,
		.flag_sram_dp_sleep_seq = 0,
		.flag_bus_seq = 0,
	},
	[MT3612_POWER_DOMAIN_MFG1] = {
		.name = "mfg1",
		.sta_mask = PWR_STATUS_MFG1,
		.ctl_offs = MFG1_PWR_CON,
		.sram_pdn_bits = MFG1_SRAM_PDN,
		.sram_pdn_ack_bits = MFG1_SRAM_PDN_ACK,
		.si0_ctl_offs = INFRA_TOPAXI_SI0_CTL,
		.si0_ctl_mask = BIT(6),
		.bus_prot_offs = INFRA_TOPAXI_PROTECTEN_2,
		.bus_protsta_offs = INFRA_TOPAXI_PROTECTSTA1_2,
		.bus_prot_mask = GENMASK(25, 22),
		.bus_prot1_offs = INFRA_TOPAXI_PROTECTEN,
		.bus_protsta1_offs = INFRA_TOPAXI_PROTECTSTA1,
		.bus_prot1_mask = BIT(24),
		.bus_prot2_offs = INFRA_TOPAXI_PROTECTEN,
		.bus_protsta2_offs = INFRA_TOPAXI_PROTECTSTA1,
		.bus_prot2_mask = GENMASK(5, 4),
		.bus_prot3_offs = SMI_PROTECTEN,
		.bus_protsta3_offs = SMI_PROTECTSTA1,
		.bus_prot3_mask = GENMASK(28, 27),
		.ini_clk_num = 1,
		.max_clk_num = 1,
		.clk_id = {CLK_MFG},
		.is_pwr_sts_vpu = 0,
		.is_pwr_sts_vpu_012 = 0,
		.is_pwr_mfg1 = 1,
		.flag_pwr_seq = 1,
		.flag_pwr_buck_seq = 0,
		.flag_pwr_els_seq = 0,
		.flag_rst_seq = 1,
		.flag_rst_dp_phy_seq = 0,
		.flag_sram_seq = 1,
		.flag_sram_dp_sleep_seq = 0,
		.flag_bus_seq = 1,
	},
	[MT3612_POWER_DOMAIN_MFG2] = {
		.name = "mfg2",
		.sta_mask = PWR_STATUS_MFG2,
		.ctl_offs = MFG2_PWR_CON,
		.sram_pdn_bits = MFG2_SRAM_PDN,
		.sram_pdn_ack_bits = MFG2_SRAM_PDN_ACK,
		.bus_prot_mask = 0,
		.ini_clk_num = 1,
		.max_clk_num = 1,
		.clk_id = {CLK_MFG},
		.is_pwr_sts_vpu = 0,
		.is_pwr_sts_vpu_012 = 0,
		.is_pwr_mfg1 = 0,
		.flag_pwr_seq = 1,
		.flag_pwr_buck_seq = 0,
		.flag_pwr_els_seq = 0,
		.flag_rst_seq = 1,
		.flag_rst_dp_phy_seq = 0,
		.flag_sram_seq = 1,
		.flag_sram_dp_sleep_seq = 0,
		.flag_bus_seq = 0,
	},
	[MT3612_POWER_DOMAIN_MFG3] = {
		.name = "mfg3",
		.sta_mask = PWR_STATUS_MFG3,
		.ctl_offs = MFG3_PWR_CON,
		.sram_pdn_bits = MFG3_SRAM_PDN,
		.sram_pdn_ack_bits = MFG3_SRAM_PDN_ACK,
		.bus_prot_mask = 0,
		.ini_clk_num = 1,
		.max_clk_num = 1,
		.clk_id = {CLK_MFG},
		.is_pwr_sts_vpu = 0,
		.is_pwr_sts_vpu_012 = 0,
		.is_pwr_mfg1 = 0,
		.flag_pwr_seq = 1,
		.flag_pwr_buck_seq = 0,
		.flag_pwr_els_seq = 0,
		.flag_rst_seq = 1,
		.flag_rst_dp_phy_seq = 0,
		.flag_sram_seq = 1,
		.flag_sram_dp_sleep_seq = 0,
		.flag_bus_seq = 0,
	}
};

static const struct scp_soc_data mt3612_data = {
	.domains = scp_domain_data_mt3612,
	.num_domains = ARRAY_SIZE(scp_domain_data_mt3612),
	.reg = {
		.pwr_sta_offs = PWR_STATUS,
		.pwr_sta2nd_offs = PWR_STATUS_2ND,
		.cpu_pwr_sta_offs = CPU_PWR_STATUS,
		.cpu_pwr_sta2nd_offs = CPU_PWR_STATUS_2ND,
	}
};

/*
 * scpsys driver init
 */

static const struct of_device_id of_scpsys_match_tbl[] = {
	{
		.compatible = "mediatek,mt3612-scpsys",
		.data = &mt3612_data,
	}, {
		/* sentinel */
	}
};

#define NUM_DOMAINS_MT3612	ARRAY_SIZE(scp_domain_data_mt3612)

static int scpsys_probe(struct platform_device *pdev)
{
	const struct of_device_id *match;
	const struct scp_subdomain *sd;
	const struct scp_soc_data *soc;
	struct scp *scp;
	struct genpd_onecell_data *pd_data;
	int i, ret;

	match = of_match_device(of_scpsys_match_tbl, &pdev->dev);
	if (!match || !match->data) {
		dev_err(&pdev->dev, "Error: No SCP\n");
		return -ENODEV;
	}

	soc = (const struct scp_soc_data *)match->data;

	scp = init_scp(pdev, soc->domains, soc->num_domains, &soc->reg);
	if (IS_ERR(scp))
		return PTR_ERR(scp);

	mtk_register_power_domains(pdev, scp, soc->num_domains);

	pd_data = &scp->pd_data;

	for (i = 0, sd = soc->subdomains ; i < soc->num_subdomains ; i++) {
		ret = pm_genpd_add_subdomain(pd_data->domains[sd->origin],
					    pd_data->domains[sd->subdomain]);
		if (ret && IS_ENABLED(CONFIG_PM))
			dev_err(&pdev->dev, "Failed to add subdomain: %d\n",
			       ret);
	}

	/* Turn on sysram before relative subsys power on for smi_protect */
	ret = pm_genpd_add_subdomain(
			pd_data->domains[MT3612_POWER_DOMAIN_SYSRAM_STBUF],
			pd_data->domains[MT3612_POWER_DOMAIN_MM_CORE]);
	if (ret && IS_ENABLED(CONFIG_PM))
		dev_err(&pdev->dev, "Failed to add subdomain: %d\n", ret);

	ret = pm_genpd_add_subdomain(
			pd_data->domains[MT3612_POWER_DOMAIN_SYSRAM_STBUF],
			pd_data->domains[MT3612_POWER_DOMAIN_MM_COM]);
	if (ret && IS_ENABLED(CONFIG_PM))
		dev_err(&pdev->dev, "Failed to add subdomain: %d\n", ret);

	ret = pm_genpd_add_subdomain(
			pd_data->domains[MT3612_POWER_DOMAIN_SYSRAM_STBUF],
			pd_data->domains[MT3612_POWER_DOMAIN_VPU_VCORE]);
	if (ret && IS_ENABLED(CONFIG_PM))
		dev_err(&pdev->dev, "Failed to add subdomain: %d\n", ret);

	ret = pm_genpd_add_subdomain(
			pd_data->domains[MT3612_POWER_DOMAIN_SYSRAM_STBUF],
			pd_data->domains[MT3612_POWER_DOMAIN_MFG1]);
	if (ret && IS_ENABLED(CONFIG_PM))
		dev_err(&pdev->dev, "Failed to add subdomain: %d\n", ret);

	ret = pm_genpd_add_subdomain(
			pd_data->domains[MT3612_POWER_DOMAIN_SYSRAM_STBUF],
			pd_data->domains[MT3612_POWER_DOMAIN_IMG_SIDE0]);
	if (ret && IS_ENABLED(CONFIG_PM))
		dev_err(&pdev->dev, "Failed to add subdomain: %d\n", ret);

	ret = pm_genpd_add_subdomain(
			pd_data->domains[MT3612_POWER_DOMAIN_SYSRAM_STBUF],
			pd_data->domains[MT3612_POWER_DOMAIN_IMG_SIDE1]);
	if (ret && IS_ENABLED(CONFIG_PM))
		dev_err(&pdev->dev, "Failed to add subdomain: %d\n", ret);

	ret = pm_genpd_add_subdomain(
			pd_data->domains[MT3612_POWER_DOMAIN_SYSRAM_GAZE],
			pd_data->domains[MT3612_POWER_DOMAIN_MM_GAZE]);
	if (ret && IS_ENABLED(CONFIG_PM))
		dev_err(&pdev->dev, "Failed to add subdomain: %d\n", ret);

	ret = pm_genpd_add_subdomain(
			pd_data->domains[MT3612_POWER_DOMAIN_SYSRAM_GAZE],
			pd_data->domains[MT3612_POWER_DOMAIN_CAM_IMG_GZ0]);
	if (ret && IS_ENABLED(CONFIG_PM))
		dev_err(&pdev->dev, "Failed to add subdomain: %d\n", ret);

	ret = pm_genpd_add_subdomain(
			pd_data->domains[MT3612_POWER_DOMAIN_SYSRAM_GAZE],
			pd_data->domains[MT3612_POWER_DOMAIN_CAM_IMG_GZ1]);
	if (ret && IS_ENABLED(CONFIG_PM))
		dev_err(&pdev->dev, "Failed to add subdomain: %d\n", ret);

	ret = pm_genpd_add_subdomain(
			pd_data->domains[MT3612_POWER_DOMAIN_SYSRAM_VR_TRK],
			pd_data->domains[MT3612_POWER_DOMAIN_CAM_SIDE0]);
	if (ret && IS_ENABLED(CONFIG_PM))
		dev_err(&pdev->dev, "Failed to add subdomain: %d\n", ret);

	ret = pm_genpd_add_subdomain(
			pd_data->domains[MT3612_POWER_DOMAIN_SYSRAM_VR_TRK],
			pd_data->domains[MT3612_POWER_DOMAIN_CAM_SIDE1]);
	if (ret && IS_ENABLED(CONFIG_PM))
		dev_err(&pdev->dev, "Failed to add subdomain: %d\n", ret);

	ret = pm_genpd_add_subdomain(
			pd_data->domains[MT3612_POWER_DOMAIN_SYSRAM_VR_TRK],
			pd_data->domains[MT3612_POWER_DOMAIN_IMG_SIDE0]);
	if (ret && IS_ENABLED(CONFIG_PM))
		dev_err(&pdev->dev, "Failed to add subdomain: %d\n", ret);

	ret = pm_genpd_add_subdomain(
			pd_data->domains[MT3612_POWER_DOMAIN_SYSRAM_VR_TRK],
			pd_data->domains[MT3612_POWER_DOMAIN_IMG_SIDE1]);
	if (ret && IS_ENABLED(CONFIG_PM))
		dev_err(&pdev->dev, "Failed to add subdomain: %d\n", ret);

	/* VPU specific power on sequence */
	ret = pm_genpd_add_subdomain(
			pd_data->domains[MT3612_POWER_DOMAIN_VPU_VCORE],
			pd_data->domains[MT3612_POWER_DOMAIN_VPU_CONN]);
	if (ret && IS_ENABLED(CONFIG_PM))
		dev_err(&pdev->dev, "Failed to add subdomain: %d\n", ret);
	ret = pm_genpd_add_subdomain(
			pd_data->domains[MT3612_POWER_DOMAIN_VPU_CONN],
			pd_data->domains[MT3612_POWER_DOMAIN_VPU0]);
	if (ret && IS_ENABLED(CONFIG_PM))
		dev_err(&pdev->dev, "Failed to add subdomain: %d\n", ret);
	ret = pm_genpd_add_subdomain(
			pd_data->domains[MT3612_POWER_DOMAIN_VPU_CONN],
			pd_data->domains[MT3612_POWER_DOMAIN_VPU1]);
	if (ret && IS_ENABLED(CONFIG_PM))
		dev_err(&pdev->dev, "Failed to add subdomain: %d\n", ret);
	ret = pm_genpd_add_subdomain(
			pd_data->domains[MT3612_POWER_DOMAIN_VPU_CONN],
			pd_data->domains[MT3612_POWER_DOMAIN_VPU2]);
	if (ret && IS_ENABLED(CONFIG_PM))
		dev_err(&pdev->dev, "Failed to add subdomain: %d\n", ret);

	return 0;
}

static struct platform_driver scpsys_drv = {
	.probe = scpsys_probe,
	.driver = {
		.name = "mtk-scpsys",
		.suppress_bind_attrs = true,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(of_scpsys_match_tbl),
	},
};
builtin_platform_driver(scpsys_drv);
