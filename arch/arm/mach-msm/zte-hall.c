/*
 * Driver for some zte_hall functions
 */
#include <linux/module.h> 
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/input/pmic8xxx-pwrkey.h>
#include <linux/gpio.h>
#include <linux/suspend.h>
#include <linux/platform_device.h>
#include <linux/mfd/pm8xxx/pm8921.h>
#include <linux/timer.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/wakelock.h>
#include <linux/of_gpio.h>

#include "zte-hall.h"

#define	ENOMEM		12	/* Out of memory */
suspend_state_t new_state_backup= PM_SUSPEND_ON;/////liukejing;
static int factory_mode = 0;
int hall_is_factory_mode = 0;
int hall_current_factory_mode = 0;
struct hall_pwrkey *hallpwrkey;
struct wake_lock hall_wake_lock;
#define HALL_WAKELOCK_TIMEOUT 14
//static struct zte_hall_platform_data hall_pdata;
static int hall_mesc = 120;
static void hall_timer_func(unsigned long);
static DEFINE_TIMER(hall_timer, hall_timer_func, 0, 0);
enum {
	DEBUG_HALL_STATE = 1U << 1,

};
static char *hall_dev_name[] = {
	"hall-wakelock",
};

#define HALL_EN 		"hall_en"
static int gpio_hall_en = 0;
struct zte_gpio_info {
	int sys_num;			//system pin number
	const char *name;
};
#define MAX_SUPPORT_GPIOS 16
struct zte_gpio_info zte_hall_gpios[MAX_SUPPORT_GPIOS];
static struct of_device_id zte_hall_of_match[] = {
	{ .compatible = "zte-hall", },
	{ },
};
MODULE_DEVICE_TABLE(of, zte_hall_of_match);

int get_hall_sysnumber_byname(char* name)
{
	int i;
	for (i = 0; i < MAX_SUPPORT_GPIOS; i++) {
		if (zte_hall_gpios[i].name) {
			if (!strcmp(zte_hall_gpios[i].name,name)) 
				return zte_hall_gpios[i].sys_num;	
		}
	}
	return -1;
}
unsigned int hallstate=(unsigned int)HALL_STATE_NULL;
module_param_named(hall_timer_debug, hall_mesc, int, S_IRUGO | S_IWUSR | S_IWGRP);


static int factory_mode_set(const char *val, struct kernel_param *kp)
{
	int ret;

	ret = param_set_int(val, kp);

	if (ret)
		return ret;
	
	if ((factory_mode & 0x02) == 0)
		hall_current_factory_mode = 0;
	else
		hall_current_factory_mode = 1;

	return 0;
}

static int factory_mode_get(char *buffer, struct kernel_param *kp)
{				
	switch(hallstate)
	{
		case HALL_STATE_OPEN:
			hall_is_factory_mode = 1;
			break;
		case HALL_STATE_CLOSE:
			hall_is_factory_mode = 0;
			break;
		default:
			hall_is_factory_mode = 0;
		break;
	}

	return	sprintf(buffer,"%d",hall_is_factory_mode);
}
module_param_call(factory_mode, factory_mode_set, factory_mode_get,
			&factory_mode, 0644);
static void hall_timer_func (unsigned long _pwrkey)
{
		bool open;
		open = gpio_get_value(gpio_hall_en);////0:close;1:open
	    wake_unlock(&hall_wake_lock);
			if(open)
				{
				if(hall_mesc&DEBUG_HALL_STATE) 
					{
						pr_info("hall_detect_high_interrupt\n");
					}
				if(!hall_current_factory_mode)
					{
						if(hall_mesc&DEBUG_HALL_STATE) 
						{
							pr_info("hall_detect_high return reason:%d\n",new_state_backup);
						}   
						if(new_state_backup == PM_SUSPEND_MEM)////lcd off
						{			
							input_report_key(hallpwrkey->hall_pwr, KEY_POWER, 1);
							input_sync(hallpwrkey->hall_pwr);
							input_report_key(hallpwrkey->hall_pwr, KEY_POWER, 0);
							input_sync(hallpwrkey->hall_pwr);
							
							pr_info("hall_detect_high_interrupt:hall open\n");
	
						}
					}
					hallstate = HALL_STATE_OPEN;
				}
			else
			{
				if(hall_mesc&DEBUG_HALL_STATE) 
					{
						pr_info("hall_detect_low_interrupt return reason:%d\n",new_state_backup);
						pr_info("hall_detect_low_interrupt\n");
					}  
			 	
				if(!hall_current_factory_mode)
				{
						if(new_state_backup == PM_SUSPEND_ON)////lcd on
						{
	
							input_report_key(hallpwrkey->hall_pwr, KEY_POWER, 1);
							input_sync(hallpwrkey->hall_pwr);
							input_report_key(hallpwrkey->hall_pwr, KEY_POWER, 0);
							input_sync(hallpwrkey->hall_pwr);
							
							pr_info("hall_detect_low_interrupt:hall close\n");
	
						  }
				}
				hallstate = HALL_STATE_CLOSE;
				
			}

}

static irqreturn_t hall_detect_interrupt(int irq, void *_pwrkey)////open
{
	mod_timer(&hall_timer, jiffies + ((unsigned long)hall_mesc/10));
	wake_lock_timeout(&hall_wake_lock, HALL_WAKELOCK_TIMEOUT);

	if(hall_mesc&DEBUG_HALL_STATE) 
		{
			pr_info("hall timer start in hall_detect_interrupt\n");
		}
	return IRQ_HANDLED;
}
static int get_devtree_pdata(struct device *dev)
{
	struct device_node *node, *pp;
	int count = -1;
	pr_info("zte_hall: translate hardware pin to system pin\n");
	node = dev->of_node;
	if (node == NULL)
		return -ENODEV;
	pp = NULL;
	while ((pp = of_get_next_child(node, pp))) {
		if (!of_find_property(pp, "label", NULL)) {
			dev_warn(dev, "HALL Found without labels\n");
			continue;
		}
		count++;
		zte_hall_gpios[count].name = kstrdup(of_get_property(pp, "label", NULL),
								GFP_KERNEL);
		zte_hall_gpios[count].sys_num = of_get_gpio(pp, 0);
		
		pr_info("zte_hall: sys_number=%d name=%s\n",zte_hall_gpios[count].sys_num,zte_hall_gpios[count].name);
	}
	return 0;
}
static int  zte_hall_probe(struct platform_device *pdev)
{
	//const struct zte_hall_platform_data *pdata = pdev->dev.platform_data;
	int ret = 0;
	int err;
	struct input_dev *hall_pwr;
   	struct device *dev = &pdev->dev;

	
	pr_info("%s +++++\n",__func__);
	
	err = get_devtree_pdata(dev);
	if (err)
		return err;	

	hallpwrkey = kzalloc(sizeof(*hallpwrkey), GFP_KERNEL);
	gpio_hall_en = get_hall_sysnumber_byname(HALL_EN);;
	if (gpio_hall_en)
	{

		if (!hallpwrkey)
		{
			pr_info("alloc address of hallpwrkey failed\n");
			return -ENOMEM;
		}
	hall_pwr = input_allocate_device();
	if (!hall_pwr) {
		pr_info("alloc address of pwr failed\n");
		err = -ENOMEM;
		goto free_pwrkey;
	}

	input_set_capability(hall_pwr, EV_KEY, KEY_POWER);
	err = input_register_device(hall_pwr);
	if (err) {
		pr_info( "Can't register hall ");
		
		goto free_input_dev;
	}

	hall_pwr->name = "hall_imitate_pwrkey";
	hall_pwr->phys = "hall_imitate_pwrkey/input0";
 
	hallpwrkey->hall_pwr = hall_pwr;
	wake_lock_init(&hall_wake_lock, WAKE_LOCK_SUSPEND,
				hall_dev_name[0]);

	ret = request_irq(gpio_to_irq(gpio_hall_en),
			hall_detect_interrupt,
			(IRQF_TRIGGER_RISING |
			IRQF_TRIGGER_FALLING),
			"hall-detect", hallpwrkey);
	hallstate = gpio_get_value(gpio_hall_en);

	if (ret) {
		pr_info("could not request IRQ HALL_DETECT_PIN for detect pin\n");
		gpio_free(gpio_hall_en);
		}
	ret = enable_irq_wake(gpio_to_irq(gpio_hall_en));
	if (ret) {
		pr_info("could not irq_set_irq_wake for detect pin\n");
		}
	}
	return 0;
free_input_dev:
	input_free_device(hall_pwr);
free_pwrkey:
	kfree(hallpwrkey);
	return err;

	}
static int  zte_hall_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver zte_hall_device_driver = {
	.probe		= zte_hall_probe,
	.remove		= zte_hall_remove,
	.driver		= {
		.name	= "zte-hall",
		.owner	= THIS_MODULE,
		.of_match_table = zte_hall_of_match,
	}
};

static int __init zte_hall_init(void)
{
	return platform_driver_register(&zte_hall_device_driver);
}

static void __exit zte_hall_exit(void)
{
	platform_driver_unregister(&zte_hall_device_driver);
}

late_initcall(zte_hall_init);
module_exit(zte_hall_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Hall driver for zte");
MODULE_ALIAS("platform:zte-hall");

