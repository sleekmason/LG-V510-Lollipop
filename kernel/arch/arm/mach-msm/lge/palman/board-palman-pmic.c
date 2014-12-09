/* Copyright (c) 2011-2012, The Linux Foundation. All rights reserved.
 * Copyright (c) 2013, LGE Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/bootmem.h>
#include <linux/mfd/pm8xxx/pm8921.h>
#include <linux/mfd/pm8xxx/pm8xxx-adc.h>
#ifdef CONFIG_BATTERY_TEMP_CONTROL
#include <linux/platform_data/battery_temp_ctrl.h>
#endif
#include <asm/mach-types.h>
#include <asm/mach/mmc.h>
#include <mach/msm_bus_board.h>
#include <mach/board.h>
#include <mach/gpiomux.h>
#include <mach/restart.h>
#include <mach/socinfo.h>
#include "devices.h"
#include "board-palman.h"

struct pm8xxx_gpio_init {
	unsigned			gpio;
	struct pm_gpio			config;
};

struct pm8xxx_mpp_init {
	unsigned			mpp;
	struct pm8xxx_mpp_config_data	config;
};

#define PM8921_GPIO_INIT(_gpio, _dir, _buf, _val, _pull, _vin, _out_strength, \
			_func, _inv, _disable) \
{ \
	.gpio	= PM8921_GPIO_PM_TO_SYS(_gpio), \
	.config	= { \
		.direction	= _dir, \
		.output_buffer	= _buf, \
		.output_value	= _val, \
		.pull		= _pull, \
		.vin_sel	= _vin, \
		.out_strength	= _out_strength, \
		.function	= _func, \
		.inv_int_pol	= _inv, \
		.disable_pin	= _disable, \
	} \
}

#define PM8921_MPP_INIT(_mpp, _type, _level, _control) \
{ \
	.mpp	= PM8921_MPP_PM_TO_SYS(_mpp), \
	.config	= { \
		.type		= PM8XXX_MPP_TYPE_##_type, \
		.level		= _level, \
		.control	= PM8XXX_MPP_##_control, \
	} \
}

#define PM8821_MPP_INIT(_mpp, _type, _level, _control) \
{ \
	.mpp	= PM8821_MPP_PM_TO_SYS(_mpp), \
	.config	= { \
		.type		= PM8XXX_MPP_TYPE_##_type, \
		.level		= _level, \
		.control	= PM8XXX_MPP_##_control, \
	} \
}

#define PM8921_GPIO_DISABLE(_gpio) \
	PM8921_GPIO_INIT(_gpio, PM_GPIO_DIR_IN, 0, 0, 0, PM_GPIO_VIN_S4, \
			 0, 0, 0, 1)

#define PM8921_GPIO_OUTPUT(_gpio, _val, _strength) \
	PM8921_GPIO_INIT(_gpio, PM_GPIO_DIR_OUT, PM_GPIO_OUT_BUF_CMOS, _val, \
			PM_GPIO_PULL_NO, PM_GPIO_VIN_S4, \
			PM_GPIO_STRENGTH_##_strength, \
			PM_GPIO_FUNC_NORMAL, 0, 0)

#define PM8921_GPIO_OUTPUT_BUFCONF(_gpio, _val, _strength, _bufconf) \
	PM8921_GPIO_INIT(_gpio, PM_GPIO_DIR_OUT,\
			PM_GPIO_OUT_BUF_##_bufconf, _val, \
			PM_GPIO_PULL_NO, PM_GPIO_VIN_S4, \
			PM_GPIO_STRENGTH_##_strength, \
			PM_GPIO_FUNC_NORMAL, 0, 0)

#define PM8921_GPIO_INPUT(_gpio, _pull) \
	PM8921_GPIO_INIT(_gpio, PM_GPIO_DIR_IN, PM_GPIO_OUT_BUF_CMOS, 0, \
			_pull, PM_GPIO_VIN_S4, \
			PM_GPIO_STRENGTH_NO, \
			PM_GPIO_FUNC_NORMAL, 0, 0)

#define PM8921_GPIO_OUTPUT_FUNC(_gpio, _val, _func) \
	PM8921_GPIO_INIT(_gpio, PM_GPIO_DIR_OUT, PM_GPIO_OUT_BUF_CMOS, _val, \
			PM_GPIO_PULL_NO, PM_GPIO_VIN_S4, \
			PM_GPIO_STRENGTH_HIGH, \
			_func, 0, 0)

#define PM8921_GPIO_OUTPUT_VIN(_gpio, _val, _vin) \
	PM8921_GPIO_INIT(_gpio, PM_GPIO_DIR_OUT, PM_GPIO_OUT_BUF_CMOS, _val, \
			PM_GPIO_PULL_NO, _vin, \
			PM_GPIO_STRENGTH_HIGH, \
			PM_GPIO_FUNC_NORMAL, 0, 0)

/* Initial PM8921 GPIO configurations */
static struct pm8xxx_gpio_init pm8921_gpios[] __initdata = {
#ifdef CONFIG_SLIMPORT_ANX7808
	PM8921_GPIO_INPUT(14, PM_GPIO_PULL_DN),
	PM8921_GPIO_OUTPUT(15, 0, HIGH), /* ANX_P_DWN_CTL */
	PM8921_GPIO_OUTPUT(17, 0, HIGH), /* ANX_AVDD33_EN */
#else
	PM8921_GPIO_OUTPUT(14, 1, HIGH),	/* HDMI Mux Selector */
#endif
	PM8921_GPIO_OUTPUT(23, 0, HIGH),	/* touchscreen power FET */
	PM8921_GPIO_OUTPUT(27, 0, HIGH), /* MSM_MAINCAM_RST_EN */
	PM8921_GPIO_OUTPUT(28, 0, HIGH), /* VTCAM_RESET */
	PM8921_GPIO_OUTPUT_BUFCONF(36, 1, LOW, OPEN_DRAIN),
	PM8921_GPIO_OUTPUT_FUNC(44, 0, PM_GPIO_FUNC_2),
	PM8921_GPIO_OUTPUT(33, 0, HIGH),
#ifndef CONFIG_SND_SOC_TPA2028D_DUAL_SPEAKER
	PM8921_GPIO_OUTPUT(20, 0, HIGH),
	PM8921_GPIO_INPUT(35, PM_GPIO_PULL_UP_30),
#endif
	PM8921_GPIO_OUTPUT(38, 0, LOW),
	/* TABLA CODEC RESET */
	PM8921_GPIO_OUTPUT(34, 0, HIGH),
	PM8921_GPIO_OUTPUT(13, 0, HIGH),               /* PCIE_CLK_PWR_EN */
	/* Enable LDO for LCD_VDD */
	PM8921_GPIO_OUTPUT(26, 1, HIGH),
};

static struct pm8xxx_gpio_init pm8921_disabled_gpios[] __initdata= {
	PM8921_GPIO_DISABLE(3),
	PM8921_GPIO_DISABLE(4),
	PM8921_GPIO_DISABLE(5),
	PM8921_GPIO_DISABLE(6),
	PM8921_GPIO_DISABLE(8),
	PM8921_GPIO_DISABLE(11),
	PM8921_GPIO_DISABLE(12),
	PM8921_GPIO_DISABLE(17),
	PM8921_GPIO_DISABLE(18),
	PM8921_GPIO_DISABLE(36),
	PM8921_GPIO_DISABLE(38),
	PM8921_GPIO_DISABLE(40),
	PM8921_GPIO_DISABLE(41),
	PM8921_GPIO_DISABLE(44),
};

static struct pm8xxx_gpio_init pm8921_mtp_kp_gpios[] __initdata = {
	PM8921_GPIO_INPUT(3, PM_GPIO_PULL_UP_30),
	PM8921_GPIO_INPUT(4, PM_GPIO_PULL_UP_30),
};

static struct pm8xxx_gpio_init pm8921_cdp_kp_gpios[] __initdata = {
	PM8921_GPIO_INPUT(27, PM_GPIO_PULL_UP_30),
	PM8921_GPIO_INPUT(42, PM_GPIO_PULL_UP_30),
};

/* Initial PM8917 GPIO configurations */
static struct pm8xxx_gpio_init pm8917_gpios[] __initdata = {
	PM8921_GPIO_OUTPUT(14, 1, HIGH),	/* HDMI Mux Selector */
	PM8921_GPIO_OUTPUT(23, 0, HIGH),	/* touchscreen power FET */
	PM8921_GPIO_OUTPUT_BUFCONF(25, 0, LOW, CMOS), /* DISP_RESET_N */
	PM8921_GPIO_OUTPUT(26, 1, HIGH), /* Backlight: on */
	PM8921_GPIO_OUTPUT_BUFCONF(36, 1, LOW, OPEN_DRAIN),
	PM8921_GPIO_OUTPUT_FUNC(38, 0, PM_GPIO_FUNC_2),
	PM8921_GPIO_OUTPUT(33, 0, HIGH),
	PM8921_GPIO_OUTPUT(20, 0, HIGH),
	PM8921_GPIO_INPUT(35, PM_GPIO_PULL_UP_30),
	PM8921_GPIO_INPUT(30, PM_GPIO_PULL_UP_30),
	/* TABLA CODEC RESET */
	PM8921_GPIO_OUTPUT(34, 1, MED),
	PM8921_GPIO_OUTPUT(13, 0, HIGH),               /* PCIE_CLK_PWR_EN */
	PM8921_GPIO_INPUT(12, PM_GPIO_PULL_UP_30),     /* PCIE_WAKE_N */
};

/* PM8921 GPIO 42 remaps to PM8917 GPIO 8 */
static struct pm8xxx_gpio_init pm8917_cdp_kp_gpios[] __initdata = {
	PM8921_GPIO_INPUT(27, PM_GPIO_PULL_UP_30),
	PM8921_GPIO_INPUT(8, PM_GPIO_PULL_UP_30),
	PM8921_GPIO_INPUT(17, PM_GPIO_PULL_UP_1P5),	/* SD_WP */
};

static struct pm8xxx_gpio_init pm8921_mpq_gpios[] __initdata = {
	PM8921_GPIO_INIT(27, PM_GPIO_DIR_IN, PM_GPIO_OUT_BUF_CMOS, 0,
			PM_GPIO_PULL_NO, PM_GPIO_VIN_VPH, PM_GPIO_STRENGTH_NO,
			PM_GPIO_FUNC_NORMAL, 0, 0),
};

static struct pm8xxx_mpp_init pm8xxx_disabled_mpps[] __initdata = {
		PM8921_MPP_INIT(2, SINK, PM8XXX_MPP_CS_OUT_5MA, CS_CTRL_DISABLE),
		PM8921_MPP_INIT(9, SINK, PM8XXX_MPP_CS_OUT_5MA, CS_CTRL_DISABLE),
		PM8921_MPP_INIT(10, SINK, PM8XXX_MPP_CS_OUT_5MA, CS_CTRL_DISABLE),
		PM8921_MPP_INIT(11, SINK, PM8XXX_MPP_CS_OUT_5MA, CS_CTRL_DISABLE),
};

/* Initial PM8XXX MPP configurations */
static struct pm8xxx_mpp_init pm8xxx_mpps[] __initdata = {
	PM8921_MPP_INIT(3, D_OUTPUT, PM8921_MPP_DIG_LEVEL_VPH, DOUT_CTRL_LOW),
	/* External 5V regulator enable; shared by HDMI and USB_OTG switches. */
	PM8921_MPP_INIT(7, D_OUTPUT, PM8921_MPP_DIG_LEVEL_VPH, DOUT_CTRL_LOW),
	PM8921_MPP_INIT(8, D_OUTPUT, PM8921_MPP_DIG_LEVEL_S4, DOUT_CTRL_LOW),
	/*MPP9 is used to detect docking station connection/removal on Liquid*/
	PM8921_MPP_INIT(9, D_INPUT, PM8921_MPP_DIG_LEVEL_S4, DIN_TO_INT),
	/*MPP1 is used to read ADC for headset 3 Button read key*/
	PM8921_MPP_INIT(1, D_INPUT, PM8921_MPP_DIG_LEVEL_S4, DIN_TO_INT),
};

#ifdef CONFIG_SWITCH_MAX1462X
static struct pm8xxx_gpio_init pm8921_gpios_audio[] __initdata = {
	PM8921_GPIO_OUTPUT(31, 0, HIGH), /* PMIC - MAX1462X_EAR_MIC_EN */
	PM8921_GPIO_OUTPUT(32, 0, LOW), /* PMIC - EXT_EAR_MIC_BIAS_EN */
};
#endif


void __init apq8064_configure_gpios(struct pm8xxx_gpio_init *data, int len)
{
	int i, rc;

	for (i = 0; i < len; i++) {
		rc = pm8xxx_gpio_config(data[i].gpio, &data[i].config);
		if (rc)
			pr_err("%s: pm8xxx_gpio_config(%u) failed: rc=%d\n",
				__func__, data[i].gpio, rc);
	}
}

void __init apq8064_pm8xxx_gpio_mpp_init(void)
{
	int i, rc;

	apq8064_configure_gpios(pm8921_disabled_gpios, ARRAY_SIZE(pm8921_disabled_gpios));

	for (i = 0; i < ARRAY_SIZE(pm8xxx_disabled_mpps); i++) {
		rc = pm8xxx_mpp_config(pm8xxx_disabled_mpps[i].mpp,
					&pm8xxx_disabled_mpps[i].config);
		if (rc) {
			pr_err("%s: pm8xxx_mpp_config: rc=%d\n", __func__, rc);
			break;
		}
	}

	if (socinfo_get_pmic_model() != PMIC_MODEL_PM8917)
		apq8064_configure_gpios(pm8921_gpios, ARRAY_SIZE(pm8921_gpios));
	else
		apq8064_configure_gpios(pm8917_gpios, ARRAY_SIZE(pm8917_gpios));

	if (machine_is_apq8064_cdp() || machine_is_apq8064_liquid()) {
		if (socinfo_get_pmic_model() != PMIC_MODEL_PM8917)
			apq8064_configure_gpios(pm8921_cdp_kp_gpios,
					ARRAY_SIZE(pm8921_cdp_kp_gpios));
		else
			apq8064_configure_gpios(pm8917_cdp_kp_gpios,
					ARRAY_SIZE(pm8917_cdp_kp_gpios));
	}

	if (machine_is_apq8064_mtp())
		apq8064_configure_gpios(pm8921_mtp_kp_gpios,
					ARRAY_SIZE(pm8921_mtp_kp_gpios));

	if (machine_is_mpq8064_cdp() || machine_is_mpq8064_hrd()
	    || machine_is_mpq8064_dtv())
		apq8064_configure_gpios(pm8921_mpq_gpios,
					ARRAY_SIZE(pm8921_mpq_gpios));

#ifdef CONFIG_SWITCH_MAX1462X
	apq8064_configure_gpios(pm8921_gpios_audio, ARRAY_SIZE(pm8921_gpios_audio));
#endif

	for (i = 0; i < ARRAY_SIZE(pm8xxx_mpps); i++) {
		rc = pm8xxx_mpp_config(pm8xxx_mpps[i].mpp,
					&pm8xxx_mpps[i].config);
		if (rc) {
			pr_err("%s: pm8xxx_mpp_config: rc=%d\n", __func__, rc);
			break;
		}
	}
}

static struct pm8xxx_pwrkey_platform_data apq8064_pm8921_pwrkey_pdata = {
	.pull_up		= 1,
	.kpd_trigger_delay_us	= 15625,
	.wakeup			= 1,
};

static struct pm8xxx_misc_platform_data apq8064_pm8921_misc_pdata = {
	.priority		= 0,
};


static struct pm8xxx_adc_amux apq8064_pm8921_adc_channels_data[] = {
	{"vcoin", CHANNEL_VCOIN, CHAN_PATH_SCALING2, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"vbat", CHANNEL_VBAT, CHAN_PATH_SCALING2, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"dcin", CHANNEL_DCIN, CHAN_PATH_SCALING4, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"ichg", CHANNEL_ICHG, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"vph_pwr", CHANNEL_VPH_PWR, CHAN_PATH_SCALING2, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"ibat", CHANNEL_IBAT, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"batt_therm", CHANNEL_BATT_THERM, CHAN_PATH_SCALING1, AMUX_RSV2,
		ADC_DECIMATION_TYPE2, ADC_SCALE_BATT_THERM},
	{"batt_id", CHANNEL_BATT_ID, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"usbin", CHANNEL_USBIN, CHAN_PATH_SCALING3, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"pmic_therm", CHANNEL_DIE_TEMP, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_PMIC_THERM},
	{"625mv", CHANNEL_625MV, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"125v", CHANNEL_125V, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"chg_temp", CHANNEL_CHG_TEMP, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"xo_therm", CHANNEL_MUXOFF, CHAN_PATH_SCALING1, AMUX_RSV0,
		ADC_DECIMATION_TYPE2, ADC_SCALE_XOTHERM},
	{"pa_therm0", ADC_MPP_1_AMUX3, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_APQ_THERM},
#ifdef CONFIG_SWITCH_MAX1462X
	{"ear_mic_jack", ADC_MPP_1_AMUX6, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
#endif
};

static struct pm8xxx_adc_properties apq8064_pm8921_adc_data = {
	.adc_vdd_reference	= 1800, /* milli-voltage for this adc */
	.bitresolution		= 15,
	.bipolar                = 0,
};

static struct pm8xxx_adc_platform_data apq8064_pm8921_adc_pdata = {
	.adc_channel		= apq8064_pm8921_adc_channels_data,
	.adc_num_board_channel	= ARRAY_SIZE(apq8064_pm8921_adc_channels_data),
	.adc_prop		= &apq8064_pm8921_adc_data,
	.adc_mpp_base		= PM8921_MPP_PM_TO_SYS(1),
	.apq_therm		= true,
};

static struct pm8xxx_mpp_platform_data
apq8064_pm8921_mpp_pdata __devinitdata = {
	.mpp_base	= PM8921_MPP_PM_TO_SYS(1),
};

static struct pm8xxx_gpio_platform_data
apq8064_pm8921_gpio_pdata __devinitdata = {
	.gpio_base	= PM8921_GPIO_PM_TO_SYS(1),
};

static struct pm8xxx_irq_platform_data
apq8064_pm8921_irq_pdata __devinitdata = {
	.irq_base		= PM8921_IRQ_BASE,
	.devirq			= MSM_GPIO_TO_INT(74),
	.irq_trigger_flag	= IRQF_TRIGGER_LOW,
	.dev_id			= 0,
};

static struct pm8xxx_rtc_platform_data
apq8064_pm8921_rtc_pdata = {
	.rtc_write_enable       = false,
	.rtc_alarm_powerup      = false,
};

static int apq8064_pm8921_therm_mitigation[] = {
	1100,
	700,
	600,
	325,
};

#ifdef CONFIG_PM8921_CHARGER_AICL_SELECTION
static int apq8064_pm8921_iusbmax_therm_mitigation[] = {
	1500,
	1300,
	1100,
	900,
	500,
};
#endif

/*
 * Battery characteristic
 * Typ.4600mAh capacity, Li-Ion Polymer 3.75V
 * Battery/VDD voltage programmable range, 20mV steps.
 */
#define MAX_VOLTAGE_MV		4320
#define CHG_TERM_MA		200
#define MAX_BATT_CHG_I_MA	2000
#define WARM_BATT_CHG_I_MA	350
#define VBATDET_DELTA_MV	50

static struct pm8921_charger_platform_data
	apq8064_pm8921_chg_pdata __devinitdata = {
	.safety_time  = 512,
	.update_time  = 60000,
	.max_voltage  = MAX_VOLTAGE_MV,
	.min_voltage  = 3200,
	.alarm_voltage  = 3400,
	.resume_voltage_delta  = VBATDET_DELTA_MV,
	.term_current  = CHG_TERM_MA,

	.cool_temp  = INT_MIN,
	.warm_temp  = INT_MIN,
	.cool_bat_chg_current  = 350,
	.warm_bat_chg_current  = WARM_BATT_CHG_I_MA,
	.cold_thr  = 1,
	.hot_thr  = 0,
	.ext_batt_temp_monitor  = 1,
	.temp_check_period  = 1,
	.max_bat_chg_current  = MAX_BATT_CHG_I_MA,
	.cool_bat_voltage  = 4100,
	.warm_bat_voltage  = 4100,
	.thermal_mitigation  = apq8064_pm8921_therm_mitigation,
	.thermal_levels  = ARRAY_SIZE(apq8064_pm8921_therm_mitigation),
	.led_src_config  = LED_SRC_MIN_VPH_5V,
	.rconn_mohm	 = 18,
#ifdef CONFIG_PM8921_CHARGER_AICL_SELECTION
	.disable_aicl = 1,
	.iusbmax_thermal_mitigation = apq8064_pm8921_iusbmax_therm_mitigation,
	.iusbmax_thermal_levels =
		ARRAY_SIZE(apq8064_pm8921_iusbmax_therm_mitigation),
#endif
};

static struct pm8xxx_ccadc_platform_data
apq8064_pm8xxx_ccadc_pdata = {
	.r_sense		= 10,
	.calib_delay_ms		= 600000,
};

static struct pm8921_bms_platform_data
apq8064_pm8921_bms_pdata __devinitdata = {
	.battery_type			= BATT_LGE_4600,
	.r_sense			= 10,
	.v_cutoff			= 3400,
	.max_voltage_uv			= MAX_VOLTAGE_MV * 1000,
	.rconn_mohm			= 18,
	.shutdown_soc_valid_limit	= 20,
	.adjust_soc_low_threshold	= 25,
	.chg_term_ua			= CHG_TERM_MA * 1000,
};

/* battery data */
static struct single_row_lut batt_4600_fcc_temp = {
	.x		= {-20, 0, 25, 40, 60},
	.y		= {4635, 4639, 4636, 4625, 4586},
	.cols	= 5
};

static struct single_row_lut batt_4600_fcc_sf = {
	.x		= {0},
	.y		= {100},
	.cols	= 1
};

static struct sf_lut batt_4600_rbatt_sf = {
	.rows		= 30,
	.cols		= 5,
	.row_entries		= {-20, 0, 25, 40, 60},
	.percent	= {100, 95, 90, 85, 80, 75, 70, 65, 60, 55,
		50, 45, 40, 35, 30, 25, 20, 16, 13,	11,
		10, 9, 8, 7, 6, 5, 4, 3, 2, 1},
	.sf		= {
				{1115, 209, 100, 91, 92},
				{1115, 209, 100, 91, 92},
				{1133, 216, 101, 92, 92},
				{1178, 222, 103, 94, 94},
				{1088, 240, 106, 95, 95},
				{1106, 250, 110, 98, 96},
				{1110, 248, 113, 100, 97},
				{1121, 247, 120, 103, 99},
				{1147, 229, 127, 109, 103},
				{1186, 223, 105, 95, 94},
				{1235, 226, 103, 94, 94},
				{1290, 232, 106, 96, 95},
				{1353, 241, 107, 97, 96},
				{1432, 256, 108, 99, 97},
				{1520, 280, 110, 96, 94},
				{1610, 309, 111, 96, 94},
				{1709, 345, 112, 97, 95},
				{1788, 377, 115, 98, 94},
				{1848, 414, 117, 99, 96},
				{1842, 442, 121, 100, 96},
				{1855, 459, 124, 102, 96},
				{1894, 480, 126, 102, 98},
				{1941, 505, 130, 102, 98},
				{1997, 527, 130, 102, 97},
				{2060, 544, 128, 102, 97},
				{2140, 562, 131, 103, 98},
				{2321, 593, 134, 104, 99},
				{2769, 635, 141, 108, 101},
				{3503, 693, 150, 115, 105},
				{4852, 784, 176, 140, 123},
	}
};

static struct pc_temp_ocv_lut batt_4600_pc_temp_ocv = {
	.rows		= 31,
	.cols		= 5,
	.temp		= {-20, 0, 25, 40, 60},
	.percent	= {100, 95, 90, 85, 80, 75, 70, 65, 60, 55,
		50, 45, 40, 35, 30, 25, 20, 16, 13, 11,
		10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0},
	.ocv		= {
				{4280, 4278, 4273, 4269, 4262},
				{4187, 4209, 4210, 4208, 4203},
				{4124, 4155, 4156, 4154, 4150},
				{4078, 4104, 4105, 4104, 4100},
				{3991, 4058, 4060, 4057, 4053},
				{3950, 4007, 4013, 4012, 4010},
				{3908, 3963, 3974, 3974, 3970},
				{3871, 3922, 3939, 3939, 3935},
				{3844, 3877, 3900, 3902, 3900},
				{3826, 3844, 3852, 3852, 3851},
				{3810, 3819, 3826, 3826, 3825},
				{3795, 3800, 3807, 3806, 3806},
				{3780, 3786, 3790, 3790, 3789},
				{3764, 3776, 3776, 3776, 3775},
				{3747, 3766, 3766, 3760, 3750},
				{3728, 3750, 3753, 3743, 3729},
				{3705, 3723, 3728, 3719, 3707},
				{3682, 3698, 3695, 3687, 3671},
				{3662, 3685, 3679, 3672, 3661},
				{3647, 3678, 3675, 3669, 3658},
				{3638, 3674, 3673, 3667, 3656},
				{3626, 3670, 3671, 3664, 3653},
				{3612, 3664, 3666, 3659, 3647},
				{3595, 3651, 3655, 3645, 3629},
				{3572, 3628, 3626, 3615, 3596},
				{3543, 3591, 3583, 3572, 3552},
				{3505, 3540, 3529, 3517, 3498},
				{3451, 3474, 3460, 3449, 3430},
				{3374, 3384, 3369, 3358, 3340},
				{3252, 3250, 3238, 3226, 3212},
				{3000, 3000, 3000, 3000, 3000}
	}
};

struct pm8921_bms_battery_data lge_4600_palman_data = {
	.fcc				= 4600,
	.fcc_temp_lut			= &batt_4600_fcc_temp,
	.fcc_sf_lut				= &batt_4600_fcc_sf,
	.pc_temp_ocv_lut		= &batt_4600_pc_temp_ocv,
	.rbatt_sf_lut			= &batt_4600_rbatt_sf,
	.default_rbatt_mohm	= 102
};

static unsigned int keymap[] = {
	KEY(0, 0, KEY_VOLUMEUP),
	KEY(0, 1, KEY_VOLUMEDOWN),
};

static struct matrix_keymap_data keymap_data = {
	.keymap_size    = ARRAY_SIZE(keymap),
	.keymap         = keymap,
};

static struct pm8xxx_keypad_platform_data keypad_data = {
	.input_name             = "gk-keypad-8064",
	.input_phys_device      = "gk-keypad-8064/input0",
	.num_rows               = 1,
	.num_cols               = 5,
	.rows_gpio_start	= PM8921_GPIO_PM_TO_SYS(9),
	.cols_gpio_start	= PM8921_GPIO_PM_TO_SYS(1),
	.debounce_ms            = 15,
	.scan_delay_ms          = 32,
	.row_hold_ns            = 91500,
	.wakeup                 = 1,
	.keymap_data            = &keymap_data,
};

#ifdef CONFIG_PMIC8XXX_VIBRATOR
static struct pm8xxx_vibrator_platform_data pm8xxx_vibrator_pdata = {
	.initial_vibrate_ms = 0,
	.max_timeout_ms = 30000,
	.level_mV =2300,
};
#endif

static struct pm8921_platform_data
apq8064_pm8921_platform_data __devinitdata = {
	.irq_pdata		= &apq8064_pm8921_irq_pdata,
	.gpio_pdata		= &apq8064_pm8921_gpio_pdata,
	.mpp_pdata		= &apq8064_pm8921_mpp_pdata,
	.rtc_pdata		= &apq8064_pm8921_rtc_pdata,
	.pwrkey_pdata		= &apq8064_pm8921_pwrkey_pdata,
	.keypad_pdata		= &keypad_data,
	.misc_pdata		= &apq8064_pm8921_misc_pdata,
	.adc_pdata		= &apq8064_pm8921_adc_pdata,
	.charger_pdata		= &apq8064_pm8921_chg_pdata,
	.bms_pdata		= &apq8064_pm8921_bms_pdata,
	.ccadc_pdata		= &apq8064_pm8xxx_ccadc_pdata,
#ifdef CONFIG_PMIC8XXX_VIBRATOR
	.vibrator_pdata	=	&pm8xxx_vibrator_pdata,
#endif
};

static struct pm8xxx_irq_platform_data
apq8064_pm8821_irq_pdata __devinitdata = {
	.irq_base		= PM8821_IRQ_BASE,
	.devirq			= PM8821_SEC_IRQ_N,
	.irq_trigger_flag	= IRQF_TRIGGER_HIGH,
	.dev_id			= 1,
};

static struct pm8xxx_mpp_platform_data
apq8064_pm8821_mpp_pdata __devinitdata = {
	.mpp_base	= PM8821_MPP_PM_TO_SYS(1),
};

static struct pm8821_platform_data
apq8064_pm8821_platform_data __devinitdata = {
	.irq_pdata	= &apq8064_pm8821_irq_pdata,
	.mpp_pdata	= &apq8064_pm8821_mpp_pdata,
};

static struct msm_ssbi_platform_data apq8064_ssbi_pm8921_pdata __devinitdata = {
	.controller_type = MSM_SBI_CTRL_PMIC_ARBITER,
	.slave	= {
		.name		= "pm8921-core",
		.platform_data	= &apq8064_pm8921_platform_data,
	},
};

static struct msm_ssbi_platform_data apq8064_ssbi_pm8821_pdata __devinitdata = {
	.controller_type = MSM_SBI_CTRL_PMIC_ARBITER,
	.slave	= {
		.name		= "pm8821-core",
		.platform_data	= &apq8064_pm8821_platform_data,
	},
};

#ifdef CONFIG_BATTERY_TEMP_CONTROL
static int batt_temp_ctrl_level[] = {
	600,
	570,
	550,
	450,
	440,
	-50,
	-80,
	-100,
};

static int batt_temp_charger_enable(void)
{
	int ret = 0;

	pr_info("%s\n", __func__);

	ret = pm8921_charger_enable(1);
	if (ret)
		pr_err("%s: failed to enable charging\n", __func__);

	return ret;
}

static int batt_temp_charger_disable(void)
{
	int ret = 0;

	pr_info("%s\n", __func__);

	ret = pm8921_charger_enable(0);
	if (ret)
		pr_err("%s: failed to disable charging\n", __func__);

	return ret;
}

static int batt_temp_ext_power_plugged(void)
{
	if (pm8921_is_usb_chg_plugged_in() ||
			pm8921_is_dc_chg_plugged_in())
		return 1;
	else
		return 0;
}

static int batt_temp_set_current_limit(int value)
{
	int ret = 0;

	pr_info("%s: value = %d\n", __func__, value);

	ret = pm8921_set_max_battery_charge_current(value);
	if (ret)
		pr_err("%s: failed to set i limit\n", __func__);
	return ret;
}

static int batt_temp_get_current_limit(void)
{
	static struct power_supply *psy;
	union power_supply_propval ret = {0,};
	int rc = 0;

	if (psy == NULL) {
		psy = power_supply_get_by_name("usb");
		if (!psy) {
			pr_err("%s: failed to get usb power supply\n", __func__);
			return 0;
		}
	}

	rc = psy->get_property(psy, POWER_SUPPLY_PROP_CURRENT_MAX, &ret);
	if (rc) {
		pr_err("%s: failed to get usb property\n", __func__);
		return 0;
	}
	pr_info("%s: value = %d\n", __func__, ret.intval);
	return ret.intval;
}

static int batt_temp_set_state(int health, int i_value)
{
	int ret = 0;

	ret = pm8921_set_ext_battery_health(health, i_value);
	if (ret)
		pr_err("%s: failed to set health\n", __func__);

	return ret;
}

static struct batt_temp_pdata palman_batt_temp_pada = {
	.set_chg_i_limit = batt_temp_set_current_limit,
	.get_chg_i_limit = batt_temp_get_current_limit,
	.set_health_state = batt_temp_set_state,
	.enable_charging = batt_temp_charger_enable,
	.disable_charging = batt_temp_charger_disable,
	.is_ext_power = batt_temp_ext_power_plugged,
	.update_time = 10000, // 10 sec
	.temp_level = batt_temp_ctrl_level,
	.temp_nums = ARRAY_SIZE(batt_temp_ctrl_level),
	.thr_mvolt = 4000, //4.0V
	.i_decrease = WARM_BATT_CHG_I_MA,
	.i_restore = MAX_BATT_CHG_I_MA,
};

struct platform_device batt_temp_ctrl = {
	.name = "batt_temp_ctrl",
	.id = -1,
	.dev = {
		.platform_data = &palman_batt_temp_pada,
	},
};
#endif

void __init apq8064_init_pmic(void)
{
	pmic_reset_irq = PM8921_IRQ_BASE + PM8921_RESOUT_IRQ;

	apq8064_device_ssbi_pmic1.dev.platform_data =
						&apq8064_ssbi_pm8921_pdata;
	apq8064_device_ssbi_pmic2.dev.platform_data =
				&apq8064_ssbi_pm8821_pdata;
	if (socinfo_get_pmic_model() != PMIC_MODEL_PM8917) {
		apq8064_pm8921_platform_data.regulator_pdatas
			= msm8064_pm8921_regulator_pdata;
		apq8064_pm8921_platform_data.num_regulators
			= msm8064_pm8921_regulator_pdata_len;
	} else {
		apq8064_pm8921_platform_data.regulator_pdatas
			= msm8064_pm8917_regulator_pdata;
		apq8064_pm8921_platform_data.num_regulators
			= msm8064_pm8917_regulator_pdata_len;
	}

#ifdef CONFIG_BATTERY_TEMP_CONTROL
	platform_device_register(&batt_temp_ctrl);
#endif
}
