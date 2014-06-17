/*
 * Copyright (C) ST-Ericsson SA 2010
 *
 * Author: Rabin Vincent <rabin.vincent@stericsson.com> for ST-Ericsson
 * Author: Lee Jones <lee.jones@linaro.org> for ST-Ericsson
 * License terms: GNU General Public License (GPL) version 2
 */

#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/sys_soc.h>
#include <linux/clksrc-dbx500-prcmu.h>
#include <linux/mfd/dbx500-prcmu.h>
#include <linux/irq.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/of.h>
#include <linux/of_irq.h>

#include <asm/hardware/gic.h>
#include <asm/mach/map.h>
#include <asm/localtimer.h>
#include <asm/system_misc.h>

#include <mach/hardware.h>
#include <mach/setup.h>
#include <mach/devices.h>
#include <mach/reboot_reasons.h>
#include <mach/pm.h>

#include "id.h"
#include "clock.h"

void __iomem *_PRCMU_BASE;

static const struct of_device_id ux500_dt_irq_match[] = {
	{ .compatible = "arm,cortex-a9-gic", .data = gic_of_init, },
	{},
};

void ux500_restart(char mode, const char *cmd)
{
	unsigned short reset_code;

	local_irq_disable();
	local_fiq_disable();

	reset_code = reboot_reason_code(cmd);
	prcmu_system_reset(reset_code);

	mdelay(1000);

	printk(KERN_ERR "Reboot via PRCMU failed -- System halted\n");
	while (1)
		;
}

void __init ux500_init_irq(void)
{
	void __iomem *dist_base;
	void __iomem *cpu_base;

	gic_arch_extn.flags = IRQCHIP_SKIP_SET_WAKE | IRQCHIP_MASK_ON_SUSPEND;

	if (cpu_is_u8500_family() || cpu_is_ux540_family()) {
		dist_base = __io_address(U8500_GIC_DIST_BASE);
		cpu_base = __io_address(U8500_GIC_CPU_BASE);
	} else
		ux500_unknown_soc();

#ifdef CONFIG_OF
	if (of_have_populated_dt())
		of_irq_init(ux500_dt_irq_match);
	else
#endif
		gic_init(0, GIC_PPI_START, dist_base, cpu_base);

	/*
	 * On WD reboot gic is in some cases decoupled.
	 * This will make sure that the GIC is correctly configured.
	 */
	ux500_pm_gic_recouple();
	/*
	 * Init clocks here so that they are available for system timer
	 * initialization.
	 */
	clk_init();
}

static const char * __init ux500_get_machine(void)
{
	return kasprintf(GFP_KERNEL, "DB%4x", dbx500_partnumber());
}

static const char * __init ux500_get_family(void)
{
	return kasprintf(GFP_KERNEL, "ux500");
}

static const char * __init ux500_get_revision(void)
{
	unsigned int rev = dbx500_revision();

	if (rev == 0x01)
		return kasprintf(GFP_KERNEL, "%s", "ED");
	else if (rev >= 0xA0)
		return kasprintf(GFP_KERNEL, "%d.%d",
				 (rev >> 4) - 0xA + 1, rev & 0xf);

	return kasprintf(GFP_KERNEL, "%s", "Unknown");
}

static ssize_t ux500_get_process(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	if (dbx500_id.process == 0x00)
		return sprintf(buf, "Standard\n");

	return sprintf(buf, "%02xnm\n", dbx500_id.process);
}

static ssize_t ux500_get_reset_reason(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return sprintf(buf, "%s", reboot_reason_string(prcmu_get_reset_code()));
}

static void __init soc_info_populate(struct soc_device_attribute *soc_dev_attr,
				     const char *soc_id)
{
	soc_dev_attr->soc_id   = soc_id;
	soc_dev_attr->machine  = ux500_get_machine();
	soc_dev_attr->family   = ux500_get_family();
	soc_dev_attr->revision = ux500_get_revision();
}

struct device_attribute ux500_soc_attr =
	__ATTR(process,  S_IRUGO, ux500_get_process,  NULL);

struct device_attribute ux500_soc_reset =
	__ATTR(reset_reason,  S_IRUGO, ux500_get_reset_reason,  NULL);

struct device * __init ux500_soc_device_init(const char *soc_id)
{
	struct device *parent;
	struct soc_device *soc_dev;
	struct soc_device_attribute *soc_dev_attr;

	soc_dev_attr = kzalloc(sizeof(*soc_dev_attr), GFP_KERNEL);
	if (!soc_dev_attr)
		return ERR_PTR(-ENOMEM);

	soc_info_populate(soc_dev_attr, soc_id);

	soc_dev = soc_device_register(soc_dev_attr);
	if (IS_ERR_OR_NULL(soc_dev)) {
	        kfree(soc_dev_attr);
		return NULL;
	}

	parent = soc_device_to_device(soc_dev);
	if (!IS_ERR_OR_NULL(parent)) {
		device_create_file(parent, &ux500_soc_attr);
		device_create_file(parent, &ux500_soc_reset);
	}
	return parent;
}
