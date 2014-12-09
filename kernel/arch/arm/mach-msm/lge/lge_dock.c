/* arch/arm/mach-msm/lge/lge_dock.c
 *
 * LGE Dock Driver.
 *
 * Copyright (C) 2013 LGE
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

#define pr_fmt(fmt) "%s %s: " fmt, "lge_dock", __func__

#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/switch.h>
#include <linux/power_supply.h>
#include <linux/mfd/pm8xxx/pm8xxx-adc.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <mach/board_lge.h>

enum {
	EXTRA_DOCK_STATE_UNDOCKED = 0,
	EXTRA_DOCK_STATE_DESK = 1,
	EXTRA_DOCK_STATE_CAR = 2,
	EXTRA_DOCK_STATE_LE_DESK = 3,
	EXTRA_DOCK_STATE_HE_DESK = 4
};

#define PHY_270K 560000
#define PHY_330K 735000

static DEFINE_MUTEX(dock_lock);
static bool dock_state;

static struct switch_dev dockdev = {
	.name = "dock",
};

extern bool slimport_is_connected(void);

static int check_dock_cable_type(void)
{
	struct pm8xxx_adc_chan_result adc_result;

	if (pm8xxx_adc_mpp_config_read(PM8XXX_AMUX_MPP_12, ADC_MPP_1_AMUX6, &adc_result) < 0)
		return -1;

	pr_debug("dock check - adc value: %d\n", (int)adc_result.physical);
	/* check 330k */
	mutex_lock(&dock_lock);
	if (PHY_270K <= (int)adc_result.physical &&
		(int)adc_result.physical <= PHY_330K) {
		dock_state = true;
	} else {
		dock_state = false;
	}
	return 0;
}

void check_dock_connected(enum power_supply_type type)
{
	if (check_dock_cable_type() < 0)
		pr_err("can't read adc!\n");

	if ((dock_state) &&
		!slimport_is_connected() && type) {
		switch_set_state(&dockdev, EXTRA_DOCK_STATE_DESK);
		pr_info("desk dock\n");
	} else {
		switch_set_state(&dockdev, EXTRA_DOCK_STATE_UNDOCKED);
		pr_debug("undocked\n");
	}
	mutex_unlock(&dock_lock);
}
EXPORT_SYMBOL(check_dock_connected);

static int lge_dock_probe(struct platform_device *pdev)
{
	dock_state = false;
	return 0;
}

static int __devexit lge_dock_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver dock_driver = {
	.probe  = lge_dock_probe,
	.remove = __devexit_p(lge_dock_remove),
	.driver = {
		.name = "lge_dock",
	},
};

static int __init lge_dock_init(void)
{
	int rc;
	rc = platform_driver_register(&dock_driver);
	if (switch_dev_register(&dockdev) < 0) {
		pr_err("failed to register dock driver.\n");
		rc = -ENODEV;
	}
	return rc;
}

static void __exit lge_dock_exit(void)
{
	switch_dev_unregister(&dockdev);
	platform_driver_unregister(&dock_driver);
}

module_init(lge_dock_init);
module_exit(lge_dock_exit);

MODULE_DESCRIPTION("LGE dock driver");
MODULE_LICENSE("GPL");
