/*
 * drivers/staging/zte_ftm/ftm.c
 *
 * Copyright (C) 2012-2013 ZTE, Corp.
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
 *  Date         Author           Comment
 *  -----------  --------------   -------------------------------------
 *  2012-08-03   Jia              The kernel module for FTM,
 *                                created by ZTE_BOO_JIA_20120803 jia.jia
 *  -------------------------------------------------------------------
 *
 */

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/io.h>
#include <linux/device.h>

#include <soc/qcom/scm.h>
#include <soc/qcom/smd.h>
//#include <mach/boot_shared_imem_cookie.h>

/*
 * Macro Definition
 */
#define FTM_VERSION "2.0"
#define FTM_CLASS_NAME "zte_ftm"

#define QFPROM_WRITE_ROW_ID	           0x03
#define QFPROM_WRITE_MULTIPLE_ROWS_ID  0x04
#define QFPROM_READ_ROW_ID             0x05
#define QFPROM_ROLLBACK_WRITE_ROW_ID   0x06

#define FUSION_OPT_UNDEFINE	          0x00
#define FUSION_OPT_WRITE_SECURE_BOOT  0x01
#define FUSION_OPT_READ_SECURE_BOOT	  0x02
#define FUSION_OPT_WRITE_SIMLOCK      0x03
#define FUSION_OPT_READ_SIMLOCK	      0x04

#define FUSION_MAGIC_NUM_1  0x46555349
#define FUSION_MAGIC_NUM_2  0x4F4E2121

#define QFPROM_STATUS_FUSE_OK   0x01
#define QFPROM_STATUS_FUSE_FAIL	0x00
#define QFPROM_STATUS_UNKOWN    0xff

/*
 * Type Definition
 */
typedef struct {
	uint32_t secboot_status;
	uint32_t simlock_status;
} ftm_sysdev_attrs_t;

typedef struct {
	uint32_t raw_row_address;  /* Row address */
	uint32_t row_data_msb;	   /* MSB row data */
	uint32_t row_data_lsb;     /* LSB row data */
} write_row_type;

typedef enum
{
	QFPROM_ADDR_SPACE_RAW  = 0,          /* Raw address space. */
	QFPROM_ADDR_SPACE_CORR = 1,          /* Corrected address space. */
	QFPROM_ADDR_SPACE_MAX  = 0x7FFFFFFF  /* Last entry in the QFPROM_ADDR_SPACE enumerator. */
} QFPROM_ADDR_SPACE;

typedef struct {
	uint32_t row_address;
	uint32_t addr_type;
	uint32_t *fuse_data;
	uint32_t *status;
} read_row_cmd;

/*
 * Global Variables Definition
 */
static ftm_sysdev_attrs_t ftm_sysdev_attrs;
static spinlock_t ftm_spinlock;

/*
 * Function declaration
 */

static ssize_t show_simlock_status(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	uint32_t *smem_fuse_status = NULL;
	
	smem_fuse_status = (int *)smem_alloc(SMEM_ID_VENDOR2, sizeof(uint32_t)*2, 0, SMEM_ANY_HOST_FLAG);
	
	if (!smem_fuse_status)
	{
		pr_err("%s: alloc smem failed!\n", __func__);
		return -EFAULT;
	}
	
	/*
	 * 0: No Blown
	 * 1: Has Blown
	 * 0xff: status error
	 */
	ftm_sysdev_attrs.simlock_status = *smem_fuse_status;

	pr_info("%s: show_simlock_fuse_status.\n", __func__);
	
	return snprintf(buf, PAGE_SIZE, "%d\n", ftm_sysdev_attrs.simlock_status);
}

static ssize_t show_secboot_status(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	uint32_t *smem_fuse_status = NULL;

	smem_fuse_status = (int *)smem_alloc(SMEM_ID_VENDOR2, sizeof(uint32_t)*2, 0, SMEM_ANY_HOST_FLAG);

	if (!smem_fuse_status)
	{
		pr_err("%s: alloc smem failed!\n", __func__);
		return -EFAULT;
	}	
	
	/*
	 * 0: No Blown
	 * 1: Has Blown
	 * 0xff: status error

	 */
	ftm_sysdev_attrs.secboot_status = *(smem_fuse_status+1);

	pr_info("%s: show_simlock_fuse_status.\n", __func__);
	
	return snprintf(buf, PAGE_SIZE, "%d\n", ftm_sysdev_attrs.secboot_status);
	
}

#if 0
static ssize_t store_efs_recovery(struct kobject *kobj, struct kobj_attribute *attr,
				  const char *buf, size_t buf_sz)
{
	struct boot_shared_imem_cookie_type *boot_shared_imem_cookie_ptr = (struct boot_shared_imem_cookie_type *)MSM_IMEM_BASE;
	uint32_t efs_recovery_enable;

	pr_info("%s: e\n", __func__);

	if(!boot_shared_imem_cookie_ptr)
		return -EFAULT;

	sscanf(buf, "%d", &efs_recovery_enable);

	pr_info("%s: efs_recovery_enable: %d\n", __func__, efs_recovery_enable);

	if(efs_recovery_enable)
	{
		boot_shared_imem_cookie_ptr->efs_recovery_flag= 0x01;
	}
	else
	{
		boot_shared_imem_cookie_ptr->efs_recovery_flag = 0x00;
	}
	mb();

	pr_info("%s: x\n", __func__);

	return buf_sz;
}

static ssize_t show_efs_recovery(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	struct boot_shared_imem_cookie_type *boot_shared_imem_cookie_ptr = (struct boot_shared_imem_cookie_type *)MSM_IMEM_BASE;

	pr_info("%s: e\n", __func__);

	if(!boot_shared_imem_cookie_ptr)
		return -EFAULT;

	return snprintf(buf, PAGE_SIZE, "%d\n", boot_shared_imem_cookie_ptr->efs_recovery_flag);
}

static struct kobj_attribute attr_efs_recovery =
	__ATTR(efs_recovery, S_IRUGO | S_IWUSR, show_efs_recovery, store_efs_recovery);
#endif

static struct kobj_attribute attr_secboot_status =
	__ATTR(secboot_status, S_IRUGO | S_IRUSR, show_secboot_status, NULL);

static struct kobj_attribute attr_simlock_status =
	__ATTR(simlock_status, S_IRUGO | S_IRUSR, show_simlock_status, NULL);

static struct device ftm_device = {
	.init_name = "zte_ftm_device",
};

static struct attribute *ftm_attrs[] = {
	&attr_secboot_status.attr,
	&attr_simlock_status.attr,
//	&attr_efs_recovery.attr,
	NULL,
};

/*
 *
 * sysdev file:
 *
 * /sys/devices/zte_ftm_device/zte_ftm/secboot_status
 * /sys/devices/zte_ftm_device/zte_ftm/simlock_status
 */
static struct attribute_group ftm_attr_group = {
	.name = FTM_CLASS_NAME,
	.attrs = ftm_attrs,
};


static int32_t __init ftm_scm_call_check(void)
{
        int ret;
        uint32_t cmd_id;

        for (cmd_id = QFPROM_WRITE_ROW_ID; cmd_id <= QFPROM_ROLLBACK_WRITE_ROW_ID; cmd_id++) {
                ret = scm_is_call_available(SCM_SVC_FUSE, cmd_id);
                pr_info("%s: cmd_id: 0x%02x, ret: %d\n", __func__, cmd_id, ret);

                if (ret <= 0) {
                        return ret;
                }
        }

        return 0;
}


/*
 * Initializes the module.
 */
static int32_t __init ftm_init(void)
{
	int ret;

	pr_info("%s: e\n", __func__);

	ftm_scm_call_check();
	
	/*
	 * Initialize spinlock
	 */
	spin_lock_init(&ftm_spinlock);

	ret = device_register(&ftm_device);
	if (ret) {
		pr_err("Error registering cpaccess device\n");
		goto exit0;
	}

	ret = sysfs_create_group(&ftm_device.kobj, &ftm_attr_group);
	if (ret) {
		pr_err("Error creating cpaccess sysfs group\n");
		goto exit1;
	}


	pr_info("%s: x\n", __func__);

	return 0;
exit1:
	device_unregister(&ftm_device);
exit0:
	return ret;
}

/*
 * Cleans up the module.
 */
static void __exit ftm_exit(void)
{
	sysfs_remove_group(&ftm_device.kobj, &ftm_attr_group);
	device_unregister(&ftm_device);
}

module_init(ftm_init);
module_exit(ftm_exit);

MODULE_DESCRIPTION("ZTE FTM Driver Ver %s" FTM_VERSION);
#ifdef MODULE_LICENSE
MODULE_LICENSE("GPL");
#endif
