/* Copyright (c) 2011-2014, The Linux Foundation. All rights reserved.
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
#include <linux/of_fdt.h>
#include <soc/qcom/scm.h>
#include <asm/mach/map.h>
#include <mach/msm_iomap.h>

#include "board-dt.h"




int __init msm_scan_dt_map_imem(unsigned long node, const char *uname,
			int depth, void *data)
{
	unsigned int *imem_prop;
	unsigned long imem_prop_len;
	struct map_desc map;
	int ret;
	const char *compat = "qcom,msm-imem";

	ret = of_flat_dt_is_compatible(node, compat);

	if (!ret)
		return 0;

	imem_prop = of_get_flat_dt_prop(node, "reg",
					&imem_prop_len);

	if (!imem_prop) {
		WARN(1, "IMEM reg field not found\n");
		return 0;
	}

	if (imem_prop_len != (2*sizeof(u32))) {
		WARN(1, "IMEM range malformed\n");
		return 0;
	}

	map.virtual = (unsigned long)MSM_IMEM_BASE;
	map.pfn = __phys_to_pfn(be32_to_cpu(imem_prop[0]));
	map.length = be32_to_cpu(imem_prop[1]);
	map.type = MT_DEVICE;
	iotable_init(&map, 1);
	pr_info("IMEM DT static mapping successful\n");

	return 1;
}


void __init board_dt_populate(struct of_dev_auxdata *adata)
{
	of_platform_populate(NULL, of_default_bus_match_table, NULL, NULL);

	/* Explicitly parent the /soc devices to the root node to preserve
	 * the kernel ABI (sysfs structure, etc) until userspace is updated
	 */
	of_platform_populate(of_find_node_by_path("/soc"),
			     of_default_bus_match_table, adata, NULL);
}



