/*
 * (C) Copyright 2013-2015
 * NVIDIA Corporation <www.nvidia.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef _HAC_001_H
#define _HAC_001_H

#include <linux/sizes.h>

#include "tegra210-common.h"

/* High-level configuration options */
#define CONFIG_TEGRA_BOARD_STRING	"Nintendo HAC-001"

/* Board-specific serial config */
#define CONFIG_TEGRA_ENABLE_UARTA
#define CONFIG_TEGRA_ENABLE_UARTB
#define CONFIG_TEGRA_ENABLE_UARTC
/*#define DEBUG*/

/* I2C */
#define CONFIG_SYS_I2C_TEGRA

/* Environment in eMMC, at the end of 2nd "boot sector" */
#define CONFIG_SYS_MMC_ENV_DEV		0
#define CONFIG_SYS_MMC_ENV_PART		2
#define CONFIG_ENV_OFFSET		(-CONFIG_ENV_SIZE)

/* USB2.0 Host support */
#define CONFIG_USB_EHCI_TEGRA

/* USB networking support */

/* General networking support */

#include "tegra-common-usb-gadget.h"
#include "tegra-common-post.h"

#endif /* _HAC_001_H */
