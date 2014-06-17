/*
 * Copyright (C) ST-Ericsson SA 2010
 *
 * Author: Hanumath Prasad <hanumath.prasad@stericsson.com>
 * License terms: GNU General Public License (GPL) version 2
 */

#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/amba/bus.h>
#include <linux/amba/mmci.h>
#include <linux/mmc/host.h>
#include <linux/platform_device.h>

#include <asm/mach-types.h>
#include <plat/ste_dma40.h>
#include <mach/devices.h>
#include <mach/hardware.h>
#include <mach/ste-dma40-db8500.h>

#include "board-common-sdi.h"
#include "devices-db8500.h"
#include "board-mop500.h"

/*
 * SDI 0 (MicroSD slot)
 */

/* GPIO pins used by the sdi0 level shifter */
static struct sdi_ios_pins sdi0_ios_pins = {
	.enable = -1,
	.vsel = -1,
};

#ifdef CONFIG_STE_DMA40
static struct stedma40_chan_cfg mop500_sdi0_dma_cfg_rx = {
	.mode = STEDMA40_MODE_LOGICAL,
	.dir = STEDMA40_PERIPH_TO_MEM,
	.src_dev_type = DB8500_DMA_DEV1_SD_MMC0_RX,
	.dst_dev_type = STEDMA40_DEV_DST_MEMORY,
	.src_info.data_width = STEDMA40_WORD_WIDTH,
	.dst_info.data_width = STEDMA40_WORD_WIDTH,
	.use_fixed_channel = true,
	.phy_channel = 0,
};

static struct stedma40_chan_cfg mop500_sdi0_dma_cfg_tx = {
	.mode = STEDMA40_MODE_LOGICAL,
	.dir = STEDMA40_MEM_TO_PERIPH,
	.src_dev_type = STEDMA40_DEV_SRC_MEMORY,
	.dst_dev_type = DB8500_DMA_DEV1_SD_MMC0_TX,
	.src_info.data_width = STEDMA40_WORD_WIDTH,
	.dst_info.data_width = STEDMA40_WORD_WIDTH,
	.use_fixed_channel = true,
	.phy_channel = 0,
};
#endif

static struct mmci_platform_data mop500_sdi0_data = {
	.f_max		= 100000000,
	.capabilities	= MMC_CAP_4_BIT_DATA |
				MMC_CAP_SD_HIGHSPEED |
				MMC_CAP_MMC_HIGHSPEED |
				MMC_CAP_UHS_SDR12 |
				MMC_CAP_UHS_SDR25 |
				MMC_CAP_UHS_DDR50,
	.capabilities2	= MMC_CAP2_DETECT_ON_ERR,
	.gpio_wp	= -1,
	.levelshifter	= true,
	.sigdir		= MCI_ST_FBCLKEN |
				MCI_ST_CMDDIREN |
				MCI_ST_DATA0DIREN |
				MCI_ST_DATA2DIREN,
#ifdef CONFIG_STE_DMA40
	.dma_filter	= stedma40_filter,
	.dma_rx_param	= &mop500_sdi0_dma_cfg_rx,
	.dma_tx_param	= &mop500_sdi0_dma_cfg_tx,
#endif
};

/*
 * SDI1 (SDIO WLAN)
 */
#ifdef CONFIG_STE_DMA40
static struct stedma40_chan_cfg sdi1_dma_cfg_rx = {
	.mode = STEDMA40_MODE_LOGICAL,
	.dir = STEDMA40_PERIPH_TO_MEM,
	.src_dev_type = DB8500_DMA_DEV32_SD_MM1_RX,
	.dst_dev_type = STEDMA40_DEV_DST_MEMORY,
	.src_info.data_width = STEDMA40_WORD_WIDTH,
	.dst_info.data_width = STEDMA40_WORD_WIDTH,
};

static struct stedma40_chan_cfg sdi1_dma_cfg_tx = {
	.mode = STEDMA40_MODE_LOGICAL,
	.dir = STEDMA40_MEM_TO_PERIPH,
	.src_dev_type = STEDMA40_DEV_SRC_MEMORY,
	.dst_dev_type = DB8500_DMA_DEV32_SD_MM1_TX,
	.src_info.data_width = STEDMA40_WORD_WIDTH,
	.dst_info.data_width = STEDMA40_WORD_WIDTH,
};
#endif

static struct mmci_platform_data mop500_sdi1_data = {
	.ocr_mask	= MMC_VDD_29_30,
	.f_max		= 50000000,
	.capabilities	= MMC_CAP_4_BIT_DATA,
	.gpio_cd	= -1,
	.gpio_wp	= -1,
#ifdef CONFIG_STE_DMA40
	.dma_filter	= stedma40_filter,
	.dma_rx_param	= &sdi1_dma_cfg_rx,
	.dma_tx_param	= &sdi1_dma_cfg_tx,
#endif
};

static void sdi0_sdi1_configure(struct device *parent)
{
	int ret;
	/* v2 has a new version of this block that need to be forced */
	u32 periphid = 0x10480180;

	ret = ux500_common_sdi0_ios_handler_init(&mop500_sdi0_data,
			&sdi0_ios_pins);
	if (ret)
		pr_err("%s: SDI0 ios hanfler init failed (%d)", __func__, ret);

	db8500_add_sdi0(parent, &mop500_sdi0_data, periphid);
	db8500_add_sdi1(parent, &mop500_sdi1_data, periphid);
}

void mop500_sdi_tc35892_init(struct device *parent)
{
	mop500_sdi0_data.gpio_cd = GPIO_SDMMC_CD;
	sdi0_ios_pins.enable = GPIO_SDMMC_EN;
	sdi0_ios_pins.vsel = GPIO_SDMMC_1V8_3V_SEL;
	sdi0_sdi1_configure(parent);
}

/*
 * SDI 2 (POP eMMC, not on DB8500ed)
 */
#ifdef CONFIG_STE_DMA40
struct stedma40_chan_cfg mop500_sdi2_dma_cfg_rx = {
	.mode = STEDMA40_MODE_LOGICAL,
	.dir = STEDMA40_PERIPH_TO_MEM,
	.src_dev_type =  DB8500_DMA_DEV28_SD_MM2_RX,
	.dst_dev_type = STEDMA40_DEV_DST_MEMORY,
	.src_info.data_width = STEDMA40_WORD_WIDTH,
	.dst_info.data_width = STEDMA40_WORD_WIDTH,
};

static struct stedma40_chan_cfg mop500_sdi2_dma_cfg_tx = {
	.mode = STEDMA40_MODE_LOGICAL,
	.dir = STEDMA40_MEM_TO_PERIPH,
	.src_dev_type = STEDMA40_DEV_SRC_MEMORY,
	.dst_dev_type = DB8500_DMA_DEV28_SD_MM2_TX,
	.src_info.data_width = STEDMA40_WORD_WIDTH,
	.dst_info.data_width = STEDMA40_WORD_WIDTH,
};
#endif

static struct mmci_platform_data mop500_sdi2_data = {
	.ocr_mask	= MMC_VDD_165_195,
	.f_max		= 100000000,
	.capabilities	= MMC_CAP_4_BIT_DATA |
				MMC_CAP_8_BIT_DATA |
				MMC_CAP_MMC_HIGHSPEED |
				MMC_CAP_ERASE |
				MMC_CAP_1_8V_DDR |
				MMC_CAP_UHS_DDR50,
	.capabilities2	= MMC_CAP2_NO_SLEEP_CMD,
	.gpio_cd	= -1,
	.gpio_wp	= -1,
#ifdef CONFIG_STE_DMA40
	.dma_filter	= stedma40_filter,
	.dma_rx_param	= &mop500_sdi2_dma_cfg_rx,
	.dma_tx_param	= &mop500_sdi2_dma_cfg_tx,
#endif
};

/*
 * SDI 4 (on-board eMMC)
 */

#ifdef CONFIG_STE_DMA40
struct stedma40_chan_cfg mop500_sdi4_dma_cfg_rx = {
	.mode = STEDMA40_MODE_LOGICAL,
	.dir = STEDMA40_PERIPH_TO_MEM,
	.src_dev_type =  DB8500_DMA_DEV42_SD_MM4_RX,
	.dst_dev_type = STEDMA40_DEV_DST_MEMORY,
	.src_info.data_width = STEDMA40_WORD_WIDTH,
	.dst_info.data_width = STEDMA40_WORD_WIDTH,
};

static struct stedma40_chan_cfg mop500_sdi4_dma_cfg_tx = {
	.mode = STEDMA40_MODE_LOGICAL,
	.dir = STEDMA40_MEM_TO_PERIPH,
	.src_dev_type = STEDMA40_DEV_SRC_MEMORY,
	.dst_dev_type = DB8500_DMA_DEV42_SD_MM4_TX,
	.src_info.data_width = STEDMA40_WORD_WIDTH,
	.dst_info.data_width = STEDMA40_WORD_WIDTH,
};
#endif

static struct mmci_platform_data mop500_sdi4_data = {
	.f_max		= 50000000,
	.capabilities	= MMC_CAP_4_BIT_DATA |
				MMC_CAP_8_BIT_DATA |
				MMC_CAP_MMC_HIGHSPEED |
				MMC_CAP_1_8V_DDR |
				MMC_CAP_UHS_DDR50,

	.gpio_cd	= -1,
	.gpio_wp	= -1,
#ifdef CONFIG_STE_DMA40
	.dma_filter	= stedma40_filter,
	.dma_rx_param	= &mop500_sdi4_dma_cfg_rx,
	.dma_tx_param	= &mop500_sdi4_dma_cfg_tx,
#endif
};

void __init mop500_sdi_init(struct device *parent)
{
	/* v2 has a new version of this block that need to be forced */
	u32 periphid = 0x10480180;

	/* sdi2 on snowball is in ATL_B mode for FSMC (LAN) */
	if (!machine_is_snowball())
		db8500_add_sdi2(parent, &mop500_sdi2_data, periphid);

	/* On-board eMMC */
	db8500_add_sdi4(parent, &mop500_sdi4_data, periphid);

	if (machine_is_hrefv60() || machine_is_u8520() ||
	    machine_is_snowball() || machine_is_a9500()) {
		if (machine_is_hrefv60() || machine_is_a9500()) {
			mop500_sdi0_data.gpio_cd = HREFV60_SDMMC_CD_GPIO;
			sdi0_ios_pins.enable = HREFV60_SDMMC_EN_GPIO;
			sdi0_ios_pins.vsel = HREFV60_SDMMC_1V8_3V_GPIO;
		} else if (machine_is_u8520()) {
			mop500_sdi0_data.gpio_cd = U8520_SDMMC_CD_GPIO;
			sdi0_ios_pins.enable = U8520_SDMMC_EN_GPIO;
			sdi0_ios_pins.vsel = U8520_SDMMC_1V8_3V_GPIO;
		} else if (machine_is_snowball()) {
			mop500_sdi0_data.gpio_cd = SNOWBALL_SDMMC_CD_GPIO;
			mop500_sdi0_data.cd_invert = true;
			sdi0_ios_pins.enable = SNOWBALL_SDMMC_EN_GPIO;
			sdi0_ios_pins.vsel = SNOWBALL_SDMMC_1V8_3V_GPIO;
		}
		sdi0_sdi1_configure(parent);
	}

	/*
	 * On boards with the TC35892 GPIO expander, sdi0 and sdi1 will finally
	 * be added when the TC35892 initializes and calls
	 * mop500_sdi_tc35892_init() above.
	 */
}
