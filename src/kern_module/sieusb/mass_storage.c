/*
 * mass_storage.c -- Mass Storage USB Gadget
 *
 * Copyright (C) 2003-2008 Alan Stern
 * Copyright (C) 2009 Samsung Electronics
 *                    Author: Michal Nazarewicz <mina86@mina86.com>
 * Copyright (C) 2020 Sony Interactive Entertainment Inc.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <linux/kernel.h>
#include <linux/usb/ch9.h>
#include <linux/module.h>

#include "f_mass_storage.h"
#include "usb.h"

static struct usb_function_instance *fi_msg;
static struct usb_function *f_msg;

/****************************** Configurations ******************************/

static struct fsg_module_parameters mod_data = {
	.stall = 1
};
FSG_MODULE_PARAMETERS(/* no prefix */, mod_data);

int usb_msc_init(struct usb_composite_dev *cdev, struct usb_function **interface)
{
	struct fsg_opts *opts;
	struct fsg_config config;
	int status;

	GADGET_LOG(LOG_DBG, "\n");

	fi_msg = usb_get_function_instance("mass_storage");
	if (IS_ERR(fi_msg))
		return PTR_ERR(fi_msg);

	fsg_config_from_params(&config, &mod_data, CONFIG_USB_GADGET_STORAGE_NUM_BUFFERS);
	opts = fsg_opts_from_func_inst(fi_msg);

	opts->no_configfs = true;
	status = fsg_common_set_num_buffers(opts->common, CONFIG_USB_GADGET_STORAGE_NUM_BUFFERS);
	if (status)
		goto fail;

	status = fsg_common_set_cdev(opts->common, cdev, config.can_stall);
	if (status)
		goto fail_set_cdev;

	fsg_common_set_sysfs(opts->common, true);
	status = fsg_common_create_luns(opts->common, &config);
	if (status)
		goto fail_set_cdev;

	fsg_common_set_inquiry_string(opts->common, config.vendor_name,
				      config.product_name);

	f_msg = usb_get_function(fi_msg);
	if (IS_ERR(f_msg)) {
		status = PTR_ERR(f_msg);
		goto fail_get_function;
	}

	*interface = f_msg;

	return 0;

fail_get_function:
	fsg_common_remove_luns(opts->common);
fail_set_cdev:
	fsg_common_free_buffers(opts->common);
fail:
	usb_put_function_instance(fi_msg);
	return status;
}

void usb_msc_destroy(void)
{
	GADGET_LOG(LOG_DBG, "\n");

	if (!IS_ERR(f_msg))
		usb_put_function(f_msg);

	if (!IS_ERR(fi_msg))
		usb_put_function_instance(fi_msg);
}

MODULE_LICENSE("GPL");
