/*
* Copyright (C) 2011-2015 MediaTek Inc.
*
* This program is free software: you can redistribute it and/or modify it under
* the terms of the
* GNU General Public License version 2 as published by the Free Software
* Foundation.
*
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
* PARTICULAR PURPOSE.
* See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with
* this program.
* If not, see <http://www.gnu.org/licenses/>.
*/

#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/reboot.h>
#include "scp_ipi.h"
#include "scp_helper.h"

/*
 * handler for wdt irq for scp
 * dump scp register
 */
static void scp_A_wdt_handler(void)
{
	panic("[SCP] CM4 A WDT exception\n");
	emergency_restart();

	return;
}

/*
 * dispatch scp irq
 * reset scp and generate exception if needed
 * @param irq:      irq id
 * @param dev_id:   should be NULL
 */
irqreturn_t scp_A_irq_handler(int irq, void *dev_id)
{
	unsigned int reg_value;
	int reboot = 0;

	reg_value = readl(SCP_A_TO_HOST_REG);

	if (reg_value & SCP_IRQ_WDT) {
		scp_A_wdt_handler();
		reboot = 1;
		reg_value &= SCP_IRQ_WDT;
	}

	if (reg_value & SCP_IRQ_SCP2HOST) {
		/* if WDT and IPI occurred at the same time, ignore the IPI */
		if (!reboot)
			scp_A_ipi_handler();
		reg_value &= SCP_IRQ_SCP2HOST;
	}

	writel(reg_value, SCP_A_TO_HOST_REG);

	return IRQ_HANDLED;
}

/*
 * scp irq initialize
 */
void scp_A_irq_init(void)
{
	writel(1UL, SCP_A_TO_HOST_REG);	/* clear scp irq */
}
