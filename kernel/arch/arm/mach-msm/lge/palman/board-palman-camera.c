/* Copyright (c) 2012, The Linux Foundation. All rights reserved.
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

#include <asm/mach-types.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <mach/board.h>
#include <mach/msm_bus_board.h>
#include <mach/gpiomux.h>
#include <mach/socinfo.h>
#include <media/msm_camera.h>
#include "devices.h"
#include "board-palman.h"
#include <mach/board_lge.h>

#ifdef CONFIG_MSM_CAMERA
static struct gpiomux_setting cam_settings[] = {
	{
		.func = GPIOMUX_FUNC_GPIO, /*suspend*/
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_DOWN,
	},

	{
		.func = GPIOMUX_FUNC_1, /*active 1*/
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
	},

	{
		.func = GPIOMUX_FUNC_GPIO, /*active 2*/
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
	},

	{
		.func = GPIOMUX_FUNC_1, /*active 3*/
		.drv = GPIOMUX_DRV_8MA,
		.pull = GPIOMUX_PULL_NONE,
	},

	{
		.func = GPIOMUX_FUNC_4, /*active 4*/
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
	},

	{
		.func = GPIOMUX_FUNC_6, /*active 5*/
		.drv = GPIOMUX_DRV_8MA,
		.pull = GPIOMUX_PULL_UP,
	},

	{
		.func = GPIOMUX_FUNC_2, /*active 6*/
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_UP,
	},

	{
		.func = GPIOMUX_FUNC_3, /*active 7*/
		.drv = GPIOMUX_DRV_8MA,
		.pull = GPIOMUX_PULL_UP,
	},

	{
		.func = GPIOMUX_FUNC_GPIO, /*i2c suspend*/
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_KEEPER,
	},

	{
		.func = GPIOMUX_FUNC_9, /*active 9*/
		.drv = GPIOMUX_DRV_8MA,
		.pull = GPIOMUX_PULL_NONE,
	},
	{
		.func = GPIOMUX_FUNC_A, /*active 10*/
		.drv = GPIOMUX_DRV_8MA,
		.pull = GPIOMUX_PULL_NONE,
	},
	{
		.func = GPIOMUX_FUNC_6, /*active 11*/
		.drv = GPIOMUX_DRV_8MA,
		.pull = GPIOMUX_PULL_NONE,
	},
	{
		.func = GPIOMUX_FUNC_4, /*active 12*/
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
	},

};

static struct msm_gpiomux_config apq8064_cam_common_configs[] = {
	{
		.gpio = GPIO_CAM_FLASH_EN, /* 7 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[2],
			[GPIOMUX_SUSPENDED] = &cam_settings[0],
		},
	},
	{
		.gpio = GPIO_CAM_MCLK0, /* 5 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[1],
			[GPIOMUX_SUSPENDED] = &cam_settings[0],
		},
	},
	{
		.gpio = GPIO_CAM_MCLK2, /* 2 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[4],
			[GPIOMUX_SUSPENDED] = &cam_settings[0],
		},
	},
	{
		.gpio = GPIO_CAM_I2C_SDA, /* 12 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[8],
		},
	},
	{
		.gpio = GPIO_CAM_I2C_SCL, /* 13 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[8],
		},
	},
};

static struct msm_gpiomux_config apq8064_cam_2d_configs[] = {
};

static struct gpio apq8064_common_cam_gpio[] = {
	{12, GPIOF_DIR_IN, "CAMIF_I2C_DATA"},
	{13, GPIOF_DIR_IN, "CAMIF_I2C_CLK"},
};

static struct msm_bus_vectors cam_init_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
};

static struct msm_bus_vectors cam_preview_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 94003200,
		.ib  = 110592000,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
};

static struct msm_bus_vectors cam_video_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 140451840,
		.ib  = 561807360,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 206807040,
		.ib  = 488816640,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
};

static struct msm_bus_vectors cam_snapshot_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 411635520,
		.ib  = 1646542080,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 540000000,
		.ib  = 1350000000,
	},
};

static struct msm_bus_vectors cam_zsl_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 411635520,
		.ib  = 1812430080,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 810000000,
		.ib  = 2025000000,
	},
};

static struct msm_bus_paths cam_bus_client_config[] = {
	{
		ARRAY_SIZE(cam_init_vectors),
		cam_init_vectors,
	},
	{
		ARRAY_SIZE(cam_preview_vectors),
		cam_preview_vectors,
	},
	{
		ARRAY_SIZE(cam_video_vectors),
		cam_video_vectors,
	},
	{
		ARRAY_SIZE(cam_snapshot_vectors),
		cam_snapshot_vectors,
	},
	{
		ARRAY_SIZE(cam_zsl_vectors),
		cam_zsl_vectors,
	},
};

static struct msm_bus_scale_pdata cam_bus_client_pdata = {
		cam_bus_client_config,
		ARRAY_SIZE(cam_bus_client_config),
		.name = "msm_camera",
};

static struct msm_camera_device_platform_data msm_camera_csi_device_data[] = {
	{
		.csid_core = 0,
		.is_vpe    = 1,
		.cam_bus_scale_table = &cam_bus_client_pdata,
	},
	{
		.csid_core = 1,
		.is_vpe    = 1,
		.cam_bus_scale_table = &cam_bus_client_pdata,
	},
};

#if defined(CONFIG_S5K4E5YA)
static struct camera_vreg_t apq_8064_back_cam_vreg[] = {
	{"cam1_vdig", REG_LDO, 1500000, 1500000, 105000, 0},
	{"cam1_vana", REG_LDO, 2800000, 2850000, 85600, 0},
	{"cam1_vio", REG_VS, 0, 0, 0, 0},		
};

static struct gpio apq8064_back_cam_gpio[] = {
	{GPIO_CAM_MCLK0, GPIOF_DIR_IN, "CAMIF_MCLK"},
};

static struct msm_camera_gpio_conf apq8064_back_cam_gpio_conf = {
	.cam_gpiomux_conf_tbl = apq8064_cam_2d_configs,
	.cam_gpiomux_conf_tbl_size = ARRAY_SIZE(apq8064_cam_2d_configs),
	.cam_gpio_common_tbl = apq8064_common_cam_gpio,
	.cam_gpio_common_tbl_size = ARRAY_SIZE(apq8064_common_cam_gpio),
	.cam_gpio_req_tbl = apq8064_back_cam_gpio,
	.cam_gpio_req_tbl_size = ARRAY_SIZE(apq8064_back_cam_gpio),
};

static struct msm_camera_i2c_conf apq8064_back_cam_i2c_conf = {
	.use_i2c_mux = 1,
	.mux_dev = &msm8960_device_i2c_mux_gsbi4,
	.i2c_mux_mode = MODE_L,
};

static struct msm_camera_sensor_flash_data flash_s5k4e5ya = {
	.flash_type	= MSM_CAMERA_FLASH_NONE,
};

static struct msm_camera_csi_lane_params s5k4e5ya_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0x3,
};

static struct msm_camera_sensor_platform_info sensor_board_info_s5k4e5ya = {
	.mount_angle	= 90,
	.cam_vreg = apq_8064_back_cam_vreg,
	.num_vreg = ARRAY_SIZE(apq_8064_back_cam_vreg),
	.gpio_conf = &apq8064_back_cam_gpio_conf,
	.i2c_conf = &apq8064_back_cam_i2c_conf,
	.csi_lane_params = &s5k4e5ya_csi_lane_params,
};


static struct i2c_board_info s5k4e5ya_eeprom_i2c_info = {
	I2C_BOARD_INFO("s5k4e5ya_eeprom", I2C_SLAVE_ADDR_S5K4E5YA_EEPROM),
};

static struct msm_eeprom_info s5k4e5ya_eeprom_info = {
	.board_info     = &s5k4e5ya_eeprom_i2c_info,
	.bus_id         = APQ_8064_GSBI4_QUP_I2C_BUS_ID,
};

#ifdef CONFIG_S5K4E5YA_ACT
static struct i2c_board_info msm_act_main_cam_i2c_info = {
	I2C_BOARD_INFO("msm_actuator", I2C_SLAVE_ADDR_S5K4E5YA_ACT),
};

static struct msm_actuator_info msm_act_main_cam_0_info = {
	.board_info     = &msm_act_main_cam_i2c_info,
	.cam_name       = MSM_ACTUATOR_MAIN_CAM_2,
	.bus_id         = APQ_8064_GSBI4_QUP_I2C_BUS_ID,
	.vcm_pwd        = 0,
	.vcm_enable     = 0,
};
#endif

static struct msm_camera_sensor_info msm_camera_sensor_s5k4e5ya_data = {
	.sensor_name	= "s5k4e5ya",
	.pdata	= &msm_camera_csi_device_data[0],
	.flash_data	= &flash_s5k4e5ya,
	.sensor_platform_info = &sensor_board_info_s5k4e5ya,
	.csi_if	= 1,
	.camera_type = BACK_CAMERA_2D,
	.sensor_type = BAYER_SENSOR,
#ifdef CONFIG_S5K4E5YA_ACT
	.actuator_info = &msm_act_main_cam_0_info,
#endif
	.eeprom_info = &s5k4e5ya_eeprom_info,
};
#endif

#ifdef CONFIG_IMX119
static struct camera_vreg_t apq_8064_front_cam_vreg[] = {
	{"cam2_vdig", REG_VS, 0, 0, 0, 0},
	{"cam2_vio", REG_LDO, 1800000, 1800000, 105000, 0},
	{"cam2_vana", REG_LDO, 2850000, 2850000, 85600, 0},
	{"cam2_i2c", REG_VS, 0, 0, 0, 0},
};

static struct gpio apq8064_front_cam_gpio[] = {
	{GPIO_CAM_MCLK2, GPIOF_DIR_IN, "CAMIF_MCLK"},
};

static struct msm_camera_gpio_conf apq8064_front_cam_gpio_conf = {
	.cam_gpiomux_conf_tbl = apq8064_cam_2d_configs,
	.cam_gpiomux_conf_tbl_size = ARRAY_SIZE(apq8064_cam_2d_configs),
	.cam_gpio_common_tbl = apq8064_common_cam_gpio,
	.cam_gpio_common_tbl_size = ARRAY_SIZE(apq8064_common_cam_gpio),
	.cam_gpio_req_tbl = apq8064_front_cam_gpio,
	.cam_gpio_req_tbl_size = ARRAY_SIZE(apq8064_front_cam_gpio),
};

static struct msm_camera_i2c_conf apq8064_front_cam_i2c_conf = {
	.use_i2c_mux = 1,
	.mux_dev = &msm8960_device_i2c_mux_gsbi4,
	.i2c_mux_mode = MODE_L,
};

static struct msm_camera_sensor_flash_data flash_imx119 = {
	.flash_type	= MSM_CAMERA_FLASH_NONE,
};

static struct msm_camera_csi_lane_params imx119_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0x1,
};

static struct msm_camera_sensor_platform_info sensor_board_info_imx119 = {
	.mount_angle	= 270,
	.cam_vreg = apq_8064_front_cam_vreg,
	.num_vreg = ARRAY_SIZE(apq_8064_front_cam_vreg),
	.gpio_conf = &apq8064_front_cam_gpio_conf,
	.i2c_conf = &apq8064_front_cam_i2c_conf,
	.csi_lane_params = &imx119_csi_lane_params,
};

static struct msm_camera_sensor_info msm_camera_sensor_imx119_data = {
	.sensor_name	= "imx119",
	.pdata	= &msm_camera_csi_device_data[1],
	.flash_data	= &flash_imx119,
	.sensor_platform_info = &sensor_board_info_imx119,
	.csi_if	= 1,
	.camera_type = FRONT_CAMERA_2D,
	.sensor_type = BAYER_SENSOR,
};
#endif

/* Enabling flash LED for camera */
struct led_flash_platform_data {
	unsigned gpio_en;
	unsigned scl_gpio;
	unsigned sda_gpio;
};

static struct led_flash_platform_data lm3559_flash_pdata[] = {
	{
		.scl_gpio = GPIO_CAM_FLASH_I2C_SCL,
		.sda_gpio = GPIO_CAM_FLASH_I2C_SDA,
		.gpio_en = GPIO_CAM_FLASH_EN,
	}
};

static struct platform_device msm_camera_server = {
	.name = "msm_cam_server",
	.id = 0,
};

void __init apq8064_init_cam(void)
{
	msm_gpiomux_install(apq8064_cam_common_configs,
		ARRAY_SIZE(apq8064_cam_common_configs));

	platform_device_register(&msm_camera_server);
	platform_device_register(&msm8960_device_i2c_mux_gsbi4);
	platform_device_register(&msm8960_device_csiphy0);
	platform_device_register(&msm8960_device_csiphy1);
	platform_device_register(&msm8960_device_csid0);
	platform_device_register(&msm8960_device_csid1);
	platform_device_register(&msm8960_device_ispif);
	platform_device_register(&msm8960_device_vfe);
	platform_device_register(&msm8960_device_vpe);
}

#ifdef CONFIG_I2C
static struct i2c_board_info apq8064_camera_i2c_boardinfo[] = {
#ifdef CONFIG_S5K4E5YA
	{
		I2C_BOARD_INFO("s5k4e5ya", I2C_SLAVE_ADDR_S5K4E5YA), /* 0x20 */
		.platform_data = &msm_camera_sensor_s5k4e5ya_data,
	},
#endif
#ifdef CONFIG_IMX119
	{
		I2C_BOARD_INFO("imx119", I2C_SLAVE_ADDR_IMX119), /* 0x6E */
		.platform_data = &msm_camera_sensor_imx119_data,
	},
#endif
};

/* Enabling flash LED for camera */
static struct i2c_board_info apq8064_lge_camera_i2c_boardinfo[] = {
	{
		I2C_BOARD_INFO("lm3559", I2C_SLAVE_ADDR_FLASH),
		.platform_data = &lm3559_flash_pdata,
	},
};

struct msm_camera_board_info apq8064_camera_board_info = {
	.board_info = apq8064_camera_i2c_boardinfo,
	.num_i2c_board_info = ARRAY_SIZE(apq8064_camera_i2c_boardinfo),
};

/* Enabling flash LED for camera */
struct msm_camera_board_info apq8064_lge_camera_board_info = {
	.board_info = apq8064_lge_camera_i2c_boardinfo,
	.num_i2c_board_info = ARRAY_SIZE(apq8064_lge_camera_i2c_boardinfo),
};
#endif /* CONFIG_I2C */
#endif /* CONFIG_MSM_CAMERA */
