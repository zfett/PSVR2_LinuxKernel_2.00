/*
* Copyright (C) 2011-2015 MediaTek Inc.
*
* This program is free software: you can redistribute it and/or modify it under
* the terms of the GNU General Public License version 2 as published by the Free
* Software Foundation.
*
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
* FOR A PARTICULAR PURPOSE.
* See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with
* this program.
* If not, see <http://www.gnu.org/licenses/>.
*/

#include <linux/module.h>	/* needed by all modules */
#include <linux/init.h>		/* needed by module macros */
#include <linux/fs.h>		/* needed by file_operations* */
#include <linux/miscdevice.h>	/* needed by miscdevice* */
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <linux/platform_device.h>
#include <linux/device.h>	/* needed by device_* */
#include <linux/vmalloc.h>	/* needed by vmalloc */
#include <linux/uaccess.h>	/* needed by copy_to_user */
#include <linux/fs.h>		/* needed by file_operations* */
#include <linux/slab.h>		/* needed by kmalloc */
#include <linux/poll.h>		/* needed by poll */
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/suspend.h>
#include <linux/timer.h>
#include <linux/notifier.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/of_fdt.h>
#include <linux/ioport.h>
/* #include <linux/wakelock.h> */
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include "scp_ipi.h"
#include "scp_helper.h"
#include "scp_feature_define.h"
#include "scp_reg.h"
/* #include <soc/mediatek/mtk_dvfs.h> */

/* scp awake timout count definition*/
#define SCP_AWAKE_TIMEOUT		5000
/* scp semaphore timout count definition*/
#define SEMAPHORE_TIMEOUT		5000
#define SEMAPHORE_3WAY_TIMEOUT		5000
/* scp ready timout definition*/
#define SCP_READY_TIMEOUT		(30 * HZ)	/* 30 seconds */
#define SCP_A_TIMER			0
#define NUM_SCP_CLKS			4
static struct clk *scp_clks[NUM_SCP_CLKS];


/* scp ready status for notify*/
unsigned int scp_ready[SCP_CORE_TOTAL];

/* scp enable status*/
unsigned int scp_enable[SCP_CORE_TOTAL];

/* scp dvfs variable*/
unsigned int scp_expected_freq;
unsigned int scp_current_freq;

struct scp_regs scp_reg;

unsigned char *scp_send_buff[SCP_CORE_TOTAL];
unsigned char *scp_recv_buff[SCP_CORE_TOTAL];
static struct workqueue_struct *scp_workqueue;
static struct scp_work_struct scp_A_notify_work;
static DEFINE_MUTEX(scp_A_notify_mutex);
const char *core_ids[SCP_CORE_TOTAL] = { "SCP A" };

DEFINE_SPINLOCK(scp_awake_spinlock);
/* set flag after driver initial done */
static bool driver_init_done;
unsigned char **scp_swap_buf;

#define scp_print pr_debug

/*
 * memory copy to scp sram
 * @param trg: trg address
 * @param src: src address
 * @param size: memory size
 */
void memcpy_to_scp(void __iomem *trg, const void *src, int size)
{
	int i;
	u32 __iomem *t = trg;
	const u32 *s = src;

	for (i = 0; i < ((size + 3) >> 2); i++)
		*t++ = *s++;
}

/*
 * memory copy from scp sram
 * @param trg: trg address
 * @param src: src address
 * @param size: memory size
 */
void memcpy_from_scp(void *trg, const void __iomem *src, int size)
{
	int i;
	u32 *t = trg;
	const u32 __iomem *s = src;

	for (i = 0; i < ((size + 3) >> 2); i++)
		*t++ = *s++;
}

/*
 * acquire a hardware semaphore
 * @param flag: semaphore id
 * return  1 :get sema success
 *        -1 :get sema timeout
 */
int get_scp_semaphore(int flag)
{
	unsigned int read_back;
	unsigned int count = 0;
	int ret = -1;
	unsigned long spin_flags;

	/* return 1 to prevent from access when driver not ready */
	if (!driver_init_done)
		return -1;

	/* spinlock context safe */
	spin_lock_irqsave(&scp_awake_spinlock, spin_flags);

	flag = (flag * 2) + 1;

	read_back = (readl(SCP_SEMAPHORE) >> flag) & 0x1;

	if (read_back == 0) {
		writel((1UL << flag), SCP_SEMAPHORE);

		while (count != SEMAPHORE_TIMEOUT) {
			/* repeat test if we get semaphore */
			read_back = (readl(SCP_SEMAPHORE) >> flag) & 0x1;

			if (read_back == 1) {
				ret = 1;
				break;
			}
			writel((1UL << flag), SCP_SEMAPHORE);
			count++;
		}

		if (ret < 0)
			scp_print("[SCP] get scp sema. %d TIMEOUT...!\n", flag);
	} else {
		pr_err("[SCP] already hold scp sema. %d\n", flag);
	}

	spin_unlock_irqrestore(&scp_awake_spinlock, spin_flags);
	return ret;
}
EXPORT_SYMBOL_GPL(get_scp_semaphore);

/*
 * release a hardware semaphore
 * @param flag: semaphore id
 * return  1 :release sema success
 *        -1 :release sema fail
 */
int release_scp_semaphore(int flag)
{
	int read_back;
	int ret = -1;
	unsigned long spin_flags;

	/* return 1 to prevent from access when driver not ready */
	if (!driver_init_done)
		return -1;

	/* spinlock context safe */
	spin_lock_irqsave(&scp_awake_spinlock, spin_flags);
	flag = (flag * 2) + 1;

	read_back = (readl(SCP_SEMAPHORE) >> flag) & 0x1;

	if (read_back == 1) {
		/* Write 1 clear */
		writel((1UL << flag), SCP_SEMAPHORE);
		read_back = (readl(SCP_SEMAPHORE) >> flag) & 0x1;
		if (read_back == 0)
			ret = 1;
		else
			scp_print("[SCP] release scp sema. %d failed\n", flag);
	} else {
		pr_err("[SCP] try to release sema. %d not own by me\n", flag);
	}

	spin_unlock_irqrestore(&scp_awake_spinlock, spin_flags);
	return ret;
}
EXPORT_SYMBOL_GPL(release_scp_semaphore);

static BLOCKING_NOTIFIER_HEAD(scp_A_notifier_list);
/*
 * register apps notification
 * NOTE: this function may be blocked and should not be called in
 *       interrupt context
 * @param nb:   notifier block struct
 */
void scp_A_register_notify(struct notifier_block *nb)
{
	mutex_lock(&scp_A_notify_mutex);
	blocking_notifier_chain_register(&scp_A_notifier_list, nb);

	scp_print("[SCP] register scp A notify callback..\n");

	if (is_scp_ready(SCP_A_ID))
		nb->notifier_call(nb, SCP_EVENT_READY, NULL);
	mutex_unlock(&scp_A_notify_mutex);
}
EXPORT_SYMBOL_GPL(scp_A_register_notify);

/*
 * unregister apps notification
 * NOTE: this function may be blocked and should not be called in
 *       interrupt context
 * @param nb:     notifier block struct
 */
void scp_A_unregister_notify(struct notifier_block *nb)
{
	mutex_lock(&scp_A_notify_mutex);
	blocking_notifier_chain_unregister(&scp_A_notifier_list, nb);
	mutex_unlock(&scp_A_notify_mutex);
}
EXPORT_SYMBOL_GPL(scp_A_unregister_notify);

void scp_schedule_work(struct scp_work_struct *scp_ws)
{
	queue_work(scp_workqueue, &scp_ws->work);
}

/*
 * callback function for work struct
 * notify apps to start their tasks or generate an exception according to flag
 * NOTE: this function may be blocked and should not be called in
 *       interrupt context
 * @param ws:   work struct
 */
static void scp_A_notify_ws(struct work_struct *ws)
{
	struct scp_work_struct *sws =
	    container_of(ws, struct scp_work_struct, work);
	unsigned int scp_notify_flag = sws->flags;

	scp_ready[SCP_A_ID] = scp_notify_flag;

	if (scp_notify_flag) {
		writel(SCP_TO_DVFSCTL_EVENT_CLR_ALL, SCP_TO_DVFSCTL_REG);
		mutex_lock(&scp_A_notify_mutex);
		blocking_notifier_call_chain(&scp_A_notifier_list,
					     SCP_EVENT_READY, NULL);
		mutex_unlock(&scp_A_notify_mutex);
	}

}

/*
 * mark notify flag to 1 to notify apps to start their tasks
 */
static void scp_A_set_ready(void)
{
	pr_debug("%s()\n", __func__);
	scp_A_notify_work.flags = 1;
	scp_schedule_work(&scp_A_notify_work);
}

/*
 * handle notification from scp
 * mark scp is ready for running tasks
 * @param id:   ipi id
 * @param data: ipi data
 * @param len:  length of ipi data
 */
static void scp_A_ready_ipi_handler(int id, void *data,
		unsigned int len, void *pdata)
{
	unsigned int scp_image_size = *(unsigned int *)data;

	if (!scp_ready[SCP_A_ID])
		scp_A_set_ready();

	/*verify scp image size*/
	if (scp_image_size != SCP_A_TCM_SIZE) {
		pr_err("[SCP]image size ERROR! AP=0x%x,SCP=0x%x\n",
			SCP_A_TCM_SIZE, scp_image_size);
		WARN_ON(1);
	}
	pr_err("[SCP] %s done\n", __func__);
}

/*
 * @return: 1 if scp is ready for running tasks
 */
unsigned int is_scp_ready(enum scp_core_id id)
{
	if (scp_ready[id])
		return 1;
	else
		return 0;
}
EXPORT_SYMBOL_GPL(is_scp_ready);

/*
 * reset scp and create a timer waiting for scp notify
 * notify apps to stop their tasks if needed
 * generate error if reset fail
 * NOTE: this function may be blocked and should not be called in
 *       interrupt context
 * @param reset:    bit[0-3]=0 for scp enable, =1 for reboot
 *                  bit[4-7]=0 for All, =1 for scp_A
 * @return:         0 if success
 */
int reset_scp(int reset)
{

	if (((reset & 0xf0) == 0x10) || ((reset & 0xf0) == 0x00)) {
		/*reset scp A */
		mutex_lock(&scp_A_notify_mutex);
		blocking_notifier_call_chain(&scp_A_notifier_list,
					     SCP_EVENT_STOP, NULL);
		mutex_unlock(&scp_A_notify_mutex);

		if (reset & 0x0f) {	/* do reset */
			/* make sure scp is in idle state */
			int timeout = 50;	/* max wait 1s */

			while (--timeout) {
				if (readl(SCP_SLEEP_STATUS_REG) &
				    SCP_A_IS_SLEEP) {
					/* reset */
					writel(SCP_CM4_RESET_ASSERT, SCP_BASE);
					scp_ready[SCP_A_ID] = 0;
					dsb(SY);
					break;
				}

				msleep(20);	/* wait time for SCP CM4 idle */
			}
			scp_print("[SCP] wait scp A reset timeout %d\n",
				  timeout);
		}

		if (scp_enable[SCP_A_ID]) {
			scp_print("[SCP] reset scp A\n");
			writel(SCP_CM4_RESET_DEASSERT, SCP_BASE);
			dsb(SY);
		}
	}

	scp_print("[SCP] reset scp done\n");

	return 0;
}

static inline ssize_t scp_A_status_show(struct device *kobj,
					struct device_attribute *attr,
					char *buf)
{
	if (scp_ready[SCP_A_ID])
		return scnprintf(buf, PAGE_SIZE, "SCP A is ready\n");
	else
		return scnprintf(buf, PAGE_SIZE, "SCP A is not ready\n");
}

DEVICE_ATTR(scp_A_status, 0444, scp_A_status_show, NULL);

static struct miscdevice scp_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "scp",
	.fops = &scp_A_log_file_ops
};

static struct miscdevice scp_crash_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "scpcrash",
	.fops = &scp_A_crashinfo_file_ops
};

/*
 * register /dev files
 * @return:     0: success, otherwise: fail
 */
static int create_files(void)
{
	int ret;

	ret = misc_register(&scp_device);
	if (unlikely(ret != 0)) {
		pr_err("[SCP] misc register failed\n");
		return ret;
	}

	ret = misc_register(&scp_crash_device);
	if (unlikely(ret != 0)) {
		pr_err("[SCP] misc register failed\n");
		return ret;
	}

	return 0;
}

static const char *scp_clk_name[NUM_SCP_CLKS] = {
	"top-scp-sel",
	"top-scp-ck0",
	"top-scp-ck1",
	"top-scp-ck2",
};

static int scp_device_probe(struct platform_device *pdev)
{
	int ret = 0, i;
	struct resource *res;
	const char *core_status = NULL;
	struct device *dev = &pdev->dev;
	struct scp_regs *scp_reg_local;
	struct clk *clk;

	scp_reg_local =
	    devm_kzalloc(&pdev->dev, sizeof(*scp_reg_local), GFP_KERNEL);
	if (!scp_reg_local)
		return -ENOMEM;

	platform_set_drvdata(pdev, scp_reg_local);

	scp_reg_local->dev = dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	scp_reg.sram = devm_ioremap_resource(dev, res);
	if (IS_ERR((void const *)scp_reg.sram)) {
		pr_err("[SCP] scp_reg.sram error\n");
		return -1;
	}
	scp_reg.total_tcmsize = (unsigned int)resource_size(res);
	scp_print("[SCP] sram base=0x%p %x\n", scp_reg.sram,
		  scp_reg.total_tcmsize);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	scp_reg.cfg = devm_ioremap_resource(dev, res);
	if (IS_ERR((void const *)scp_reg.cfg)) {
		pr_err("[SCP] scp_reg.cfg error\n");
		return -1;
	}
	scp_print("[SCP] cfg base=0x%p\n", scp_reg.cfg);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	scp_reg.clkctrl = devm_ioremap_resource(dev, res);
	if (IS_ERR((void const *)scp_reg.clkctrl)) {
		pr_err("[SCP] scp_reg.clkctrl error\n");
		return -1;
	}
	scp_print("[SCP] clkctrl base=0x%p\n", scp_reg.clkctrl);

	scp_reg.irq = platform_get_irq(pdev, 0);

	scp_print("[SCP] scp_reg.irq=%d\n", scp_reg.irq);

	/* scp core 1 */
	of_property_read_string(pdev->dev.of_node, "core_1", &core_status);
	if (strcmp(core_status, "enable") != 0) {
		pr_err("[SCP] core_1 not enable\n");
	} else {
		scp_print("[SCP] core_1 enable\n");
		scp_enable[SCP_A_ID] = 1;
	}

	for (i = 0; i < NUM_SCP_CLKS; i++) {
		clk = devm_clk_get(&pdev->dev, scp_clk_name[i]);
		if (IS_ERR(clk)) {
			if (of_find_property(pdev->dev.of_node, "clocks",
				NULL)) {
				dev_err(&pdev->dev, "fail to get clock\n");
				return -ENODEV;
			}
			dev_info(&pdev->dev, "No available clocks in DTS\n");
		} else {
			scp_clks[i] = clk;
			ret = clk_prepare_enable(clk);
			dev_info(&pdev->dev, "[SCP] clock:%s scp enable\n",
				scp_clk_name[i]);
			if (ret) {
				dev_err(&pdev->dev, "failed to enable clock\n");
				return ret;
			}
		}
	}
	pr_info("[SCP] SCP driver probe ok\n");

	return ret;
}

static int scp_device_remove(struct platform_device *dev)
{
	int i;

	scp_print("[SCP] remove begin!\n");
	misc_deregister(&scp_crash_device);
	misc_deregister(&scp_device);
	kfree(scp_send_buff[SCP_A_ID]);
	kfree(scp_recv_buff[SCP_A_ID]);

	for (i = 0; i < NUM_SCP_CLKS; i++) {
		if (IS_ERR(scp_clks[i]))
			continue;
		clk_disable_unprepare(scp_clks[i]);
	}
	scp_print("[SCP] remove success!\n");

	return 0;
}

static const struct of_device_id scp_of_ids[] = {
	{.compatible = "mediatek,mt3612-scp"},
	{}
};

static struct platform_driver mtk_scp_device = {
	.probe = scp_device_probe,
	.remove = scp_device_remove,
	.driver = {
		   .name = "scp",
		   .owner = THIS_MODULE,
		   .of_match_table = scp_of_ids,
		   },
};

/*
 * driver initialization entry point
 */
static int __init scp_init(void)
{
	int ret = 0;
	int i = 0;

	/* scp ready static flag initialise */
	for (i = 0; i < SCP_CORE_TOTAL; i++) {
		scp_enable[i] = 0;
		scp_ready[i] = 0;
	}

	/* scp platform initialise */
	scp_print("[SCP] platform init\n");
	scp_workqueue = create_workqueue("SCP_WQ");

	if (platform_driver_register(&mtk_scp_device))
		pr_err("[SCP] scp probe fail\n");

	/* skip initial if dts status = "disable" */
	if (!scp_enable[SCP_A_ID]) {
		pr_err("[SCP] scp disabled!!\n");
		return -1;
	}
	/* scp ipi initialise */
	scp_send_buff[SCP_A_ID] = kmalloc((size_t) SHARE_BUF_SIZE, GFP_KERNEL);
	if (!scp_send_buff[SCP_A_ID])
		return -1;

	scp_recv_buff[SCP_A_ID] = kmalloc((size_t) SHARE_BUF_SIZE, GFP_KERNEL);
	if (!scp_recv_buff[SCP_A_ID])
		return -1;

	INIT_WORK(&scp_A_notify_work.work, scp_A_notify_ws);

	scp_A_irq_init();
	scp_A_ipi_init();
	scp_ready[SCP_A_ID] = 1;

	scp_ipi_registration(SCP_IPI_SCP_READY, scp_A_ready_ipi_handler,
		"scp_A_ready", NULL);

	ret = create_files();
	if (unlikely(ret != 0)) {
		pr_err("[SCP] create files failed\n");
		return -1;
	}

	/* scp request irq */
	scp_print("[SCP] request_irq\n");
	ret =
	    request_irq(scp_reg.irq, scp_A_irq_handler, IRQF_TRIGGER_HIGH,
			"SCP A IPC2HOST", NULL);
	if (ret) {
		pr_err("[SCP] CM4 A require irq failed\n");
		return -1;
	}

	driver_init_done = true;
	reset_scp(SCP_ALL_ENABLE);

	return ret;
}

/*
 * driver exit point
 */
static void __exit scp_exit(void)
{
	scp_print("[SCP] scp exit\n");
	free_irq(scp_reg.irq, NULL);
	scp_ipi_unregistration(SCP_IPI_SCP_READY);
	platform_driver_unregister(&mtk_scp_device);
	flush_workqueue(scp_workqueue);
	destroy_workqueue(scp_workqueue);
	kfree(scp_swap_buf);
}


module_init(scp_init);
module_exit(scp_exit);

MODULE_LICENSE("GPL v2");
