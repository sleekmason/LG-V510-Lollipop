/*
 *  I2C backlight device driver
 *
 *  Copyright (C) 2013, LG Eletronics,Inc. All rights reservced.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/backlight.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <mach/board.h>
#include <linux/i2c.h>
#include <linux/i2c_bl.h>
#include <linux/workqueue.h>
#include <linux/ctype.h>

#include <mach/board_lge.h>
#if defined(CONFIG_HAS_EARLYSUSPEND)
#include <linux/earlysuspend.h>
#endif

#define I2C_BL_NAME             "i2c_bl"
#define MAX_BRIGHTNESS_I2C_BL   0xFF
#define MIN_BRIGHTNESS_I2C_BL   0x0F
#define DEFAULT_BRIGHTNESS      0xFF
#define DEFAULT_FTM_BRIGHTNESS  0x0F

#define BL_ON        1
#define BL_OFF       0

struct i2c_bl_device {
	struct i2c_client *client;
	struct backlight_device *bl_dev;
	int gpio;
	int min_brightness;
	int max_brightness;
	int default_brightness;
	int factory_brightness;
	struct mutex bl_mutex;

#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif /* CONFIG_HAS_EARLYSUSPEND */

	int store_level_used;
	int delay_for_shaking;
	int cur_main_lcd_level;
	int saved_main_lcd_level;
	int backlight_status;
	int exp_min_value;
	int cal_value;

	int percentage;
	int percentage_current;
	unsigned long duration_step;
};

static struct i2c_bl_device *main_dev;

static const struct i2c_device_id i2c_bl_id[] = {
	{ I2C_BL_NAME, 0 },
	{ },
};

static void update_level_scale(struct work_struct *work);

static int i2c_bl_read_reg(struct i2c_client *client, u8 reg, u8 *buf);
static int i2c_bl_write_reg(struct i2c_client *client,
		unsigned char reg, unsigned char val);
static int i2c_bl_write_regs(struct i2c_client *client,
		struct i2c_bl_cmd *bl_cmds, int size);
static int i2c_bl_read_regs(struct i2c_client *client,
		struct i2c_bl_cmd *bl_cmds, int size);
static int i2c_bl_set_regs(struct i2c_client *client,
		struct i2c_bl_cmd *bl_cmds, int size, unsigned char value);

static void i2c_bl_lcd_backlight_set_level(struct i2c_client *client,
		int level);

void i2c_bl_lcd_backlight_set_level_export(int level)
{
	if (main_dev != NULL &&
		main_dev->client != NULL) {
		i2c_bl_lcd_backlight_set_level(main_dev->client, level);
	} else {
		pr_err("%s(): No client\n", __func__);
	}
}
EXPORT_SYMBOL(i2c_bl_lcd_backlight_set_level_export);

static DECLARE_DELAYED_WORK(update_level_scale_work, update_level_scale);

static void update_level_scale(struct work_struct *work)
{
	int percentage = main_dev->percentage;
	int percentage_current = main_dev->percentage_current;

	if (percentage > percentage_current)
		percentage_current++;
	else if	(percentage < percentage_current)
		percentage_current--;
	else
		return;

	main_dev->percentage_current = percentage_current;
	i2c_bl_lcd_backlight_set_level_export(main_dev->cur_main_lcd_level);

	if (percentage != percentage_current)
		schedule_delayed_work(&update_level_scale_work,
			msecs_to_jiffies(main_dev->duration_step));
}

void i2c_bl_lcd_backlight_set_level_scale(int percentage,
		unsigned long duration)
{
	if (main_dev != NULL && main_dev->client != NULL) {
		main_dev->percentage = percentage;

		if (duration <= 0) {
			main_dev->percentage_current = percentage;
			main_dev->duration_step = 0;
			i2c_bl_lcd_backlight_set_level_export(
					main_dev->cur_main_lcd_level);
		} else {
			int percentage_current = main_dev->percentage_current;

			if (delayed_work_pending(&update_level_scale_work))
				cancel_delayed_work(&update_level_scale_work);

			if (percentage < percentage_current)
				main_dev->duration_step = duration /
					(percentage_current - percentage);
			else if (percentage > percentage_current)
				main_dev->duration_step = duration /
					(percentage - percentage_current);
			else
				main_dev->duration_step = 0;

			if (main_dev->duration_step > 0)
				schedule_delayed_work(&update_level_scale_work, 0);
		}
	}
}
EXPORT_SYMBOL(i2c_bl_lcd_backlight_set_level_scale);

static void i2c_bl_hw_reset(struct i2c_client *client)
{
	struct i2c_bl_platform_data *pdata = client->dev.platform_data;

	int gpio = pdata->gpio;

	if (gpio_is_valid(gpio)) {
		gpio_direction_output(gpio, 1);
		gpio_set_value_cansleep(gpio, 1);
		mdelay(10);
	} else {
		pr_err("%s: gpio is not valid !!\n", __func__);
	}
}

static int i2c_bl_read_reg(struct i2c_client *client, u8 reg, u8 *buf)
{
    s32 ret;

    pr_debug("reg: %x\n", reg);

    ret = i2c_smbus_read_byte_data(client, reg);
    if (ret < 0)
           pr_err("i2c_smbus_read_byte_data error\n");

    *buf = ret;

    return ret;
}

static int i2c_bl_write_reg(struct i2c_client *client,
		unsigned char reg, unsigned char val)
{
	int err;
	u8 buf[2];
	struct i2c_msg msg = {
		client->addr, 0, 2, buf
	};

	buf[0] = reg;
	buf[1] = val;

	err = i2c_transfer(client->adapter, &msg, 1);
	if (err < 0)
		dev_err(&client->dev, "i2c write error\n");

	return err;
}

static int i2c_bl_write_regs(struct i2c_client *client,
		struct i2c_bl_cmd *bl_cmds, int size)
{
	if (bl_cmds != NULL && size > 0) {
		while (size--) {
			unsigned char addr, ovalue, value, mask;

			addr = bl_cmds->addr;
			value = bl_cmds->value;
			mask = bl_cmds->mask;

			bl_cmds++;

			if (mask == 0)
				continue;

			if (mask == 0xff)
				i2c_bl_write_reg(client, addr, value);
			else {
				i2c_bl_read_reg(client, addr, &ovalue);
				i2c_bl_write_reg(client, addr,
					(ovalue & (~mask)) | (value & mask));
			}
		}
	}

	return 0;
}

static int i2c_bl_read_regs(struct i2c_client *client,
		struct i2c_bl_cmd *bl_cmds, int size)
{
	if (bl_cmds != NULL && size > 0) {
		while (size--) {
			i2c_bl_read_reg(client, bl_cmds->addr, &bl_cmds->value);
			bl_cmds++;
		}
	}

	return 0;
}


static int i2c_bl_set_regs(struct i2c_client *client,
		struct i2c_bl_cmd *bl_cmds, int size, unsigned char value)
{
	if (bl_cmds != NULL && size > 0) {
		while (size--) {
			unsigned char addr, ovalue, mask;

			addr = bl_cmds->addr;
			mask = bl_cmds->mask;

			bl_cmds++;

			if (mask == 0)
				continue;

			if (mask == 0xff)
				i2c_bl_write_reg(client, addr, value);
			else {
				i2c_bl_read_reg(client, addr, &ovalue);
				i2c_bl_write_reg(client, addr,
					(ovalue & (~mask)) | (value & mask));
			}
		}
	}

	return 0;
}

static void i2c_bl_set_main_current_level(struct i2c_client *client, int level)
{
	struct i2c_bl_device *dev = i2c_get_clientdata(client);
	struct i2c_bl_platform_data *pdata = client->dev.platform_data;
	int cal_value;

	if ((pdata->factory_mode) && level)
	{
		level = pdata->factory_brightness;
	}

	if (level == -1)
		level = dev->default_brightness;

	mutex_lock(&dev->bl_mutex);

	dev->cur_main_lcd_level = level;
	dev->bl_dev->props.brightness = dev->cur_main_lcd_level;

	dev->store_level_used = 0;
	if (level != 0) {
		if (pdata->blmap != NULL) {
			if (level < pdata->blmap_size)
				cal_value = pdata->blmap[level];
			else {
				pr_err("Out of blmap range, wanted=%d,"
					"limit=%d\n", level, pdata->blmap_size);
				cal_value = level;
			}
		} else
			cal_value = level;

		if (dev->percentage_current != 100) {
			cal_value = (cal_value * dev->percentage_current) / 100;
		}

		dev->cal_value = cal_value;

		i2c_bl_set_regs(client, pdata->set_brightness_cmds,
			pdata->set_brightness_cmds_size, dev->cal_value);
	} else {
		i2c_bl_write_regs(client, pdata->deinit_cmds,
			pdata->deinit_cmds_size);
		dev->backlight_status = BL_OFF;
	}

	mutex_unlock(&dev->bl_mutex);

	pr_debug("backlight level=%d, cal_value=%d\n", level, dev->cal_value);
}

static void i2c_bl_set_main_current_level_no_mapping(struct i2c_client *client,
		int level)
{
	struct i2c_bl_device *dev = i2c_get_clientdata(client);
	struct i2c_bl_platform_data *pdata = client->dev.platform_data;

	if (level > 255)
		level = 255;
	else if (level < 0)
		level = 0;

	dev->cur_main_lcd_level = level;
	dev->bl_dev->props.brightness = dev->cur_main_lcd_level;

	dev->store_level_used = 1;

	mutex_lock(&dev->bl_mutex);
	if (level != 0) {
		i2c_bl_set_regs(client, pdata->set_brightness_cmds,
			pdata->set_brightness_cmds_size, level);
	} else {
		i2c_bl_write_regs(client, pdata->deinit_cmds,
			pdata->deinit_cmds_size);
		dev->backlight_status = BL_OFF;
	}
	mutex_unlock(&dev->bl_mutex);
}

void i2c_bl_backlight_on(struct i2c_client *client, int level)
{
	struct i2c_bl_device *dev = i2c_get_clientdata(client);
	struct i2c_bl_platform_data *pdata = client->dev.platform_data;

	if (dev->backlight_status == BL_OFF) {
		i2c_bl_hw_reset(client);
		mutex_lock(&dev->bl_mutex);
		i2c_bl_write_regs(client, pdata->init_cmds,
				pdata->init_cmds_size);
		mutex_unlock(&dev->bl_mutex);
	}

	mdelay(1);

	i2c_bl_set_main_current_level(client, level);
	dev->backlight_status = BL_ON;

	return;
}

static void i2c_bl_backlight_off(struct i2c_client *client)
{
	struct i2c_bl_device *dev = i2c_get_clientdata(client);
	struct i2c_bl_platform_data *pdata = client->dev.platform_data;
	int gpio = pdata->gpio;

	if (dev->backlight_status == BL_OFF)
		return;

	dev->saved_main_lcd_level = dev->cur_main_lcd_level;
	i2c_bl_set_main_current_level(client, 0);
	dev->backlight_status = BL_OFF;

	gpio_direction_output(gpio, 0);
	msleep(6);

	return;
}

static void i2c_bl_lcd_backlight_set_level(struct i2c_client *client, int level)
{
	if (level > MAX_BRIGHTNESS_I2C_BL)
		level = MAX_BRIGHTNESS_I2C_BL;

	if (client != NULL) {
		if (level == 0) {
			i2c_bl_backlight_off(client);
		} else {
			i2c_bl_backlight_on(client, level);
		}
	} else {
		pr_err("%s(): No client\n", __func__);
	}
}

#ifdef CONFIG_HAS_EARLYSUSPEND
void i2c_bl_early_suspend(struct early_suspend * h)
{
	struct i2c_bl_device *dev = container_of(h, struct i2c_bl_device,
			early_suspend);

	pr_info("%s: backlight_status: %d\n",
			__func__, dev->backlight_status);
	if (dev->backlight_status == BL_OFF)
		return;

	i2c_bl_lcd_backlight_set_level(dev->client, 0);
	return;
}

void i2c_bl_late_resume(struct early_suspend * h)
{
	struct i2c_bl_device *dev = container_of(h, struct i2c_bl_device,
			early_suspend);

	pr_info("%s: backlight_status: %d\n",
			__func__, dev->backlight_status);
	if (dev->backlight_status == BL_ON)
		return;

	if (lge_get_boot_mode() == LGE_BOOT_MODE_CHARGER)
		i2c_bl_lcd_backlight_set_level(dev->client, dev->saved_main_lcd_level);

	return;
}
#endif /* CONFIG_HAS_EARLYSUSPEND */

static int bl_set_intensity(struct backlight_device *bd)
{
	struct i2c_client *client = to_i2c_client(bd->dev.parent);
	struct i2c_bl_device *dev = i2c_get_clientdata(client);

	if (dev->backlight_status == BL_ON)
		i2c_bl_set_main_current_level(client, bd->props.brightness);

	return 0;
}

static int bl_get_intensity(struct backlight_device *bd)
{
	struct i2c_client *client = to_i2c_client(bd->dev.parent);
	struct i2c_bl_device *dev = i2c_get_clientdata(client);
	struct i2c_bl_platform_data *pdata = client->dev.platform_data;

	unsigned char val = 0;

	mutex_lock(&dev->bl_mutex);
	i2c_bl_read_regs(client, pdata->get_brightness_cmds,
			pdata->get_brightness_cmds_size);
	mutex_unlock(&dev->bl_mutex);

	val = pdata->get_brightness_cmds[0].value &
			pdata->get_brightness_cmds[0].mask;

	return (int)val;
}

static ssize_t lcd_backlight_show_level(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int r = 0;
	struct i2c_client *client = to_i2c_client(dev);
	struct i2c_bl_device *i2c_dev = i2c_get_clientdata(client);

	if (i2c_dev->store_level_used == 0)
		r = snprintf(buf, PAGE_SIZE, "LCD Backlight Level is : %d\n",
				i2c_dev->cal_value);
	else if (i2c_dev->store_level_used == 1)
		r = snprintf(buf, PAGE_SIZE, "LCD Backlight Level is : %d\n",
				i2c_dev->cur_main_lcd_level);

	return r;
}

static ssize_t lcd_backlight_store_level(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int level;
	struct i2c_client *client = to_i2c_client(dev);

	if (!count)
		return -EINVAL;

	level = simple_strtoul(buf, NULL, 10);

	i2c_bl_set_main_current_level_no_mapping(client, level);
	pr_debug("write %d direct to backlight register\n", level);

	return count;
}

static int i2c_bl_resume(struct i2c_client *client)
{
	struct i2c_bl_device *dev = i2c_get_clientdata(client);

	i2c_bl_lcd_backlight_set_level(client, dev->saved_main_lcd_level);
	return 0;
}

static int i2c_bl_suspend(struct i2c_client *client, pm_message_t state)
{
#if !defined(CONFIG_HAS_EARLYSUSPEND)
	struct i2c_bl_device *dev = i2c_get_clientdata(client);
#endif

	pr_debug("%s: new state: %d\n", __func__, state.event);

#if !defined(CONFIG_HAS_EARLYSUSPEND)
	i2c_bl_lcd_backlight_set_level(client, dev->saved_main_lcd_level);
#else
	i2c_bl_backlight_off(client);
#endif
	return 0;
}

static ssize_t lcd_backlight_show_on_off(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct i2c_bl_device *i2c_dev = i2c_get_clientdata(client);
	int r = 0;

	pr_info("%s received (prev backlight_status: %s)\n",
			__func__, i2c_dev->backlight_status ? "ON" : "OFF");

	return r;
}

static ssize_t lcd_backlight_store_on_off(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int on_off;
	struct i2c_client *client = to_i2c_client(dev);
	struct i2c_bl_device *i2c_dev = i2c_get_clientdata(client);

	if (!count)
		return -EINVAL;

	pr_info("%s received (prev backlight_status: %s)\n",
			__func__, i2c_dev->backlight_status ? "ON" : "OFF");

	on_off = simple_strtoul(buf, NULL, 10);

	pr_debug("%s: on_off=%d", __func__, on_off);

	if (on_off == 1) {
		i2c_bl_resume(client);
	} else if (on_off == 0)
	    i2c_bl_suspend(client, PMSG_SUSPEND);

	return count;

}
static ssize_t lcd_backlight_show_exp_min_value(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct i2c_bl_device *i2c_dev = i2c_get_clientdata(client);
	int r;

	r = snprintf(buf, PAGE_SIZE, "LCD Backlight  : %d\n",
			i2c_dev->exp_min_value);

	return r;
}

static ssize_t lcd_backlight_store_exp_min_value(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct i2c_bl_device *i2c_dev = i2c_get_clientdata(client);
	int value;

	if (!count)
		return -EINVAL;

	value = simple_strtoul(buf, NULL, 10);
	i2c_dev->exp_min_value = value;

	return count;
}

static ssize_t lcd_backlight_show_shaking_delay(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct i2c_bl_device *i2c_dev = i2c_get_clientdata(client);
	int r = 0;

	pr_debug("%s received (shaking delay : %d)\n",
			__func__, i2c_dev->delay_for_shaking);

	return r;
}

static ssize_t lcd_backlight_store_shaking_delay(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct i2c_bl_device *i2c_dev = i2c_get_clientdata(client);
	int delay;

	if (!count)
		return -EINVAL;

	delay = simple_strtoul(buf, NULL, 10);
	pr_debug("%s received (you input : %d for shaking delay)\n",
			__func__, delay);
	i2c_dev->delay_for_shaking = delay;

	return count;

}

static ssize_t lcd_backlight_show_dump_reg(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct i2c_bl_platform_data *pdata = client->dev.platform_data;
	struct i2c_bl_cmd *dump_regs = pdata->dump_regs;
	struct i2c_bl_device *i2c_dev = i2c_get_clientdata(client);

	int dump_regs_size = pdata->dump_regs_size;

	int r;
	int ret = 0;

	if (i2c_dev->backlight_status == BL_OFF) {
		ret = snprintf(buf, PAGE_SIZE, "I2C BL are power down!!\n");
		return ret;
	}

	mutex_lock(&i2c_dev->bl_mutex);

	while (dump_regs_size--) {
		i2c_bl_read_reg(client, dump_regs->addr, &dump_regs->value);
		mdelay(3);
		r = snprintf(buf+ret, PAGE_SIZE-ret, "%02X: %02X, %s\n",
			(unsigned int)dump_regs->addr,
			(unsigned int)dump_regs->value, dump_regs->description);
		if (r > 0)
			ret += r;
		else
			break;
		dump_regs++;
	}
	mutex_unlock(&i2c_dev->bl_mutex);

	return ret;
}

static ssize_t lcd_backlight_store_dump_reg(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct i2c_bl_device *i2c_dev = i2c_get_clientdata(client);

	unsigned int addr, value, mask;
	unsigned char ovalue;

	if (i2c_dev->backlight_status == BL_OFF)
		return 0;

	sscanf(buf, "%x %x %x", &addr, &value, &mask);

	if (mask == 0)
		return count;

	mutex_lock(&i2c_dev->bl_mutex);
	if (mask == 0xff)
		i2c_bl_write_reg(client, addr, value);
	else {
		i2c_bl_read_reg(client, addr, &ovalue);
		i2c_bl_write_reg(client, addr,
				(ovalue & (~mask)) | (value & mask));
	}
	mutex_lock(&i2c_dev->bl_mutex);

	return count;
}

static ssize_t lcd_backlight_show_blmap(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct i2c_bl_platform_data *pdata = client->dev.platform_data;
	int i;
	int j;

	if (!pdata->blmap_size)
		return 0;

	buf[0] = '{';

	for (i = 0, j = 2; i < pdata->blmap_size && j < PAGE_SIZE; ++i) {
		if (!(i % 15)) {
			buf[j] = '\n';
			++j;
		}

		sprintf(&buf[j], "%d, ", pdata->blmap[i]);
		if (pdata->blmap[i] < 10)
			j += 3;
		else if (pdata->blmap[i] < 100)
			j += 4;
		else
			j += 5;
	}

	buf[j] = '\n';
	++j;
	buf[j] = '}';
	++j;

	return j;
}

static ssize_t lcd_backlight_store_blmap(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct i2c_bl_platform_data *pdata = client->dev.platform_data;
	static char *blmap;
	static char *old_blmap;
	int i;
	int j;
	int value;

	if (count < 1)
		return count;

	if (buf[0] != '{')
		return -EINVAL;

	old_blmap = blmap;

	blmap = kmalloc(256, GFP_KERNEL);
	for (i = 1, j = 0; i < count && j < 256; ++i) {
		if (!isdigit(buf[i]))
			continue;

		sscanf(&buf[i], "%d", &value);
		blmap[j] = (char)value;

		while (isdigit(buf[i]))
			++i;
		++j;
	}

	pdata->blmap = blmap;
	pdata->blmap_size = j;

	kfree(old_blmap);

	return count;
}

DEVICE_ATTR(i2c_bl_level, 0644, lcd_backlight_show_level,
		lcd_backlight_store_level);
DEVICE_ATTR(i2c_bl_backlight_on_off, 0644, lcd_backlight_show_on_off,
		lcd_backlight_store_on_off);
DEVICE_ATTR(i2c_bl_exp_min_value, 0644, lcd_backlight_show_exp_min_value,
		lcd_backlight_store_exp_min_value);
DEVICE_ATTR(i2c_bl_shaking_delay, 0644, lcd_backlight_show_shaking_delay,
		lcd_backlight_store_shaking_delay);
DEVICE_ATTR(i2c_bl_dump_reg, 0644, lcd_backlight_show_dump_reg,
		lcd_backlight_store_dump_reg);
DEVICE_ATTR(i2c_bl_blmap, 0644, lcd_backlight_show_blmap,
		lcd_backlight_store_blmap);

static struct backlight_ops i2c_bl_ops = {
	.update_status = bl_set_intensity,
	.get_brightness = bl_get_intensity,
};

static int i2c_bl_probe(struct i2c_client *i2c_dev,
		const struct i2c_device_id *id)
{
	struct i2c_bl_platform_data *pdata;
	struct i2c_bl_device *dev;
	struct backlight_device *bl_dev;
	struct backlight_properties props;
	int err;

	pr_info("%s: start\n", __func__);

	pdata = i2c_dev->dev.platform_data;

	dev = kzalloc(sizeof(struct i2c_bl_device), GFP_KERNEL);
	if (dev == NULL) {
		dev_err(&i2c_dev->dev, "fail alloc for i2c_bl_device\n");
		return 0;
	}

	pr_debug("%s: gpio = %d\n", __func__, pdata->gpio);

	if (pdata->gpio && gpio_request(pdata->gpio, "i2c_bl reset") != 0) {
		return -ENODEV;
	}

	main_dev = dev;

	memset(&props, 0, sizeof(struct backlight_properties));
	props.type = BACKLIGHT_RAW;

	props.max_brightness = MAX_BRIGHTNESS_I2C_BL;
	bl_dev = backlight_device_register(I2C_BL_NAME, &i2c_dev->dev, NULL,
			&i2c_bl_ops, &props);
	bl_dev->props.max_brightness = MAX_BRIGHTNESS_I2C_BL;
	bl_dev->props.brightness = DEFAULT_BRIGHTNESS;
	bl_dev->props.power = FB_BLANK_UNBLANK;

	dev->bl_dev = bl_dev;
	dev->client = i2c_dev;
	dev->gpio = pdata->gpio;
	dev->min_brightness = pdata->min_brightness;
	dev->default_brightness = pdata->default_brightness;
	dev->max_brightness = pdata->max_brightness;
	dev->store_level_used = 0;
	dev->delay_for_shaking = 50;
	dev->cur_main_lcd_level = DEFAULT_BRIGHTNESS;
	dev->saved_main_lcd_level = DEFAULT_BRIGHTNESS;
	dev->backlight_status = BL_ON;
	dev->exp_min_value = 150;
	dev->percentage = 100;
	dev->percentage_current = 100;
	dev->duration_step = 0;
	i2c_set_clientdata(i2c_dev, dev);

	pdata->factory_mode = 0;

	mutex_init(&dev->bl_mutex);

	err = device_create_file(&i2c_dev->dev, &dev_attr_i2c_bl_level);
	err = device_create_file(&i2c_dev->dev,
			&dev_attr_i2c_bl_backlight_on_off);
	err = device_create_file(&i2c_dev->dev, &dev_attr_i2c_bl_exp_min_value);
	err = device_create_file(&i2c_dev->dev, &dev_attr_i2c_bl_shaking_delay);
	err = device_create_file(&i2c_dev->dev, &dev_attr_i2c_bl_dump_reg);
	err = device_create_file(&i2c_dev->dev, &dev_attr_i2c_bl_blmap);

#ifdef CONFIG_HAS_EARLYSUSPEND
	dev->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
	dev->early_suspend.suspend = i2c_bl_early_suspend;
	dev->early_suspend.resume = i2c_bl_late_resume;
	register_early_suspend(&dev->early_suspend);
#endif /* CONFIG_HAS_EARLYSUSPEND */
	return 0;
}

static int i2c_bl_remove(struct i2c_client *client)
{
	struct i2c_bl_device *dev = i2c_get_clientdata(client);
	int gpio;

	device_remove_file(&client->dev, &dev_attr_i2c_bl_level);
	device_remove_file(&client->dev, &dev_attr_i2c_bl_backlight_on_off);
	device_remove_file(&client->dev, &dev_attr_i2c_bl_exp_min_value);
	device_remove_file(&client->dev, &dev_attr_i2c_bl_shaking_delay);
	device_remove_file(&client->dev, &dev_attr_i2c_bl_dump_reg);

	gpio = dev->gpio;

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&dev->early_suspend);
#endif /* CONFIG_HAS_EARLYSUSPEND */

	backlight_device_unregister(dev->bl_dev);
	i2c_set_clientdata(client, NULL);

	if (gpio_is_valid(gpio))
		gpio_free(gpio);

	return 0;
}

static struct i2c_driver main_i2c_bl_driver = {
	.probe = i2c_bl_probe,
	.remove = i2c_bl_remove,
	.suspend = NULL,
	.resume = NULL,
	.id_table = i2c_bl_id,
	.driver = {
		.name = I2C_BL_NAME,
		.owner = THIS_MODULE,
	},
};

static int __init lcd_backlight_init(void)
{
	static int err;

	err = i2c_add_driver(&main_i2c_bl_driver);

	return err;
}

module_init(lcd_backlight_init);

MODULE_DESCRIPTION("I2C_BL Backlight Control");
MODULE_AUTHOR("Gilbert Ahn");
MODULE_LICENSE("GPL");
