/* Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
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

/*
  * add zte lm3642 cci interface led flash driver
  * add  camera led flash engineering mode  interface
  * by ZTE_YCM_20140711 yi.changming 000008
  */
// --->

#include <linux/module.h>
#include "zte_led_flash.h"
#include <mach/gpiomux.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include "../cci/msm_cci.h"


#define CONFIG_MSMB_CAMERA_DEBUG
#undef CDBG
#ifdef CONFIG_MSMB_CAMERA_DEBUG
#define CDBG(fmt, args...) pr_err(fmt, ##args)
#else
#define CDBG(fmt, args...) do { } while (0)
#endif

#define  FLASH_NAME "qcom,camera-led-cci-flash"

static struct msm_led_flash_ctrl_t fctrl_t ;


static struct msm_camera_i2c_fn_t msm_led_trigger_cci_func_tbl = {
	.i2c_read = msm_camera_cci_i2c_read,
	.i2c_read_seq = msm_camera_cci_i2c_read_seq,
	.i2c_write = msm_camera_cci_i2c_write,
	.i2c_write_table = msm_camera_cci_i2c_write_table,
	.i2c_write_seq_table = msm_camera_cci_i2c_write_seq_table,
	.i2c_write_table_w_microdelay =	msm_camera_cci_i2c_write_table_w_microdelay,
	.i2c_util = msm_sensor_cci_i2c_util,
	.i2c_write_conf_tbl = msm_camera_cci_i2c_write_conf_tbl,
};

static struct msm_flash_fn_t msm_led_trigger_func_tbl = {
	.flash_get_subdev_id = msm_led_trigger_get_subdev_id,
	.flash_led_config = msm_led_trigger_config,
	.flash_led_init = msm_led_trigger_init,
	.flash_led_release = msm_led_trigger_release,
	.flash_led_off = msm_led_trigger_off,
	.flash_led_low = msm_led_trigger_low,
	.flash_led_high = msm_led_trigger_high,
};



static int32_t msm_led_trigger_get_dt_vreg_data(struct device_node *of_node,
													struct msm_flash_board_info_t *board_info);
static int32_t msm_led_trigger_get_dt_gpio_req_tbl(struct device_node *of_node,
												struct msm_flash_gpio_conf_t *gconf,
												uint16_t *gpio_array,uint16_t gpio_array_size);
static int32_t msm_led_trigger_init_gpio_pin_tbl(struct device_node *of_node,
												struct msm_flash_gpio_conf_t *gconf,
												 uint16_t *gpio_array,
												uint16_t gpio_array_size);
static int32_t msm_led_trigger_get_dt_data(struct device_node *of_node, struct msm_led_flash_ctrl_t *fctrl);
static int32_t msm_led_trigger_match_id(struct msm_led_flash_ctrl_t *fctrl);




static int32_t msm_led_trigger_get_dt_vreg_data(struct device_node *of_node,
											struct msm_flash_board_info_t *board_info)
{
	struct camera_vreg_t *vreg_conf = NULL;
	uint32_t *vreg_array = NULL;
	uint32_t count = 0;
	int32_t rc = 0, i = 0;

	rc = of_property_read_bool(of_node, "qcom,cam-vreg-name") ;
	if(!rc)
		return 0;
	count = of_property_count_strings(of_node, "qcom,cam-vreg-name");
	CDBG("%s qcom,cam-vreg-name count %d\n", __func__, count);
	if (!count) {
		return -EINVAL;
	}
	board_info->num_vreg = count;

	board_info->vreg_conf = kzalloc(sizeof(struct camera_vreg_t) * count, GFP_KERNEL);
	if (!board_info->vreg_conf) {
		pr_err("%s: failed %d\n", __func__, __LINE__);
		return -ENOMEM;
	}
	vreg_conf = board_info->vreg_conf;

	for (i = 0; i < count; i++) {
		rc = of_property_read_string_index(of_node,	"qcom,cam-vreg-name", i, &vreg_conf[i].reg_name);
		CDBG("%s: reg_name[%d] = %s\n", __func__, i, vreg_conf[i].reg_name);
		if (rc < 0) {
			pr_err("%s: failed %d\n", __func__, __LINE__);
			goto msm_led_trigger_get_dt_vreg_data_failed;
		}
	}

	vreg_array = kzalloc(sizeof(uint32_t) * count, GFP_KERNEL);
	if (!vreg_array) {
		pr_err("%s: failed %d\n", __func__, __LINE__);
		rc = -ENOMEM;
		goto msm_led_trigger_get_dt_vreg_data_failed;
	}


	rc = of_property_read_u32_array(of_node, "qcom,cam-vreg-min-voltage", vreg_array, count);
	if (rc < 0) {
		pr_err("%s: failed %d\n", __func__, __LINE__);
		goto msm_led_trigger_get_dt_vreg_data_failed;
	}
	for (i = 0; i < count; i++) {
		vreg_conf[i].min_voltage = vreg_array[i];
		CDBG("%s: cam_vreg[%d].min_voltage = %d\n", __func__, i, vreg_conf[i].min_voltage);
	}

	rc = of_property_read_u32_array(of_node, "qcom,cam-vreg-max-voltage", vreg_array, count);
	if (rc < 0) {
		pr_err("%s: failed %d\n", __func__, __LINE__);
		goto msm_led_trigger_get_dt_vreg_data_failed;
	}
	for (i = 0; i < count; i++) {
		vreg_conf[i].max_voltage = vreg_array[i];
		CDBG("%s: cam_vreg[%d].max_voltage = %d\n", __func__, i, vreg_conf[i].max_voltage);
	}

	rc = of_property_read_u32_array(of_node, "qcom,cam-vreg-op-mode", vreg_array, count);
	if (rc < 0) {
		pr_err("%s: failed %d\n", __func__, __LINE__);
		goto msm_led_trigger_get_dt_vreg_data_failed;
	}
	for (i = 0; i < count; i++) {
		vreg_conf[i].op_mode = vreg_array[i];
		CDBG("%s: cam_vreg[%d].op_mode = %d\n", __func__, i, vreg_conf[i].op_mode);
	}

	kfree(vreg_array);

	return 0;

msm_led_trigger_get_dt_vreg_data_failed:

	if (vreg_array) {
		kfree(vreg_array);
	}

	if (board_info->vreg_conf) {
		kfree(board_info->vreg_conf);
		board_info->vreg_conf = NULL;
	}

	return rc;
}

static int32_t msm_led_trigger_get_dt_gpio_req_tbl(struct device_node *of_node,
																														 struct msm_flash_gpio_conf_t *gconf,
																														 uint16_t *gpio_array,
																														 uint16_t gpio_array_size)
{
	int32_t rc = 0, i = 0;
	uint32_t count = 0;
	uint32_t *val_array = NULL;

	if (!of_get_property(of_node, "qcom,gpio-req-tbl-num", &count)) {
		return 0;
	}

	count /= sizeof(uint32_t);
	if (!count) {
		pr_err("%s: qcom,gpio-req-tbl-num 0\n", __func__);
		return 0;
	}

	val_array = kzalloc(sizeof(uint32_t) * count, GFP_KERNEL);
	if (!val_array) {
		pr_err("%s: failed %d\n", __func__, __LINE__);
		return -ENOMEM;
	}

	gconf->flash_gpio_req_tbl = kzalloc(sizeof(struct gpio) * count, GFP_KERNEL);
	if (!gconf->flash_gpio_req_tbl) {
		pr_err("%s: failed %d\n", __func__, __LINE__);
		rc = -ENOMEM;
		goto msm_led_trigger_get_dt_gpio_req_tbl_failed;
	}
	gconf->flash_gpio_req_tbl_size = count;

	rc = of_property_read_u32_array(of_node, "qcom,gpio-req-tbl-num",	val_array, count);
	if (rc < 0) {
		pr_err("%s: failed %d\n", __func__, __LINE__);
		goto msm_led_trigger_get_dt_gpio_req_tbl_failed;
	}

	for (i = 0; i < count; i++) {
		if (val_array[i] >= gpio_array_size) {
			pr_err("%s: gpio req tbl index %d invalid\n",	__func__, val_array[i]);
			rc = -EINVAL;
			goto msm_led_trigger_get_dt_gpio_req_tbl_failed;
		}
		gconf->flash_gpio_req_tbl[i].gpio = gpio_array[val_array[i]];
		CDBG("%s: flash_gpio_req_tbl[%d].gpio = %d\n", __func__, i, gconf->flash_gpio_req_tbl[i].gpio);
	}

	rc = of_property_read_u32_array(of_node, "qcom,gpio-req-tbl-flags",	val_array, count);
	if (rc < 0) {
		pr_err("%s: failed %d\n", __func__, __LINE__);
		goto msm_led_trigger_get_dt_gpio_req_tbl_failed;
	}

	for (i = 0; i < count; i++) {
		gconf->flash_gpio_req_tbl[i].flags = val_array[i];
		CDBG("%s: flash_gpio_req_tbl[%d].flags = %ld\n", __func__, i, gconf->flash_gpio_req_tbl[i].flags);
	}

	for (i = 0; i < count; i++) {
		rc = of_property_read_string_index(of_node,	"qcom,gpio-req-tbl-label", i,	&gconf->flash_gpio_req_tbl[i].label);
		CDBG("%s: flash_gpio_req_tbl[%d].label = %s\n", __func__, i,	gconf->flash_gpio_req_tbl[i].label);
		if (rc < 0) {
			pr_err("%s: failed %d\n", __func__, __LINE__);
			goto msm_led_trigger_get_dt_gpio_req_tbl_failed;
		}
	}

	kfree(val_array);

	return 0;

msm_led_trigger_get_dt_gpio_req_tbl_failed:

	if (gconf->flash_gpio_req_tbl) {
		kfree(gconf->flash_gpio_req_tbl);
		gconf->flash_gpio_req_tbl = NULL;
	}
	gconf->flash_gpio_req_tbl_size = 0;

	if (val_array) {
		kfree(val_array);
	}

	return rc;
}

static int32_t msm_led_trigger_init_gpio_pin_tbl(struct device_node *of_node,
																												 struct msm_flash_gpio_conf_t *gconf,
																												 uint16_t *gpio_array,
																												 uint16_t gpio_array_size)
{
	int32_t rc = 0, val = 0;

	gconf->gpio_num_info = kzalloc(sizeof(struct msm_flash_gpio_num_info_t), GFP_KERNEL);
	if (!gconf->gpio_num_info) {
		pr_err("%s: failed %d\n", __func__, __LINE__);
		rc = -ENOMEM;
		return rc;
	}

	if (of_property_read_bool(of_node, "qcom,gpio-enable") == true) {
		rc = of_property_read_u32(of_node, "qcom,gpio-enable", &val);
		if (rc < 0) {
			pr_err("%s: %d read qcom,gpio-enable failed rc %d\n",	__func__, __LINE__, rc);
			goto msm_led_trigger_init_gpio_pin_tbl_failed;
		} else if (val >= gpio_array_size) {
			pr_err("%s: %d qcom,gpio-enable invalid %d\n", __func__, __LINE__, val);
			rc = -EINVAL;
			goto msm_led_trigger_init_gpio_pin_tbl_failed;
		}
		gconf->gpio_num_info->gpio_num[FLASH_GPIO_ENABLE] =	gpio_array[val];
		CDBG("%s qcom,gpio-enable %d\n", __func__, gconf->gpio_num_info->gpio_num[FLASH_GPIO_ENABLE]);
		if (gpio_is_valid(gconf->flash_gpio_req_tbl[FLASH_GPIO_ENABLE].gpio)) {
			rc = gpio_request(gconf->flash_gpio_req_tbl[FLASH_GPIO_ENABLE].gpio, "FLASH_LED_EN");
			if (!rc) {
				gpio_set_value((gconf->flash_gpio_req_tbl[FLASH_GPIO_ENABLE].gpio), 1);
			}
		}
	}

	if (of_property_read_bool(of_node, "qcom,gpio-strobe") == true) {
		rc = of_property_read_u32(of_node, "qcom,gpio-strobe", &val);
		if (rc < 0) {
			pr_err("%s: %d read qcom,gpio-strobe failed rc %d\n",	__func__, __LINE__, rc);
			goto msm_led_trigger_init_gpio_pin_tbl_failed;
		} else if (val >= gpio_array_size) {
			pr_err("%s: %d qcom,gpio-strobe invalid %d\n", __func__, __LINE__, val);
			rc = -EINVAL;
			goto msm_led_trigger_init_gpio_pin_tbl_failed;
		}
		gconf->gpio_num_info->gpio_num[FLASH_GPIO_STROBE] =	gpio_array[val];
		CDBG("%s qcom,gpio-strobe %d\n", __func__, gconf->gpio_num_info->gpio_num[FLASH_GPIO_STROBE]);
	}

	if (of_property_read_bool(of_node, "qcom,gpio-torch") == true) {
		rc = of_property_read_u32(of_node, "qcom,gpio-torch", &val);
		if (rc < 0) {
			pr_err("%s: %d read qcom,gpio-torch failed rc %d\n",	__func__, __LINE__, rc);
			goto msm_led_trigger_init_gpio_pin_tbl_failed;
		} else if (val >= gpio_array_size) {
			pr_err("%s: %d qcom,gpio-torch invalid %d\n", __func__, __LINE__, val);
			rc = -EINVAL;
			goto msm_led_trigger_init_gpio_pin_tbl_failed;
		}
		gconf->gpio_num_info->gpio_num[FLASH_GPIO_TORCH] =	gpio_array[val];
		CDBG("%s qcom,gpio-torch %d\n", __func__, gconf->gpio_num_info->gpio_num[FLASH_GPIO_TORCH]);
	}

	return 0;

msm_led_trigger_init_gpio_pin_tbl_failed:

	if (gconf->gpio_num_info) {
		kfree(gconf->gpio_num_info);
		gconf->gpio_num_info = NULL;
	}

	return rc;
}
static int32_t msm_led_cci_trigger_parse_i2c_cmds(struct device_node *of_node,char *cmd_key,struct msm_camera_i2c_reg_conf **para)
{
	const uint8_t *data;
	struct msm_camera_i2c_reg_conf *table = NULL;
	int blen = 0, len,i = 0;
	
	data = of_get_property(of_node, cmd_key, &blen);
	if(!blen || (blen%2 != 0)){
		pr_err("%s:%sparse failed %d\n", __func__,cmd_key, __LINE__);
		return 0;
	}
	
	len = blen /2;

	table = kzalloc(len * sizeof(struct msm_camera_i2c_reg_conf),
						GFP_KERNEL);
	*para = table;
	while(i < len){
		table[i].reg_addr = data[i*2];
		table[i].reg_data = data[i*2+1];
		pr_err("%s:%x  %x\n",cmd_key,data[i*2],data[i*2+1]);
		i++;	
	}
	return len;
	
		
}
static int32_t msm_led_trigger_get_slave_device_i2c_reg_info(struct device_node *of_node, struct msm_flash_reg_t *preg_setting)
{
	int32_t rc = 0;
	if(of_property_read_bool(of_node, "zte,init_setting") )
		preg_setting->init_setting_size = msm_led_cci_trigger_parse_i2c_cmds(of_node,"zte,init_setting",&(preg_setting->init_setting));

	if(of_property_read_bool(of_node, "zte,low_setting") )
		preg_setting->low_setting_size = msm_led_cci_trigger_parse_i2c_cmds(of_node,"zte,low_setting",&(preg_setting->low_setting));

	if(of_property_read_bool(of_node, "zte,high_setting") )
		preg_setting->high_setting_size = msm_led_cci_trigger_parse_i2c_cmds(of_node,"zte,high_setting",&(preg_setting->high_setting));

	if(of_property_read_bool(of_node, "zte,off_setting") )
		preg_setting->off_setting_size = msm_led_cci_trigger_parse_i2c_cmds(of_node,"zte,off_setting",&(preg_setting->off_setting));

	if(of_property_read_bool(of_node, "zte,release_setting") )
		preg_setting->release_setting_size = msm_led_cci_trigger_parse_i2c_cmds(of_node,"zte,release_setting",&(preg_setting->release_setting));
	if(of_property_read_bool(of_node, "zte,clear_error_flag")){
		rc = of_property_read_u32(of_node, "zte,clear_error_flag", &(preg_setting->clear_error_reg_addr));
		if (rc < 0) {
			preg_setting->clear_error_flag_enable = 0;
			pr_err("%s: failed %d\n", __func__, __LINE__);
		}else
			preg_setting->clear_error_flag_enable = 1;
	}else
		preg_setting->clear_error_flag_enable = 0;
	return 0;
}

 /*
  * solve that led flash device node can't light 
  * 
  * by ZTE_YCM_20140728 yi.changming 000030
  */
// --->
static void msm_led_trigger_get_camera_device(struct device_node *of_node,struct msm_led_flash_ctrl_t *fctrl)
{
	struct device_node *camera_sensor_node = NULL;
	struct platform_device *camera_sensor = NULL;
	
	fctrl->camera_rear_sensor = NULL;
	fctrl->camera_front_sensor = NULL;
 /*
  * fix flash problem in pv system
  * 
  * by ZTE_YCM_20140916 yi.changming 000069
  */
// --->
	fctrl->cci_power_gpio_size = 0;

	fctrl->cci_power_gpio[fctrl->cci_power_gpio_size] = of_get_named_gpio(of_node,
					"zte,cci-power-iovdd-gpios", 0);
	if (fctrl->cci_power_gpio[fctrl->cci_power_gpio_size] < 0) {
		pr_err("%s: can't find zte,cci-power-iovdd-gpios\n", __func__);
	}else{
		if (!gpio_is_valid(fctrl->cci_power_gpio[fctrl->cci_power_gpio_size])) {
			pr_err("%s: Invalid gpio: %d", __func__,fctrl->cci_power_gpio[fctrl->cci_power_gpio_size]);
		}else
			fctrl->cci_power_gpio_size++;
	}

	fctrl->cci_power_gpio[fctrl->cci_power_gpio_size] = of_get_named_gpio(of_node,
					"zte,cci-power-dvdd-gpios", 0);
	if (fctrl->cci_power_gpio[fctrl->cci_power_gpio_size] < 0) {
		pr_err("%s: can't find zte,cci-power-dvdd-gpios\n", __func__);
	}else{
		if (!gpio_is_valid(fctrl->cci_power_gpio[fctrl->cci_power_gpio_size])) {
			pr_err("%s: Invalid gpio: %d", __func__,fctrl->cci_power_gpio[fctrl->cci_power_gpio_size]);
		}else
			fctrl->cci_power_gpio_size++;
	}

	fctrl->cci_power_gpio[fctrl->cci_power_gpio_size] = of_get_named_gpio(of_node,
					"zte,cci-power-avdd-gpios", 0);
	if (fctrl->cci_power_gpio[fctrl->cci_power_gpio_size] < 0) {
		pr_err("%s: can't find zte,cci-power-avdd-gpios\n", __func__);
	}else{
		if (!gpio_is_valid(fctrl->cci_power_gpio[fctrl->cci_power_gpio_size])) {
			pr_err("%s: Invalid gpio: %d", __func__,fctrl->cci_power_gpio[fctrl->cci_power_gpio_size]);
		}else
			fctrl->cci_power_gpio_size++;
	}

	fctrl->cci_power_gpio[fctrl->cci_power_gpio_size] = of_get_named_gpio(of_node,
					"zte,cci-other-power-gpios", 0);
	if (fctrl->cci_power_gpio[fctrl->cci_power_gpio_size] < 0) {
		pr_err("%s: can't find zte,cci-other-power-gpios\n", __func__);
	}else{
		if (!gpio_is_valid(fctrl->cci_power_gpio[fctrl->cci_power_gpio_size])) {
			pr_err("%s: Invalid gpio: %d", __func__,fctrl->cci_power_gpio[fctrl->cci_power_gpio_size]);
		}else
			fctrl->cci_power_gpio_size++;
	}
// <---000069
	camera_sensor_node = of_parse_phandle(of_node, "zte,camera-sensor-rear-src", 0);
	if (!camera_sensor_node) {
		pr_err("%s: can't find rear sensor phandle\n", __func__);
	}else{
		camera_sensor = of_find_device_by_node(camera_sensor_node);
		if (!camera_sensor) {
			pr_err("%s:%d: can't find the device by node\n", __func__,__LINE__);
		}else{
			fctrl->camera_rear_sensor = camera_sensor;
		}
	}
	
	camera_sensor_node = NULL;
	camera_sensor = NULL;
	
	camera_sensor_node = of_parse_phandle(of_node, "zte,camera-sensor-front-src", 0);
	if (!camera_sensor_node) {
		pr_err("%s: can't find front sensor phandle\n", __func__);
	}else{
		camera_sensor = of_find_device_by_node(camera_sensor_node);
		if (!camera_sensor) {
			pr_err("%s:%d: can't find the device by node\n", __func__,__LINE__);
		}else{
			fctrl->camera_front_sensor = camera_sensor;
		}
	}
	
}
// <---	

static int32_t msm_led_trigger_get_dt_data(struct device_node *of_node, struct msm_led_flash_ctrl_t *fctrl)
{
	struct msm_flash_gpio_conf_t *gconf = NULL;
	uint16_t *gpio_array = NULL;
	uint16_t gpio_array_size = 0;
	uint32_t id_info[3] = {0};
	int32_t rc = 0, i = 0;

	rc = of_property_read_u32(of_node, "cell-index", &fctrl->pdev->id);
	CDBG("%s: pdev id %d\n", __func__, fctrl->pdev->id);
	if (rc < 0) {
		pr_err("%s: failed %d\n", __func__, __LINE__);
		return -EINVAL;
	}

	rc = of_property_read_u32(of_node, "qcom,cci-master",	&fctrl->cci_i2c_master);
	CDBG("%s: qcom,cci-master %d, rc %d\n", __func__, fctrl->cci_i2c_master, rc);
	if (rc < 0) {
		fctrl->cci_i2c_master = MASTER_0;
		rc = 0;
	}

	rc = msm_led_trigger_get_dt_vreg_data(of_node, &fctrl->board_info);
	if (rc < 0) {
		pr_err("%s: failed %d\n", __func__, __LINE__);
	}

	if(of_property_read_bool(of_node, "gpios") ){
		gpio_array_size = of_gpio_count(of_node);
		CDBG("%s: gpio count %d\n", __func__, gpio_array_size);
	}
	if (gpio_array_size) {

		fctrl->board_info.gpio_conf = kzalloc(sizeof(struct msm_flash_gpio_conf_t), GFP_KERNEL);
		if (!fctrl->board_info.gpio_conf) {
			pr_err("%s: failed %d\n", __func__, __LINE__);
			rc = -ENOMEM;
			goto msm_led_trigger_get_dt_data_failed;
		}
		gconf = fctrl->board_info.gpio_conf;
	
		gpio_array = kzalloc(sizeof(uint16_t) * gpio_array_size, GFP_KERNEL);
		if (!gpio_array) {
			pr_err("%s: failed %d\n", __func__, __LINE__);
			rc = -ENOMEM;
			goto msm_led_trigger_get_dt_data_failed;
		}

		for (i = 0; i < gpio_array_size; i++) {
			gpio_array[i] = of_get_gpio(of_node, i);
			CDBG("%s: gpio_array[%d] = %d\n", __func__, i, gpio_array[i]);
		}

		rc = msm_led_trigger_get_dt_gpio_req_tbl(of_node, gconf, gpio_array, gpio_array_size);
		if (rc < 0) {
			pr_err("%s: failed %d\n", __func__, __LINE__);
			goto msm_led_trigger_get_dt_data_failed;
		}

		rc = msm_led_trigger_init_gpio_pin_tbl(of_node, gconf, gpio_array, gpio_array_size);
		if (rc < 0) {
			pr_err("%s: failed %d\n", __func__, __LINE__);
			goto msm_led_trigger_get_dt_data_failed;
		}
	}

	fctrl->board_info.slave_info = kzalloc(sizeof(struct msm_flash_slave_info_t),	GFP_KERNEL);
	if (!fctrl->board_info.slave_info) {
		pr_err("%s: failed %d\n", __func__, __LINE__);
		rc = -ENOMEM;
		goto msm_led_trigger_get_dt_data_failed;
	}

	rc = of_property_read_u32_array(of_node, "qcom,slave-id",	id_info, 3);
	if (rc < 0) {
		pr_err("%s: failed %d\n", __func__, __LINE__);
		goto msm_led_trigger_get_dt_data_failed;
	}

	fctrl->board_info.slave_info->flash_slave_addr = id_info[0];
	fctrl->board_info.slave_info->flash_id_reg_addr = id_info[1];
	fctrl->board_info.slave_info->flash_id = id_info[2];

	rc = of_property_read_string(of_node, "qcom,flash-name", &fctrl->board_info.flash_name);
	if (rc < 0) {
		pr_err("%s: failed %d\n", __func__, __LINE__);
		goto msm_led_trigger_get_dt_data_failed;
	}

	rc = msm_led_trigger_get_slave_device_i2c_reg_info(of_node, fctrl->reg_setting);
	if (rc < 0) {
		pr_err("%s: failed %d\n", __func__, __LINE__);
	}

	kfree(gpio_array);

	return 0;

msm_led_trigger_get_dt_data_failed:

	if (fctrl->board_info.slave_info) {
		kfree(fctrl->board_info.slave_info);
		fctrl->board_info.slave_info = NULL;
	}

	if (fctrl->board_info.gpio_conf->gpio_num_info) {
		kfree(fctrl->board_info.gpio_conf->gpio_num_info);
		fctrl->board_info.gpio_conf->gpio_num_info = NULL;
	}

	if (fctrl->board_info.gpio_conf->flash_gpio_req_tbl) {
		kfree(fctrl->board_info.gpio_conf->flash_gpio_req_tbl);
		fctrl->board_info.gpio_conf->flash_gpio_req_tbl = NULL;
	}

	if (gpio_array) {
		kfree(gpio_array);
	}

	if (fctrl->board_info.gpio_conf) {
		kfree(fctrl->board_info.gpio_conf);
		fctrl->board_info.gpio_conf = NULL;
	}

	if (fctrl->board_info.vreg_conf) {
		kfree(fctrl->board_info.vreg_conf);
		fctrl->board_info.vreg_conf = NULL;
	}

	return rc;
}

static int32_t __attribute__((unused)) msm_led_trigger_match_id(struct msm_led_flash_ctrl_t *fctrl)
{
	uint16_t chipid = 0;
	int32_t rc = 0;

	rc = fctrl->flash_i2c_client->i2c_func_tbl->i2c_read(fctrl->flash_i2c_client,
							fctrl->board_info.slave_info->flash_id_reg_addr,
							&chipid, fctrl->reg_setting->default_data_type);
	if (rc < 0) {
		pr_err("%s: read id failed\n", __func__);
		return rc;
	}

	pr_info("%s: read id: %x expected id %x\n", __func__, chipid, fctrl->board_info.slave_info->flash_id);

	if (chipid != fctrl->board_info.slave_info->flash_id) {
		pr_err("msm_led_trigger_match_id chip id doesnot match\n");
		return -ENODEV;
	}

	return rc;
}

int32_t msm_led_trigger_cci_register(struct msm_led_flash_ctrl_t *fctrl)
{
	int32_t rc;

	if (fctrl->current_state == ZTE_LED_SHUTDOWN) {
		return 0;
	}

	
	if(fctrl->cci_reg > 0){
		fctrl->cci_reg++;
		return 0;
	}

	rc = fctrl->flash_i2c_client->i2c_func_tbl->i2c_util(fctrl->flash_i2c_client, MSM_CCI_INIT);
	if (rc < 0) {
		pr_err("%s: cci_init failed\n", __func__);
		return -EINVAL;
	}
	fctrl->cci_reg++;

	return 0;
}

int32_t msm_led_trigger_cci_unregister(struct msm_led_flash_ctrl_t *fctrl)
{
	int32_t rc;

	if (fctrl->current_state == ZTE_LED_SHUTDOWN || !fctrl->cci_reg) {
		return 0;
	}


	if(fctrl->cci_reg > 1){
		fctrl->cci_reg--;
		return 0;
	}

	rc = fctrl->flash_i2c_client->i2c_func_tbl->i2c_util(fctrl->flash_i2c_client, MSM_CCI_RELEASE);
	if (rc < 0) {
		pr_err("%s: cci_release failed\n", __func__);
		return -EINVAL;
	}
	fctrl->cci_reg--;

	return 0;
}

int32_t msm_led_trigger_get_subdev_id(struct msm_led_flash_ctrl_t *fctrl,	void *arg)
{
	uint32_t *subdev_id = (uint32_t *)arg;
	if (!subdev_id) {
		pr_err("%s: %d failed\n", __func__, __LINE__);
		return -EINVAL;
	}
	*subdev_id = fctrl->pdev->id;
	CDBG("%s: %d subdev_id %d\n", __func__, __LINE__, *subdev_id);
	return 0;
}

int32_t msm_led_trigger_config(struct msm_led_flash_ctrl_t *fctrl,	void *data)
{
	int32_t rc = 0;
	struct msm_camera_led_cfg_t *cfg = (struct msm_camera_led_cfg_t *)data;

	CDBG("%s: led_state %d\n", __func__, cfg->cfgtype);

	if (!fctrl || !fctrl->func_tbl) {
		pr_err("%s: failed\n", __func__);
		return -EINVAL;
	}

	switch (cfg->cfgtype) {
	case MSM_CAMERA_LED_INIT:
		rc = msm_led_trigger_cci_register(fctrl);
		if (rc < 0) break;
		rc = fctrl->func_tbl->flash_led_init(fctrl);
		break;

	case MSM_CAMERA_LED_LOW:
		rc = fctrl->func_tbl->flash_led_low(fctrl);
		break;

	case MSM_CAMERA_LED_HIGH:
		rc = fctrl->func_tbl->flash_led_high(fctrl);
		break;

	case MSM_CAMERA_LED_OFF:
		rc = fctrl->func_tbl->flash_led_off(fctrl);
		break;

	case MSM_CAMERA_LED_RELEASE:
		rc = fctrl->func_tbl->flash_led_release(fctrl);
		if (rc < 0) break;
		rc = msm_led_trigger_cci_unregister(fctrl);
		break;

	default:
		rc = -EFAULT;
		break;
	}

	CDBG("%s: rc %d\n", __func__, rc);

	return rc;
}

static int32_t led_trigger_cci_write_table(struct msm_led_flash_ctrl_t *fctrl,struct msm_camera_i2c_reg_conf *table,	 int32_t num)
{
	int32_t i = 0, rc = 0;

	for (i = 0; i < num; ++i) {
		rc = fctrl->flash_i2c_client->i2c_func_tbl->i2c_write(fctrl->flash_i2c_client,table->reg_addr,table->reg_data,fctrl->reg_setting->default_data_type);
		if (rc < 0) {
			break;
		}

		table++;
	}

	return rc;
}

static int32_t led_trigger_cci_read_error_flag_reg(struct msm_led_flash_ctrl_t *fctrl,uint32_t reg_addr)
{
	int32_t  rc = 0;
	uint16_t flags = 0;
	rc = fctrl->flash_i2c_client->i2c_func_tbl->i2c_read(fctrl->flash_i2c_client,reg_addr,&flags,fctrl->reg_setting->default_data_type);
	if (rc < 0) {
		pr_err("%s: read flags error \n", __func__);
		return rc;
	}
	CDBG("%s:  read reg[0x%x] = 0x%x\n", __func__,reg_addr,flags);
	return 0;
}

int32_t msm_led_trigger_init(struct msm_led_flash_ctrl_t *fctrl)
{
	int32_t rc = 0;
	
	CDBG("%s: E   len = %d\n", __func__,fctrl->reg_setting->init_setting_size);
	if (fctrl->current_state == ZTE_LED_SHUTDOWN || fctrl->current_state == ZTE_LED_INIT
		||!fctrl->cci_reg) {
		return 0;
	}
		
	if(!(fctrl->reg_setting->init_setting_size))
		return 0;

	if(fctrl->reg_setting->clear_error_flag_enable){
		if(led_trigger_cci_read_error_flag_reg(fctrl,fctrl->reg_setting->clear_error_reg_addr))
			return  rc;
	}
	
	rc = led_trigger_cci_write_table(fctrl, fctrl->reg_setting->init_setting, fctrl->reg_setting->init_setting_size);
	if (rc < 0) {
		pr_err("%s: i2c write table failed\n", __func__);
		return rc;
	}
	fctrl->current_state = ZTE_LED_INIT;
	CDBG("%s: X\n", __func__);

	return 0;
}

int32_t msm_led_trigger_low(struct msm_led_flash_ctrl_t *fctrl)
{
	int32_t rc = 0;
	
	CDBG("%s: E   len = %d\n", __func__,fctrl->reg_setting->low_setting_size);
	
	if (fctrl->current_state == ZTE_LED_SHUTDOWN || fctrl->current_state == ZTE_LED_LOW
		||!fctrl->cci_reg) {
		return 0;
	}
	if(!(fctrl->reg_setting->low_setting_size))
		return 0;
		
	rc = led_trigger_cci_write_table(fctrl, fctrl->reg_setting->low_setting, fctrl->reg_setting->low_setting_size);
	if (rc < 0) {
		pr_err("%s: i2c write table failed\n", __func__);
		return rc;
	}
	fctrl->current_state = ZTE_LED_LOW;
	CDBG("%s: X\n", __func__);

	return 0;
}

int32_t msm_led_trigger_high(struct msm_led_flash_ctrl_t *fctrl)
{
	int32_t rc = 0;
	
	CDBG("%s: E   len = %d\n", __func__,fctrl->reg_setting->high_setting_size);

	if (fctrl->current_state == ZTE_LED_SHUTDOWN || !fctrl->cci_reg) {
		return 0;
	}
	
	if(!(fctrl->reg_setting->high_setting_size))
		return 0;
	
		
	rc = led_trigger_cci_write_table(fctrl, fctrl->reg_setting->high_setting, fctrl->reg_setting->high_setting_size);
	if (rc < 0) {
		pr_err("%s: i2c write table failed\n", __func__);
		return rc;
	}
	
	fctrl->current_state = ZTE_LED_HIGH;
	
	CDBG("%s: X\n", __func__);

	return 0;
}

int32_t msm_led_trigger_off(struct msm_led_flash_ctrl_t *fctrl)
{
	int32_t rc = 0;
	
	CDBG("%s: E   len = %d\n", __func__,fctrl->reg_setting->off_setting_size);
	
	if (fctrl->current_state == ZTE_LED_SHUTDOWN || fctrl->current_state == ZTE_LED_OFF
		||!fctrl->cci_reg) {
		return 0;
	}
	
	if(!(fctrl->reg_setting->off_setting_size))
		return 0;

		
	rc = led_trigger_cci_write_table(fctrl, fctrl->reg_setting->off_setting, fctrl->reg_setting->off_setting_size);
	if (rc < 0) {
		pr_err("%s: i2c write table failed\n", __func__);
		return rc;
	}
	
	fctrl->current_state = ZTE_LED_OFF;
	
	CDBG("%s: X\n", __func__);

	return 0;
}

int32_t msm_led_trigger_release(struct msm_led_flash_ctrl_t *fctrl)
{
	int32_t rc = 0;
	
	CDBG("%s: E   len = %d\n", __func__,fctrl->reg_setting->release_setting_size);
	
	if (fctrl->current_state == ZTE_LED_SHUTDOWN || fctrl->current_state == ZTE_LED_RELEASE
		||!fctrl->cci_reg) {
		return 0;
	}
	if(fctrl->reg_setting->clear_error_flag_enable){
		if(led_trigger_cci_read_error_flag_reg(fctrl,fctrl->reg_setting->clear_error_reg_addr))
			return  rc;
	}
	
	if(!(fctrl->reg_setting->release_setting_size))
		return 0;
		
	rc = led_trigger_cci_write_table(fctrl, fctrl->reg_setting->release_setting, fctrl->reg_setting->release_setting_size);
	if (rc < 0) {
		pr_err("%s: i2c write table failed\n", __func__);
		return rc;
	}
	
	fctrl->current_state = ZTE_LED_RELEASE;
	
	CDBG("%s: X\n", __func__);

	return 0;
}

int32_t msm_led_trigger_free_data(struct msm_led_flash_ctrl_t *fctrl)
{
	if (!fctrl->pdev) {
		return 0;
	}

	if (fctrl->board_info.gpio_conf->gpio_num_info) {
		kfree(fctrl->board_info.gpio_conf->gpio_num_info);
		fctrl->board_info.gpio_conf->gpio_num_info = NULL;
	}

	if (fctrl->board_info.gpio_conf->flash_gpio_req_tbl) {
		kfree(fctrl->board_info.gpio_conf->flash_gpio_req_tbl);
		fctrl->board_info.gpio_conf->flash_gpio_req_tbl = NULL;
	}

	if (fctrl->board_info.gpio_conf) {
		kfree(fctrl->board_info.gpio_conf);
		fctrl->board_info.gpio_conf = NULL;
	}

	if (fctrl->board_info.vreg_conf) {
		kfree(fctrl->board_info.vreg_conf);
		fctrl->board_info.vreg_conf = NULL;
	}

	if (fctrl->board_info.slave_info) {
		kfree(fctrl->board_info.slave_info);
		fctrl->board_info.slave_info = NULL;
	}

	(void)msm_led_trigger_cci_unregister(fctrl);

	if (fctrl->flash_i2c_client->cci_client) {
		kfree(fctrl->flash_i2c_client->cci_client);
		fctrl->flash_i2c_client->cci_client = NULL;
	}

	return 0;
}


static void zte_flash_shutdown(struct platform_device *pdev)
{
	struct msm_led_flash_ctrl_t *pfctrl = &fctrl_t;
	int cci_power;
	pr_err("%s: E\n", __func__);

	if(pfctrl->current_state >= ZTE_LED_RELEASE  && !pfctrl->cci_reg)
		return;
 /*
  * fix flash shutdown cci power problem
  * 
  * by ZTE_YCM_20140916 yi.changming 000070
  */
// --->
	cci_power = cci_power_is_enable(pfctrl);
	if(!cci_power && pfctrl->cci_power_gpio_size > 0){
		CDBG("%s:%d:sensor is closed ,need to pull power\n", __func__, __LINE__);
		cci_sensor_power(pfctrl,1);
	}
// <---000070
	(void)msm_led_trigger_cci_register(pfctrl);

	pfctrl->func_tbl->flash_led_release(pfctrl);

	while(pfctrl->cci_reg)
		(void)msm_led_trigger_cci_unregister(pfctrl);
 /*
  * fix flash shutdown cci power problem
  * 
  * by ZTE_YCM_20140916 yi.changming 000070
  */
// --->
	if(!cci_power && pfctrl->cci_power_gpio_size > 0){
		cci_sensor_power(pfctrl,0);
	}
// <---000070
	pfctrl->current_state = ZTE_LED_SHUTDOWN;

	pr_err("%s: X \n", __func__);
}


int32_t msm_led_cci_trigger_probe(struct platform_device *pdev)
{
	struct device_node *of_node = pdev->dev.of_node;
	struct msm_led_flash_ctrl_t *pfctrl = &fctrl_t;
	struct msm_camera_cci_client *cci_client = NULL;
	int32_t rc = 0;

	if (!of_node) {
		pr_err("%s: of_node NULL\n", __func__);
		return -EINVAL;
	}
	
	pfctrl->pdev = pdev;


	rc = msm_led_trigger_get_dt_data(of_node, pfctrl);
	if (rc < 0) {
		pr_err("%s: failed line %d\n", __func__, __LINE__);
		return rc;
	}
 /*
  * solve that led flash device node can't light 
  * 
  * by ZTE_YCM_20140728 yi.changming 000030
  */
// --->
	msm_led_trigger_get_camera_device(of_node, pfctrl);
// <---	

	pfctrl->flash_i2c_client->i2c_func_tbl = &msm_led_trigger_cci_func_tbl;
	
	pfctrl->flash_i2c_client->cci_client = kzalloc(sizeof(struct msm_camera_cci_client), GFP_KERNEL);
	if (!pfctrl->flash_i2c_client->cci_client) {
		pr_err("%s: failed line %d\n", __func__, __LINE__);
		return rc;
	}

	pfctrl->flash_device_type = MSM_CAMERA_PLATFORM_DEVICE;
	pfctrl->cci_reg = 0;

	cci_client = pfctrl->flash_i2c_client->cci_client;
	cci_client->cci_subdev = msm_cci_get_subdev();
	cci_client->cci_i2c_master = pfctrl->cci_i2c_master;
	cci_client->sid =	pfctrl->board_info.slave_info->flash_slave_addr >> 1;
	cci_client->retries = 3;
	cci_client->id_map = 0;
	cci_client->i2c_freq_mode = I2C_FAST_MODE;


	rc = msm_led_flash_create_v4lsubdev(pdev, pfctrl);
	if (rc < 0) {
		pr_err("%s: failed line %d\n", __func__, __LINE__);
		goto msm_led_trigger_probe_failed;
	}

	(void)msm_led_trigger_register_sysdev(pfctrl);

  	pfctrl->current_state = ZTE_LED_RELEASE;


	CDBG("%s %s probe succeeded\n", __func__, pfctrl->board_info.flash_name);

	return 0;

msm_led_trigger_probe_failed:

	if (!cci_client) {
		kfree(cci_client);
		cci_client = NULL;
	}

	return rc;
}
static const struct of_device_id msm_led_trigger_dt_match[] = {
	{.compatible = "qcom,camera-led-cci-flash"},
	{}
};

MODULE_DEVICE_TABLE(of, msm_led_trigger_dt_match);

static struct platform_driver msm_led_trigger_driver = {
	.driver = {
		.name = FLASH_NAME,
		.owner = THIS_MODULE,
		.of_match_table = msm_led_trigger_dt_match,
	},
	.shutdown = zte_flash_shutdown,
};
static int __init msm_led_cci_trigger_add_driver(void)
{
	CDBG("called\n");
	return platform_driver_probe(&msm_led_trigger_driver,
		msm_led_cci_trigger_probe);
}
static struct msm_flash_reg_t reg_setting = {
	.default_data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.init_setting = NULL,
	.init_setting_size = 0,
	.low_setting = NULL,
	.low_setting_size = 0,
	.high_setting = NULL,
	.high_setting_size = 0,
	.off_setting = NULL,
	.off_setting_size = 0,
	.release_setting = NULL,
	.release_setting_size = 0,
};

static struct msm_camera_i2c_client i2c_client = {
	.addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
};
static struct msm_led_flash_ctrl_t fctrl_t = {
	.flash_i2c_client = &i2c_client,
	.func_tbl = &msm_led_trigger_func_tbl,
	.reg_setting = &reg_setting,
};

module_init(msm_led_cci_trigger_add_driver);
MODULE_DESCRIPTION("LED TRIGGER FLASH");
MODULE_LICENSE("GPL v2");
// <---

