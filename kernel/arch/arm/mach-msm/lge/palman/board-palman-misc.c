/*
  * Copyright (C) 2011,2012 LGE, Inc.
  *
  * Author: Sungwoo Cho <sungwoo.cho@lge.com>
  *
  * This software is licensed under the terms of the GNU General
  * License version 2, as published by the Free Software Foundation,
  * may be copied, distributed, and modified under those terms.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
  * GNU General Public License for more details.
  */

#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/gpio_keys.h>
#include <mach/board_lge.h>
#ifdef CONFIG_SLIMPORT_ANX7808
#include <linux/platform_data/slimport_device.h>
#endif
#include "board-palman.h"
#ifdef CONFIG_ANDROID_IRRC
#include <linux/android_irrc.h>
#endif

#ifdef CONFIG_SLIMPORT_ANX7808
#define GPIO_SLIMPORT_CBL_DET       PM8921_GPIO_PM_TO_SYS(14)
#define GPIO_SLIMPORT_PWR_DWN       PM8921_GPIO_PM_TO_SYS(15)
#define ANX_AVDD33_EN               PM8921_GPIO_PM_TO_SYS(17)
#define GPIO_SLIMPORT_RESET_N       31
#define GPIO_SLIMPORT_INT_N         32
#define GPIO_SLIMPORT_INT_N_REV_OLD 43

static int anx7808_dvdd_onoff(bool on)
{
	static bool power_state = 0;
	static struct regulator *anx7808_dvdd_reg = NULL;
	int rc = 0;

	if (power_state == on) {
		pr_info("anx7808 dvdd is already %s \n", power_state ? "on" : "off");
		goto out;
	}

	if (!anx7808_dvdd_reg) {
		anx7808_dvdd_reg= regulator_get(NULL, "slimport_dvdd");
		if (IS_ERR(anx7808_dvdd_reg)) {
			rc = PTR_ERR(anx7808_dvdd_reg);
			pr_err("%s: regulator_get anx7808_dvdd_reg failed. rc=%d\n",
					__func__, rc);
			anx7808_dvdd_reg = NULL;
			goto out;
		}
		rc = regulator_set_voltage(anx7808_dvdd_reg, 1100000, 1100000);
		if (rc ) {
			pr_err("%s: regulator_set_voltage anx7808_dvdd_reg failed\
				rc=%d\n", __func__, rc);
			goto out;
		}
	}

	if (on) {
		rc = regulator_set_optimum_mode(anx7808_dvdd_reg, 100000);
		if (rc < 0) {
			pr_err("%s : set optimum mode 100000, anx7808_dvdd_reg failed \
					(%d)\n", __func__, rc);
			goto out;
		}
		rc = regulator_enable(anx7808_dvdd_reg);
		if (rc) {
			pr_err("%s : anx7808_dvdd_reg enable failed (%d)\n",
					__func__, rc);
			goto out;
		}
	}
	else {
		rc = regulator_disable(anx7808_dvdd_reg);
		if (rc) {
			pr_err("%s : anx7808_dvdd_reg disable failed (%d)\n",
				__func__, rc);
			goto out;
		}
		rc = regulator_set_optimum_mode(anx7808_dvdd_reg, 100);
		if (rc < 0) {
			pr_err("%s : set optimum mode 100, anx7808_dvdd_reg failed \
				(%d)\n", __func__, rc);
			goto out;
		}
	}
	power_state = on;

out:
	return rc;

}

static int anx7808_avdd_onoff(bool on)
{
	static bool init_done=0;
	int rc = 0;

	if (!init_done) {
		rc = gpio_request_one(ANX_AVDD33_EN,
					GPIOF_OUT_INIT_HIGH, "anx_avdd33_en");
		if (rc) {
			pr_err("request anx_avdd33_en failed, rc=%d\n", rc);
			return rc;
		}
		init_done = 1;
	}

	gpio_set_value(ANX_AVDD33_EN, on);
	return 0;
}

static struct anx7808_platform_data anx7808_pdata = {
	.gpio_p_dwn = GPIO_SLIMPORT_PWR_DWN,
	.gpio_reset = GPIO_SLIMPORT_RESET_N,
	.gpio_int = GPIO_SLIMPORT_INT_N,
	.gpio_cbl_det = GPIO_SLIMPORT_CBL_DET,
	.dvdd_power = anx7808_dvdd_onoff,
	.avdd_power = anx7808_avdd_onoff,
};

struct i2c_board_info i2c_anx7808_info[] = {
	{
		I2C_BOARD_INFO("anx7808", 0x72 >> 1),
		.platform_data = &anx7808_pdata,
	},
};

static struct i2c_registry i2c_anx7808_devices __initdata = {
	I2C_FFA,
	APQ_8064_GSBI1_QUP_I2C_BUS_ID,
	i2c_anx7808_info,
	ARRAY_SIZE(i2c_anx7808_info),
};

struct pm_gpio pm_gpio_disable = {
	.direction	= PM_GPIO_DIR_IN,
	.output_buffer	= 0,
	.output_value	= 0,
	.pull		= 0,
	.vin_sel	= PM_GPIO_VIN_S4,
	.out_strength	= 0,
	.function	= 0,
	.inv_int_pol	= 0,
	.disable_pin	= 1,
};

static void __init lge_add_i2c_anx7808_device(void)
{
	pm8xxx_gpio_config(ANX_AVDD33_EN, &pm_gpio_disable);

	i2c_register_board_info(i2c_anx7808_devices.bus,
		i2c_anx7808_devices.info,
		i2c_anx7808_devices.len);
}
#endif /* CONFIG_SLIMPORT_ANX7808 */


/* Support cover-switch */
#define KEY_COVER_SWITCH       0  // KEY_RESERVED
#define GPIO_KEY_COVER_SWITCH  7

static struct gpio_keys_button palman_keys[] = {
	{
		.code       = KEY_COVER_SWITCH,
		.gpio       = GPIO_KEY_COVER_SWITCH,
		.desc       = "cover-switch",
		.active_low = 1,
		.type       = EV_SW,
		.wakeup     = 1,
		.debounce_interval = 15,
	},
};

static struct gpio_keys_platform_data palman_keys_data = {
	.buttons        = palman_keys,
	.nbuttons       = ARRAY_SIZE(palman_keys),
};

static struct platform_device palman_kp_pdev = {
	.name           = "gpio-keys",
	.id             = -1,
	.dev            = {
		.platform_data = &palman_keys_data,
	},
};

#ifdef CONFIG_ANDROID_IRRC
#define GPIO_IRRC_PWM	34

static int irrc_init(void)
{
	int rc;

	pr_info("%s\n", __func__);
	rc = gpio_request(GPIO_IRRC_PWM, "irrc_pwm");
	gpio_direction_output(GPIO_IRRC_PWM, 1);
	if (unlikely(rc < 0))
		ERR_MSG("not able to get gpio\n");

	return 0;
}

static struct android_irrc_platform_data irrc_data = {
	.enable_status = 0,
	.irrc_init = irrc_init,
};

static struct platform_device android_irrc_device = {
	.name = "android-irrc",
	.id = -1,
	.dev = {
		.platform_data = &irrc_data,
	},
};
#endif

static struct platform_device *misc_devices[] __initdata = {
	&palman_kp_pdev,
};

void __init apq8064_init_misc(void)
{
	platform_add_devices(misc_devices, ARRAY_SIZE(misc_devices));
#ifdef CONFIG_SLIMPORT_ANX7808
	lge_add_i2c_anx7808_device();
#endif

#ifdef CONFIG_ANDROID_IRRC
	platform_device_register(&android_irrc_device);
#endif
}
