/* Copyright (c) 2009-2013, The Linux Foundation. All rights reserved.
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
#ifndef MSM_LED_FLASH_H
#define MSM_LED_FLASH_H

#include <linux/leds.h>
#include <linux/platform_device.h>
#include <media/v4l2-subdev.h>
#include <media/msm_cam_sensor.h>
#include <soc/qcom/camera2.h>
#include "msm_camera_i2c.h"
#include "msm_sd.h"


#define MAX_LED_TRIGGERS 3

struct msm_led_flash_ctrl_t;

enum msm_flash_seq_gpio_t {
	FLASH_GPIO_ENABLE,
	FLASH_GPIO_STROBE,
	FLASH_GPIO_TORCH,
	FLASH_GPIO_MAX,
};

struct msm_flash_slave_info_t {
	uint16_t flash_slave_addr;
	uint16_t flash_id_reg_addr;
	uint16_t flash_id;
};

struct msm_flash_gpio_num_info_t {
	uint16_t gpio_num[FLASH_GPIO_MAX];
};

struct msm_flash_gpio_conf_t {
	struct gpio *flash_gpio_req_tbl;
	uint8_t flash_gpio_req_tbl_size;
	struct msm_flash_gpio_num_info_t *gpio_num_info;
};

struct msm_flash_board_info_t {
	const char *flash_name;
	struct msm_flash_slave_info_t *slave_info;
	struct camera_vreg_t *vreg_conf;
	int32_t num_vreg;
	struct msm_flash_gpio_conf_t *gpio_conf;
};

enum msm_camera_flash_led_status_t {
	ZTE_LED_OFF,
	ZTE_LED_LOW,
	ZTE_LED_HIGH,
	ZTE_LED_INIT,
	ZTE_LED_RELEASE,
	ZTE_LED_SHUTDOWN,
};

struct msm_flash_reg_t {
	enum msm_camera_i2c_data_type default_data_type;
	struct msm_camera_i2c_reg_conf *init_setting;
	uint8_t init_setting_size;
	struct msm_camera_i2c_reg_conf *low_setting;
	uint8_t low_setting_size;
	struct msm_camera_i2c_reg_conf *high_setting;
	uint8_t high_setting_size;
	struct msm_camera_i2c_reg_conf *off_setting;
	uint8_t off_setting_size;
	struct msm_camera_i2c_reg_conf *release_setting;
	uint8_t release_setting_size;
	uint8_t clear_error_flag_enable;
	uint32_t clear_error_reg_addr;
	
};

struct msm_flash_fn_t {
	int32_t (*flash_get_subdev_id)(struct msm_led_flash_ctrl_t *, void *);
	int32_t (*flash_led_config)(struct msm_led_flash_ctrl_t *, void *);
	int32_t (*flash_led_init)(struct msm_led_flash_ctrl_t *);
	int32_t (*flash_led_release)(struct msm_led_flash_ctrl_t *);
	int32_t (*flash_led_off)(struct msm_led_flash_ctrl_t *);
	int32_t (*flash_led_low)(struct msm_led_flash_ctrl_t *);
	int32_t (*flash_led_high)(struct msm_led_flash_ctrl_t *);
};

struct msm_led_flash_ctrl_t {
	struct msm_camera_i2c_client *flash_i2c_client;
	struct msm_sd_subdev msm_sd;
	struct platform_device *pdev;
	struct msm_flash_fn_t *func_tbl;
	const char *flash_trigger_name[MAX_LED_TRIGGERS];
	struct led_trigger *flash_trigger[MAX_LED_TRIGGERS];
	uint32_t flash_op_current[MAX_LED_TRIGGERS];
	const char *torch_trigger_name;
	struct led_trigger *torch_trigger;
	uint32_t torch_op_current;
	void *data;
	uint32_t num_sources;
	enum msm_camera_flash_led_status_t current_state;
	uint32_t cci_reg;
	struct msm_flash_board_info_t board_info;
	enum cci_i2c_master_t cci_i2c_master;
	enum msm_camera_device_type_t flash_device_type;
	struct msm_flash_reg_t *reg_setting;
 /*
  * solve that led flash device node can't light 
  * 
  * by ZTE_YCM_20140728 yi.changming 000030
  */
// --->
	struct platform_device *camera_rear_sensor;
	struct platform_device *camera_front_sensor;
	int cci_power_gpio[6];
	int cci_power_gpio_size;
// <---	
};

int32_t msm_led_trigger_get_subdev_id(struct msm_led_flash_ctrl_t *fctrl, void *arg);
int32_t msm_led_trigger_config(struct msm_led_flash_ctrl_t *fctrl, void *data);
int32_t msm_led_trigger_init(struct msm_led_flash_ctrl_t *fctrl);
int32_t msm_led_trigger_release(struct msm_led_flash_ctrl_t *fctrl);
int32_t msm_led_trigger_off(struct msm_led_flash_ctrl_t *fctrl);
int32_t msm_led_trigger_low(struct msm_led_flash_ctrl_t *fctrl);
int32_t msm_led_trigger_high(struct msm_led_flash_ctrl_t *fctrl);
int32_t msm_led_trigger_free_data(struct msm_led_flash_ctrl_t *fctrl);
int32_t msm_led_trigger_probe(struct platform_device *pdev, void *data);

int32_t msm_led_flash_create_v4lsubdev(struct platform_device *pdev,
	void *data);

int32_t msm_led_trigger_cci_register(struct msm_led_flash_ctrl_t *fctrl);/*init cci*/
int32_t msm_led_trigger_cci_unregister(struct msm_led_flash_ctrl_t *fctrl);/*release cci*/

int32_t msm_led_trigger_register_sysdev(struct msm_led_flash_ctrl_t *fctrl);
 /*
  * fix flash shutdown cci power problem
  * 
  * by ZTE_YCM_20140916 yi.changming 000070
  */
// --->
int cci_power_is_enable(struct msm_led_flash_ctrl_t *fctrl);
void cci_sensor_power(struct msm_led_flash_ctrl_t *fctrl,int enable);
// <---000070
#endif
// <---