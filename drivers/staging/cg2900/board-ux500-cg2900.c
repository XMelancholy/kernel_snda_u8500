/*
 * Copyright (C) ST-Ericsson SA 2011
 *
 * Author: Par-Gunnar Hjalmdahl <par-gunnar.p.hjalmdahl@stericsson.com>
 * Author: Hemant Gupta <hemant.gupta@stericsson.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 *
 */

#include <asm/mach-types.h>
#include <linux/gpio.h>
#include <linux/ioport.h>
#include <linux/mfd/abx500/ab8500-gpio.h>
#include <linux/platform_device.h>
#include <mach/gpio.h>
#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci.h>
#include <plat/gpio-nomadik.h>
#include <plat/pincfg.h>

#include "board-mop500.h"
#include "cg2900.h"
#include "devices-cg2900.h"
#include "pins-db8500.h"
#include "pins.h"
#include "id.h"

#define CG2900_BT_ENABLE_GPIO		170
#define CG2900_GBF_ENA_RESET_GPIO	171
#define WLAN_PMU_EN_GPIO		226
#define WLAN_PMU_EN_GPIO_SNOWBALL	161
#define WLAN_PMU_EN_GPIO_U9500		AB8500_PIN_GPIO(11)
#define CG2900_UX500_BT_CTS_GPIO	0

/* MACROS FOR U9540 */
#define WLAN_U9540_PMU_EN_GPIO			AB8500_PIN_GPIO(16)
#define CG2900_U9540_GBF_ENA_RESET_GPIO		AB8500_PIN_GPIO(21)

/* MACROS FOR U8540 */
#define CG2900_U8540_GBF_ENA_RESET_GPIO		AB8500_PIN_GPIO(51)

enum cg2900_gpio_pull_sleep ux500_cg2900_sleep_gpio[23] = {
	CG2900_NO_PULL,		/* GPIO 0:  PTA_CONFX */
	CG2900_PULL_DN,		/* GPIO 1:  PTA_STATUS */
	CG2900_NO_PULL,		/* GPIO 2:  UART_CTSN */
	CG2900_PULL_UP,		/* GPIO 3:  UART_RTSN */
	CG2900_PULL_UP,		/* GPIO 4:  UART_TXD */
	CG2900_NO_PULL,		/* GPIO 5:  UART_RXD */
	CG2900_PULL_DN,		/* GPIO 6:  IOM_DOUT */
	CG2900_NO_PULL,		/* GPIO 7:  IOM_FSC */
	CG2900_NO_PULL,		/* GPIO 8:  IOM_CLK */
	CG2900_NO_PULL,		/* GPIO 9:  IOM_DIN */
	CG2900_PULL_DN,		/* GPIO 10: PWR_REQ */
	CG2900_PULL_DN,		/* GPIO 11: HOST_WAKEUP */
	CG2900_PULL_DN,		/* GPIO 12: IIS_DOUT */
	CG2900_NO_PULL,		/* GPIO 13: IIS_WS */
	CG2900_NO_PULL,		/* GPIO 14: IIS_CLK */
	CG2900_NO_PULL,		/* GPIO 15: IIS_DIN */
	CG2900_PULL_DN,		/* GPIO 16: PTA_FREQ */
	CG2900_PULL_DN,		/* GPIO 17: PTA_RF_ACTIVE */
	CG2900_NO_PULL,		/* GPIO 18: NotConnected (J6428) */
	CG2900_NO_PULL,		/* GPIO 19: EXT_DUTY_CYCLE */
	CG2900_NO_PULL,		/* GPIO 20: EXT_FRM_SYNCH */
	CG2900_PULL_UP,		/* GPIO 21: BT_ANT_SEL_CLK */
	CG2900_PULL_UP,		/* GPIO 22: BT_ANT_SEL_DATA */
};

static struct platform_device ux500_cg2900_device = {
	.name = "cg2900",
};

static struct platform_device ux500_cg2900_chip_device = {
	.name = "cg2900-chip",
	.dev = {
		.parent = &ux500_cg2900_device.dev,
	},
};

static struct platform_device ux500_stlc2690_chip_device = {
	.name = "stlc2690-chip",
	.dev = {
		.parent = &ux500_cg2900_device.dev,
	},
};

static struct cg2900_platform_data ux500_cg2900_test_platform_data = {
	.bus = HCI_VIRTUAL,
	.gpio_sleep = ux500_cg2900_sleep_gpio,
};

static struct platform_device ux500_cg2900_test_device = {
	.name = "cg2900-test",
	.dev = {
		.parent = &ux500_cg2900_device.dev,
		.platform_data = &ux500_cg2900_test_platform_data,
	},
};

static struct resource cg2900_uart_resources[] = {
	{
		.start = NOMADIK_GPIO_TO_IRQ(CG2900_UX500_BT_CTS_GPIO),
		.end = NOMADIK_GPIO_TO_IRQ(CG2900_UX500_BT_CTS_GPIO),
		.flags = IORESOURCE_IRQ,
		.name = "cts_irq",
	},
};

static pin_cfg_t ux500_cg2900_uart_enabled[] = {
	GPIO0_U0_CTSn   | PIN_INPUT_PULLUP,
	GPIO1_U0_RTSn   | PIN_OUTPUT_HIGH,
	GPIO2_U0_RXD    | PIN_INPUT_PULLUP,
	GPIO3_U0_TXD    | PIN_OUTPUT_HIGH
};

static pin_cfg_t ux500_cg2900_uart_disabled[] = {
	GPIO0_GPIO   | PIN_INPUT_PULLUP,	/* CTS pull up. */
	GPIO1_GPIO   | PIN_OUTPUT_HIGH,		/* RTS high-flow off. */
	GPIO2_GPIO   | PIN_INPUT_PULLUP,	/* RX pull up. */
	GPIO3_GPIO   | PIN_OUTPUT_LOW		/* TX low - break on. */
};

static struct cg2900_platform_data ux500_cg2900_uart_platform_data = {
	.bus = HCI_UART,
	.gpio_sleep = ux500_cg2900_sleep_gpio,
	.uart = {
		.n_uart_gpios = 4,
	},
};

static struct platform_device ux500_cg2900_uart_device = {
	.name = "cg2900-uart",
	.dev = {
		.platform_data = &ux500_cg2900_uart_platform_data,
		.parent = &ux500_cg2900_device.dev,
	},
};

static bool mach_supported(void)
{
	if (machine_is_u8500() ||
	    machine_is_hrefv60() ||
	    machine_is_u8520() ||
	    machine_is_nomadik() ||
	    machine_is_snowball() ||
	    machine_is_u9540() ||
	    machine_is_u8540() ||
	    machine_is_a9500())
		return true;

	return false;
}

static void set_pdata_gpios(struct cg2900_platform_data *pdata,
			    int gbf_ena_reset, int bt_enable,
			    int cts_gpio, int pmu_en)
{
	pdata->gpios.gbf_ena_reset = gbf_ena_reset;
	pdata->gpios.bt_enable = bt_enable;
	pdata->gpios.cts_gpio = cts_gpio;
	pdata->gpios.pmu_en = pmu_en;
}

static int __init board_cg2900_init(void)
{
	int err;

	if (!mach_supported())
		return 0;

	dcg2900_init_platdata(&ux500_cg2900_test_platform_data);
	ux500_cg2900_uart_platform_data.uart.uart_enabled =
		ux500_cg2900_uart_enabled;
	ux500_cg2900_uart_platform_data.uart.uart_disabled =
		ux500_cg2900_uart_disabled;
	ux500_cg2900_uart_platform_data.regulator_id = "gbf_1v8";
	ux500_cg2900_uart_platform_data.regulator_wlan_id = NULL;

	/* Set -1 as default (i.e. GPIO not used) */
	set_pdata_gpios(&ux500_cg2900_uart_platform_data, -1, -1, -1, -1);

	dcg2900_init_platdata(&ux500_cg2900_uart_platform_data);

	ux500_cg2900_uart_device.num_resources =
			ARRAY_SIZE(cg2900_uart_resources);
	ux500_cg2900_uart_device.resource =
			cg2900_uart_resources;

	if (machine_is_a9500()) {
		set_pdata_gpios(&ux500_cg2900_uart_platform_data,
				CG2900_GBF_ENA_RESET_GPIO,
				-1,
				CG2900_UX500_BT_CTS_GPIO,
				WLAN_PMU_EN_GPIO_U9500);
	} else if (cpu_is_u8500_family()) {
		if (machine_is_hrefv60() || machine_is_u8520()
		    || machine_is_a9500()) {
			set_pdata_gpios(&ux500_cg2900_uart_platform_data,
					CG2900_GBF_ENA_RESET_GPIO,
					-1,
					CG2900_UX500_BT_CTS_GPIO,
					WLAN_PMU_EN_GPIO);
		} else if (machine_is_snowball()) {
			/* snowball have diffrent PMU_EN gpio */
			ux500_cg2900_uart_platform_data.regulator_wlan_id =
					"vdd";
			set_pdata_gpios(&ux500_cg2900_uart_platform_data,
					CG2900_GBF_ENA_RESET_GPIO,
					-1,
					CG2900_UX500_BT_CTS_GPIO,
					WLAN_PMU_EN_GPIO_SNOWBALL);
		} else {
			/* u8500 pre v60*/
			set_pdata_gpios(&ux500_cg2900_uart_platform_data,
					CG2900_GBF_ENA_RESET_GPIO,
					CG2900_BT_ENABLE_GPIO,
					CG2900_UX500_BT_CTS_GPIO,
					-1);
		}
	} else if (cpu_is_ux540_family()) {
		if (machine_is_u8540()) {
			ux500_cg2900_uart_platform_data.regulator_id = NULL;
			set_pdata_gpios(&ux500_cg2900_uart_platform_data,
					CG2900_U8540_GBF_ENA_RESET_GPIO,
					-1,
					CG2900_UX500_BT_CTS_GPIO,
					-1);
		} else {
			/* u9540 */
			set_pdata_gpios(&ux500_cg2900_uart_platform_data,
					CG2900_U9540_GBF_ENA_RESET_GPIO,
					-1,
					CG2900_UX500_BT_CTS_GPIO,
					WLAN_U9540_PMU_EN_GPIO);
		}
	}

	err = platform_device_register(&ux500_cg2900_device);
	if (err)
		return err;
	err = platform_device_register(&ux500_cg2900_uart_device);
	if (err)
		return err;
	err = platform_device_register(&ux500_cg2900_test_device);
	if (err)
		return err;
	err = platform_device_register(&ux500_cg2900_chip_device);
	if (err)
		return err;
	err = platform_device_register(&ux500_stlc2690_chip_device);
	if (err)
		return err;

	dev_info(&ux500_cg2900_device.dev, "CG2900 initialized\n");
	return 0;
}

static void __exit board_cg2900_exit(void)
{
	if (!mach_supported())
		return;

	platform_device_unregister(&ux500_stlc2690_chip_device);
	platform_device_unregister(&ux500_cg2900_chip_device);
	platform_device_unregister(&ux500_cg2900_test_device);
	platform_device_unregister(&ux500_cg2900_uart_device);
	platform_device_unregister(&ux500_cg2900_device);

	dev_info(&ux500_cg2900_device.dev, "CG2900 removed\n");
}

module_init(board_cg2900_init);
module_exit(board_cg2900_exit);
