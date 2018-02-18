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
 */

#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/of_fdt.h>
#include <linux/of_irq.h>
#include <asm/mach/arch.h>
#include <soc/qcom/socinfo.h>
#include <mach/board.h>
#include <mach/msm_memtypes.h>
#include <soc/qcom/rpm-smd.h>
#include <soc/qcom/smd.h>
#include <soc/qcom/smem.h>
#include <soc/qcom/spm.h>
#include <soc/qcom/pm.h>
#include "board-dt.h"
#include "platsmp.h"

/*
 * Support for FTM & RECOVERY mode by ZTE_BOOT_RXZ_20131018 ruan.xianzhang
 */
#ifdef CONFIG_ZTE_BOOT_MODE
#define SOCINFO_CMDLINE_BOOTMODE          "androidboot.mode="
#define SOCINFO_CMDLINE_BOOTMODE_NORMAL   "normal"
#define SOCINFO_CMDLINE_BOOTMODE_FTM      "ftm"
#define SOCINFO_CMDLINE_BOOTMODE_RECOVERY "recovery"

static int __init bootmode_init(char *mode)
{
	int is_boot_into_ftm = 0;
	int is_boot_into_recovery = 0;

	if (!strncmp(mode, SOCINFO_CMDLINE_BOOTMODE_NORMAL, strlen(SOCINFO_CMDLINE_BOOTMODE_NORMAL)))
	{
		is_boot_into_ftm = 0;
		is_boot_into_recovery = 0;
	}
	else if (!strncmp(mode, SOCINFO_CMDLINE_BOOTMODE_FTM, strlen(SOCINFO_CMDLINE_BOOTMODE_FTM)))
	{
		is_boot_into_ftm = 1;
		is_boot_into_recovery = 0;
	}
	else if (!strncmp(mode, SOCINFO_CMDLINE_BOOTMODE_RECOVERY, strlen(SOCINFO_CMDLINE_BOOTMODE_RECOVERY)))
	{
		is_boot_into_ftm = 0;
		is_boot_into_recovery = 1;
	}
	else
	{
		is_boot_into_ftm = 0;
		is_boot_into_recovery = 0;
	}

	socinfo_set_ftm_flag(is_boot_into_ftm);
	socinfo_set_recovery_flag(is_boot_into_recovery);

	return 1;
}

#if 0 // To fix compiling error
__setup(SOCINFO_CMDLINE_BOOTMODE, bootmode_init);
#else
static const char __setup_str_bootmode_init[] __initconst __aligned(1) = SOCINFO_CMDLINE_BOOTMODE;
static struct obs_kernel_param __setup_bootmode_init __used __section(.init.setup) __attribute__((aligned((sizeof(long))))) =
	{ SOCINFO_CMDLINE_BOOTMODE, bootmode_init, 0 };
#endif
#endif /* CONFIG_ZTE_BOOT_MODE */

#ifdef CONFIG_ZTE_BOARD_ID
static int __init zte_hw_ver_init(char *ver)
{
	socinfo_set_hw_ver(ver);
	return 0;
}

#define SOCINFO_CMDLINE_HW_VER "androidboot.hw_ver="
static const char __setup_str_hw_ver_init[] __initconst __aligned(1) = SOCINFO_CMDLINE_HW_VER;
static struct obs_kernel_param __setup_hw_ver_init __used __section(.init.setup) __attribute__((aligned((sizeof(long))))) =
	{ SOCINFO_CMDLINE_HW_VER, zte_hw_ver_init, 0 };
#endif

#ifdef CONFIG_ZTE_BOOT_MODE//for PV version detect
#define SOCINFO_CMDLINE_PV_FLAG "androidboot.pv-version="
#define SOCINFO_CMDLINE_PV_VERSION   "1"
#define SOCINFO_CMDLINE_NON_PV_VERSION      "0"
static int __init zte_pv_flag_init(char *ver)
{
	int is_pv_ver = 0;
	
	if (!strncmp(ver, SOCINFO_CMDLINE_PV_VERSION, strlen(SOCINFO_CMDLINE_PV_VERSION)))
	{
		is_pv_ver = 1;
	}
	
	socinfo_set_pv_flag(is_pv_ver);
	return 0;
}

static const char __setup_str_pv_flag_init[] __initconst __aligned(1) = SOCINFO_CMDLINE_PV_FLAG;
static struct obs_kernel_param __setup_pv_flag_init __used __section(.init.setup) __attribute__((aligned((sizeof(long))))) =
	{ SOCINFO_CMDLINE_PV_FLAG, zte_pv_flag_init, 0 };
#endif

static void __init msm8916_dt_reserve(void)
{
	of_scan_flat_dt(dt_scan_for_memory_reserve, NULL);
}

static void __init msm8916_map_io(void)
{
	msm_map_msm8916_io();
}

static struct of_dev_auxdata msm8916_auxdata_lookup[] __initdata = {
	{}
};

/*
 * Used to satisfy dependencies for devices that need to be
 * run early or in a particular order. Most likely your device doesn't fall
 * into this category, and thus the driver should not be added here. The
 * EPROBE_DEFER can satisfy most dependency problems.
 */
void __init msm8916_add_drivers(void)
{
	msm_smd_init();
	msm_rpm_driver_init();
	msm_spm_device_init();
	msm_pm_sleep_status_init();
}

static void __init msm8916_init(void)
{
	struct of_dev_auxdata *adata = msm8916_auxdata_lookup;

	/*
	 * populate devices from DT first so smem probe will get called as part
	 * of msm_smem_init.  socinfo_init needs smem support so call
	 * msm_smem_init before it.
	 */
	of_platform_populate(NULL, of_default_bus_match_table, adata, NULL);
	msm_smem_init();

	if (socinfo_init() < 0)
		pr_err("%s: socinfo_init() failed\n", __func__);

	msm8916_add_drivers();
}

static const char *msm8916_dt_match[] __initconst = {
	"qcom,msm8916",
	"qcom,apq8016",
	NULL
};

static const char *msm8936_dt_match[] __initconst = {
	"qcom,msm8936",
	NULL
};

static const char *msm8939_dt_match[] __initconst = {
	"qcom,msm8939",
	NULL
};

DT_MACHINE_START(MSM8916_DT,
		"Qualcomm Technologies, Inc. MSM 8916 (Flattened Device Tree)")
	.map_io = msm8916_map_io,
	.init_machine = msm8916_init,
	.dt_compat = msm8916_dt_match,
	.reserve = msm8916_dt_reserve,
	.smp = &msm8916_smp_ops,
MACHINE_END

DT_MACHINE_START(MSM8939_DT,
		"Qualcomm Technologies, Inc. MSM 8939 (Flattened Device Tree)")
	.map_io = msm8916_map_io,
	.init_machine = msm8916_init,
	.dt_compat = msm8939_dt_match,
	.reserve = msm8916_dt_reserve,
	.smp = &msm8936_smp_ops,
MACHINE_END

DT_MACHINE_START(MSM8936_DT,
		"Qualcomm Technologies, Inc. MSM 8936 (Flattened Device Tree)")
	.map_io = msm8916_map_io,
	.init_machine = msm8916_init,
	.dt_compat = msm8936_dt_match,
	.reserve = msm8916_dt_reserve,
	.smp = &msm8936_smp_ops,
MACHINE_END
