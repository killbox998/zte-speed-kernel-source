/* Copyright (c) 2013-2014, The Linux Foundation. All rights reserved.
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
#include <linux/device.h>
#include "zte_led_flash.h"
 /*
  * solve that led flash device node can't light 
  * 
  * by ZTE_YCM_20140728 yi.changming 000030
  */
// --->
#include <linux/gpio.h>
#include "msm_sensor.h"
// <---

#define CONFIG_MSMB_CAMERA_DEBUG
#undef CDBG
#ifdef CONFIG_MSMB_CAMERA_DEBUG
#define CDBG(fmt, args...) pr_err(fmt, ##args)
#else
#define CDBG(fmt, args...) do { } while (0)
#endif



typedef struct {
	const char *name;
	struct msm_led_flash_ctrl_t *fctrl;
} camera_led_flash_sysdev_info_t;

static camera_led_flash_sysdev_info_t flash_info;

static struct bus_type camera_led_flash_subsys = {
	.name = "led-flash",
	.dev_name = "led-flash",
};

static struct device device_camera_led_flash;

 /*
  * solve that led flash device node can't light 
  * 
  * by ZTE_YCM_20140728 yi.changming 000030
  */
// --->
static int sensor_power_is_enable(struct msm_sensor_ctrl_t *s_ctrl)
{
	struct msm_camera_power_ctrl_t *power_info = NULL;
	int enable = 0;
	
	if(s_ctrl &&s_ctrl->sensordata){
		power_info = &s_ctrl->sensordata->power_info;
		if(power_info)
			 enable = power_info->cam_pinctrl_status;
	}

	return enable;
}
int cci_power_is_enable(struct msm_led_flash_ctrl_t *fctrl)
{
	struct msm_sensor_ctrl_t *s_ctrl  = NULL;
	int enable = 0;
	
	if(fctrl->camera_rear_sensor){
		s_ctrl = platform_get_drvdata(fctrl->camera_rear_sensor);
		enable = sensor_power_is_enable(s_ctrl);
		if(enable)
			return enable;
	}

	if(fctrl->camera_front_sensor){
		s_ctrl = platform_get_drvdata(fctrl->camera_front_sensor);
		enable = sensor_power_is_enable(s_ctrl);
		if(enable)
			return enable;
	}

	return enable;
}
// <---

 /*
  * fix flash problem in pv system
  * 
  * by ZTE_YCM_20140916 yi.changming 000069
  */
// --->
void cci_sensor_power(struct msm_led_flash_ctrl_t *fctrl,int enable)
{
	int i ;
	for(i = 0; i < fctrl->cci_power_gpio_size; i++){
		if(enable){
		gpio_request_one( fctrl->cci_power_gpio[i],0, NULL);
		gpio_set_value_cansleep( fctrl->cci_power_gpio[i],1);
		}else{
		gpio_set_value_cansleep( fctrl->cci_power_gpio[i],0);
		gpio_free( fctrl->cci_power_gpio[i]);
		}
	}
}
// <---000069

static ssize_t set_torch(struct device *dev, struct device_attribute *attr, const char *buf,size_t buf_sz)
{

	int32_t enable = 0;
	struct msm_led_flash_ctrl_t *fctrl = flash_info.fctrl;
 /*
  * solve that led flash device node can't light 
  * 
  * by ZTE_YCM_20140728 yi.changming 000030
  */
// --->
	int cci_power;
// <---
	CDBG("%s:%d E\n", __func__, __LINE__);
		
   	if(fctrl->current_state == ZTE_LED_SHUTDOWN){
		CDBG("%s:%d X  LED STATUS IS SHUTDOWN\n", __func__, __LINE__);
      		return buf_sz;
   	}

	sscanf(buf, "%d", &enable);

	if((enable && fctrl->current_state == ZTE_LED_LOW) 
		||(!enable && fctrl->current_state != ZTE_LED_LOW)){
		
		CDBG("%s:%d X  LED STATUS IS NOT CHANGE\n", __func__, __LINE__);
      		return buf_sz;
   	}
 /*
  * solve that led flash device node can't light 
  * 
  * by ZTE_YCM_20140728 yi.changming 000030
  */
// --->
	cci_power = cci_power_is_enable(fctrl);
// <---	
	if(!cci_power && fctrl->cci_power_gpio_size > 0){
		CDBG("%s:%d:sensor is closed ,need to pull power\n", __func__, __LINE__);
		cci_sensor_power(fctrl,1);
	}
// <---	
	msm_led_trigger_cci_register(fctrl);
	
	if (enable) {		
		fctrl->func_tbl->flash_led_init(fctrl);
		fctrl->func_tbl->flash_led_low(fctrl);
	} else {
		fctrl->func_tbl->flash_led_off(fctrl);
		fctrl->func_tbl->flash_led_release(fctrl);
	}
	
	msm_led_trigger_cci_unregister(fctrl);
 /*
  * solve that led flash device node can't light 
  * 
  * by ZTE_YCM_20140728 yi.changming 000030
  */
// --->
	if(!cci_power && fctrl->cci_power_gpio_size > 0){
		cci_sensor_power(fctrl,0);
	}
// <---
	CDBG("%s:%d X\n", __func__, __LINE__);
	
	return buf_sz;
}

static DEVICE_ATTR(torch, S_IWUSR , NULL, set_torch);

static ssize_t set_flash(struct device *dev, struct device_attribute *attr, const char *buf,size_t buf_sz)
{
	int32_t enable = 0;
	struct msm_led_flash_ctrl_t *fctrl = flash_info.fctrl;
 /*
  * solve that led flash device node can't light 
  * 
  * by ZTE_YCM_20140728 yi.changming 000030
  */
// --->
	int cci_power;
// <---
	CDBG("%s:%d E\n", __func__, __LINE__);
		
   	if(fctrl->current_state == ZTE_LED_SHUTDOWN){
		CDBG("%s:%d X  LED STATUS IS SHUTDOWN\n", __func__, __LINE__);
      		return buf_sz;
   	}

	sscanf(buf, "%d", &enable);
 /*
  * solve that led flash device node can't light 
  * 
  * by ZTE_YCM_20140728 yi.changming 000030
  */
// --->
	cci_power = cci_power_is_enable(fctrl);
	
	if(!cci_power && fctrl->cci_power_gpio_size > 0){
		CDBG("%s:%d:sensor is closed ,need to pull power\n", __func__, __LINE__);
		cci_sensor_power(fctrl,1);
	}
// <---	
	msm_led_trigger_cci_register(fctrl);
	
	if (enable) {		
		fctrl->func_tbl->flash_led_init(fctrl);
		fctrl->func_tbl->flash_led_high(fctrl);
	} else {
		fctrl->func_tbl->flash_led_off(fctrl);
		fctrl->func_tbl->flash_led_release(fctrl);
	}
	
	msm_led_trigger_cci_unregister(fctrl);
 /*
  * solve that led flash device node can't light 
  * 
  * by ZTE_YCM_20140728 yi.changming 000030
  */
// --->
	if(!cci_power && fctrl->cci_power_gpio_size > 0){
		cci_sensor_power(fctrl,0);
	}
// <---		
	CDBG("%s:%d X\n", __func__, __LINE__);
	
	return buf_sz;
}

static DEVICE_ATTR(flash, S_IWUSR , NULL, set_flash);

static ssize_t name_read(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;

	CDBG("%s: E\n", __func__);

	ret = snprintf(buf,PAGE_SIZE,"%s\n",flash_info.name);

	CDBG("%s: X\n", __func__);

	return ret;
}
static DEVICE_ATTR(name, S_IRUGO , name_read, NULL);



const struct device_attribute *camera_led_flash_dev_attrs[] = {
	&dev_attr_name,
	&dev_attr_torch,
	&dev_attr_flash,
};
/*
 * MSM LED Trigger Sys Device Register
 *
 * 1. Torch Mode
 *     enable: $ echo "1" > /sys/devices/system/led-flash/led-flash0/torch
 *    disable: $ echo "0" > /sys/devices/system/led-flash/led-flash0/torch
 *
 * 2. Flash Mode
 *     enable: $ echo "1" > /sys/devices/system/led-flash/led-flash0/flash
 *    disable: $ echo "0" > /sys/devices/system/led-flash/led-flash0/flash
 */
int32_t msm_led_trigger_register_sysdev(struct msm_led_flash_ctrl_t *fctrl)
{
	int32_t i, rc;

	rc = subsys_system_register(&camera_led_flash_subsys, NULL);
	if (rc) {
			return rc;
	}

	flash_info.name = fctrl->board_info.flash_name;
	flash_info.fctrl =  fctrl;

	
	device_camera_led_flash.id = 0;
	device_camera_led_flash.bus =  &camera_led_flash_subsys;


	rc = device_register(&device_camera_led_flash);
	if (rc) {
		return rc;
	}

	for (i = 0; i < ARRAY_SIZE(camera_led_flash_dev_attrs); ++i) {
		rc = device_create_file(&device_camera_led_flash,
					camera_led_flash_dev_attrs[i]);
		if (rc) {
			goto register_sysdev_error;
		}
	}

	return rc;

register_sysdev_error:

	while (--i >= 0) device_remove_file(&device_camera_led_flash, camera_led_flash_dev_attrs[i]);

	device_unregister(&device_camera_led_flash);

	return rc;

}
// <---
