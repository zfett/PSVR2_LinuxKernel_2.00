/*
* Copyright (C) 2011-2015 MediaTek Inc.
*
* This program is free software: you can redistribute it and/or modify it
* under the terms of the
* GNU General Public License version 2 as published by the Free Software
* Foundation.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
* PARTICULAR PURPOSE.
* See the GNU General Public License for more details
*
* You should have received a copy of the GNU General Public License along
* with this program.
* If not, see <http://www.gnu.org/licenses/>.
*/

#include <linux/mutex.h>
#include <linux/io.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>
#include "scp_ipi.h"
#include "scp_helper.h"

#define PRINT_THRESHOLD 10000
enum scp_ipi_id scp_ipi_id_record;
enum scp_ipi_id scp_ipi_mutex_owner[SCP_CORE_TOTAL];
enum scp_ipi_id scp_ipi_owner[SCP_CORE_TOTAL];

unsigned int scp_ipi_id_record_count;
unsigned int scp_to_ap_ipi_count;
unsigned int ap_to_scp_ipi_count;

struct scp_ipi_desc scp_ipi_desc[SCP_NR_IPI];
struct share_obj *scp_send_obj[SCP_CORE_TOTAL];
struct share_obj *scp_rcv_obj[SCP_CORE_TOTAL];
struct mutex scp_ipi_mutex[SCP_CORE_TOTAL];
/*
 * find an ipi handler and invoke it
 */
void scp_A_ipi_handler(void)
{
	enum scp_ipi_id scp_id;

	scp_id = scp_rcv_obj[SCP_A_ID]->id;

	pr_debug("scp A ipi handler %d\n", scp_id);

	if (scp_id >= SCP_NR_IPI || scp_id <= 0) {
		/* ipi id abnormal */
		pr_debug("[SCP] A ipi handler id abnormal, id=%d\n", scp_id);
	} else if (scp_ipi_desc[scp_id].handler) {
		memcpy_from_scp(scp_recv_buff[SCP_A_ID],
				scp_rcv_obj[SCP_A_ID]->share_buf,
				scp_rcv_obj[SCP_A_ID]->len);

		scp_ipi_desc[scp_id].recv_count++;
		scp_to_ap_ipi_count++;
		scp_ipi_desc[scp_id].handler(scp_id, scp_recv_buff[SCP_A_ID],
					     scp_rcv_obj[SCP_A_ID]->len,
					     scp_ipi_desc[scp_id].pdata);

		/* After SCP IPI handler,
		 * send a awake ipi to avoid
		 * SCP keeping in ipi busy idle state
		 */
		/* set a direct IPI to awake SCP */
		writel((1UL << SCP_A_IPI_AWAKE_NUM), GIPC_TO_SCP_REG);
	} else {
		/* scp_ipi_handler is null or ipi id abnormal */
		pr_debug("[SCP] A ipi handler is null or abnormal, id=%d\n",
			 scp_id);
	}
	/* AP side write 1 to clear SCP to DVFSCTL reg.
	 * scp side write 1 to set SCP to DVFSCTL reg.
	 * scp set        bit[0]
	 */
	writel(SCP_TO_DVFSCTL_EVENT_CLR, SCP_TO_DVFSCTL_REG);
}

/*
 * ipi initialize
 */
void scp_A_ipi_init(void)
{
	int i = 0;

	mutex_init(&scp_ipi_mutex[SCP_A_ID]);
	scp_rcv_obj[SCP_A_ID] = SCP_A_SHARE_BUFFER;
	scp_send_obj[SCP_A_ID] = scp_rcv_obj[SCP_A_ID] + 1;
	pr_debug("scp_rcv_obj[SCP_A_ID] = 0x%p\n", scp_rcv_obj[SCP_A_ID]);
	pr_debug("scp_send_obj[SCP_A_ID] = 0x%p\n", scp_send_obj[SCP_A_ID]);
	memset_io(scp_send_obj[SCP_A_ID], 0, SHARE_BUF_SIZE);
	scp_to_ap_ipi_count = 0;
	ap_to_scp_ipi_count = 0;

	for (i = 0; i < SCP_NR_IPI; i++) {
		scp_ipi_desc[i].recv_count = 0;
		scp_ipi_desc[i].success_count = 0;
		scp_ipi_desc[i].busy_count = 0;
		scp_ipi_desc[i].error_count = 0;
	}
}

/*
 * API let apps can register an ipi handler to receive IPI
 * @param id:	   IPI ID
 * @param handler:  IPI handler
 * @param name:	 IPI name
 */
enum scp_ipi_status scp_ipi_registration(unsigned int id,
					 void (*handler)(int id, void *data,
						 unsigned int len, void *udata),
					 const char *name, void *pdata)
{
	if (id < SCP_NR_IPI) {
		scp_ipi_desc[id].name = name;

		if (handler == NULL)
			return SCP_ERROR;

		scp_ipi_desc[id].handler = handler;
		scp_ipi_desc[id].pdata = pdata;
		return SCP_DONE;
	} else {
		return SCP_ERROR;
	}
}
EXPORT_SYMBOL_GPL(scp_ipi_registration);

/*
 * API let apps unregister an ipi handler
 * @param id:	   IPI ID
 */
enum scp_ipi_status scp_ipi_unregistration(unsigned int id)
{
	if (id < SCP_NR_IPI) {
		scp_ipi_desc[id].name = "";
		scp_ipi_desc[id].handler = NULL;
		scp_ipi_desc[id].pdata = NULL;
		return SCP_DONE;
	} else {
		return SCP_ERROR;
	}
}
EXPORT_SYMBOL_GPL(scp_ipi_unregistration);

/*
 * API for apps to send an IPI to scp
 * @param id:   IPI ID
 * @param buf:  the pointer of data
 * @param len:  data length
 * @param wait: If true, wait (atomically) until data have been gotten by Host
 * @param len:  data length
 */
enum scp_ipi_status scp_ipi_send(unsigned int id, void *buf,
				 unsigned int len, unsigned int wait,
				 enum scp_core_id scp_id)
{
	unsigned int reg_value;

	/* avoid scp log print too much */
	if (scp_ipi_id_record == id)
		scp_ipi_id_record_count++;
	else
		scp_ipi_id_record_count = 0;

	scp_ipi_id_record = id;

	if (scp_id >= SCP_CORE_TOTAL) {
		pr_err("scp_ipi_send: scp_id:%d wrong\n", scp_id);
		scp_ipi_desc[id].error_count++;
		return SCP_ERROR;
	}

	if (in_interrupt()) {
		if (wait) {
			pr_err("scp_ipi_send: cannot use in isr\n");
			scp_ipi_desc[id].error_count++;
			return SCP_ERROR;
		}
	}

	if (id >= SCP_NR_IPI) {
		pr_err("scp_ipi_send: ipi id %d wrong\n", id);
		return SCP_ERROR;
	}
	if (is_scp_ready(scp_id) == 0) {
		/* pr_err("scp_ipi_send: %s not enabled, id=%d\n",
		 * core_ids[scp_id], id);
		 */
		scp_ipi_desc[id].error_count++;
		return SCP_ERROR;
	}
	if (len > sizeof(scp_send_obj[scp_id]->share_buf) || buf == NULL) {
		pr_err("scp_ipi_send: %s buffer error\n", core_ids[scp_id]);
		scp_ipi_desc[id].error_count++;
		return SCP_ERROR;
	}

	if (mutex_trylock(&scp_ipi_mutex[scp_id]) == 0) {
		/* avoid scp ipi send log print too much */
		if ((scp_ipi_id_record_count % PRINT_THRESHOLD == 0) ||
		    (scp_ipi_id_record_count % PRINT_THRESHOLD == 1)) {
			pr_err
			   ("scp_ipi_send:%s %d mutex_trylock busy, owner=%d\n",
			     core_ids[scp_id], id, scp_ipi_mutex_owner[scp_id]);
		}
		scp_ipi_desc[id].busy_count++;
		return SCP_BUSY;
	}

	/* get scp ipi mutex owner */
	scp_ipi_mutex_owner[scp_id] = id;
	reg_value = readl(GIPC_TO_SCP_REG);
	if ((reg_value & (1UL << scp_id)) > 0) {
		/* avoid scp ipi send log print too much */
		if ((scp_ipi_id_record_count % PRINT_THRESHOLD == 0) ||
		    (scp_ipi_id_record_count % PRINT_THRESHOLD == 1)) {
			pr_err
			("%s: %s %d host to scp busy, ipi last time = %d\n",
			__func__, core_ids[scp_id], id,
			scp_ipi_owner[scp_id]);
		}

		scp_ipi_desc[id].busy_count++;
		mutex_unlock(&scp_ipi_mutex[scp_id]);
		return SCP_BUSY;
	}

	/* get scp ipi send owner */
	scp_ipi_owner[scp_id] = id;

	memcpy(scp_send_buff[scp_id], buf, len);
	memcpy_to_scp((void *)scp_send_obj[scp_id]->share_buf,
		      scp_send_buff[scp_id], len);
	scp_send_obj[scp_id]->len = len;
	scp_send_obj[scp_id]->id = id;
	dsb(SY);

	/* record timestamp */
	scp_ipi_desc[id].success_count++;
	ap_to_scp_ipi_count++;

	/* send host to scp ipi */
	pr_debug("scp_ipi_send: SCP A send host to scp ipi\n");
	writel((1UL << scp_id), GIPC_TO_SCP_REG);

	if (wait) {
		do {
			if ((readl(GIPC_TO_SCP_REG) & (1UL << scp_id)) == 0)
				break;
			usleep_range(500,1000);
		} while (1);
	}

	mutex_unlock(&scp_ipi_mutex[scp_id]);

	return SCP_DONE;
}
EXPORT_SYMBOL_GPL(scp_ipi_send);

void scp_ipi_info_dump(enum scp_ipi_id id)
{
	pr_debug("%u\t%u\t%u\t%u\t%u\t%s\n\r",
		 id,
		 scp_ipi_desc[id].recv_count,
		 scp_ipi_desc[id].success_count,
		 scp_ipi_desc[id].busy_count,
		 scp_ipi_desc[id].error_count, scp_ipi_desc[id].name);

}

void scp_ipi_status_dump_id(enum scp_ipi_id id)
{
	pr_debug("[SCP]id\trecv\tsuccess\tbusy\terror\tname\n\r");
	scp_ipi_info_dump(id);
}

void scp_ipi_status_dump(void)
{
	enum scp_ipi_id id;

	pr_debug("[SCP]id\trecv\tsuccess\tbusy\terror\tname\n\r");
	for (id = 0; id < SCP_NR_IPI; id++) {
		if (scp_ipi_desc[id].recv_count > 0 ||
			scp_ipi_desc[id].success_count > 0 ||
			scp_ipi_desc[id].busy_count > 0 ||
			scp_ipi_desc[id].error_count > 0)
			scp_ipi_info_dump(id);
	}
	pr_debug("ap->scp total=%u scp->ap total=%u\n\r", ap_to_scp_ipi_count,
		 scp_to_ap_ipi_count);
}
