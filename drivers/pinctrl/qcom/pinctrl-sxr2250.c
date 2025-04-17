// SPDX-License-Identifier: GPL-2.0+
/*
 * Qualcomm sxr2250 pinctrl
 *
 * (C) Copyright 2024 Linaro Ltd.
 * (C) Copyright 2025 Max Thomas <mtinc2@gmail.com>
 *
 * Based on similar U-Boot drivers.
 * Constants were taken from the Linux drivers and device trees.
 */

#include <dm.h>

#include "pinctrl-qcom.h"

#define MAX_PIN_NAME_LEN 32
static char pin_name[MAX_PIN_NAME_LEN] __section(".data");

static const struct pinctrl_function msm_pinctrl_functions[] = {
    {"qup1_se7", 1},
    {"gpio", 0},
    {"pcie1_clk_req_n", 1},
};

#define SDC_QDSD_PINGROUP(pg_name, ctl, pull, drv)  \
    {                           \
        .name = pg_name,            \
        .ctl_reg = ctl,             \
        .io_reg = 0,                \
        .pull_bit = pull,           \
        .drv_bit = drv,             \
        .oe_bit = -1,               \
        .in_bit = -1,               \
        .out_bit = -1,              \
    }

#define UFS_RESET(pg_name, ctl, io)         \
    {                           \
        .name = pg_name,            \
        .ctl_reg = ctl,             \
        .io_reg = io,               \
        .pull_bit = 3,              \
        .drv_bit = 0,               \
        .oe_bit = -1,               \
        .in_bit = -1,               \
        .out_bit = 0,               \
    }

static const struct msm_special_pin_data msm_special_pins_data[] = {
    [0] = UFS_RESET("ufs_reset", 0x1ee000, 0x1ee004),
    [1] = SDC_QDSD_PINGROUP("sdc2_clk", 0x1e4000, 14, 6),
    [2] = SDC_QDSD_PINGROUP("sdc2_cmd", 0x1e4000, 11, 3),
    [3] = SDC_QDSD_PINGROUP("sdc2_data", 0x1e4000, 9, 0),
};

static const char *sxr2250_get_function_name(struct udevice *dev,
                         unsigned int selector)
{
    return msm_pinctrl_functions[selector].name;
}

static const char *sxr2250_get_pin_name(struct udevice *dev,
                    unsigned int selector)
{
    if (selector >= 224 && selector <= 227)
        snprintf(pin_name, MAX_PIN_NAME_LEN,
             msm_special_pins_data[selector - 224].name);
    else
        snprintf(pin_name, MAX_PIN_NAME_LEN, "gpio%u", selector);

    return pin_name;
}

static unsigned int sxr2250_get_function_mux(__maybe_unused unsigned int pin,
                        unsigned int selector)
{
    return msm_pinctrl_functions[selector].val;
}

static struct msm_pinctrl_data sxr2250_data = {
    .pin_data = {
        .pin_count = 228,
        .special_pins_start = 224,
        .special_pins_data = msm_special_pins_data,
    },
    .functions_count = ARRAY_SIZE(msm_pinctrl_functions),
    .get_function_name = sxr2250_get_function_name,
    .get_function_mux = sxr2250_get_function_mux,
    .get_pin_name = sxr2250_get_pin_name,
};

static const struct udevice_id msm_pinctrl_ids[] = {
    { .compatible = "qcom,sxr2250-tlmm", .data = (ulong)&sxr2250_data },
    { .compatible = "qcom,anorak-pinctrl", .data = (ulong)&sxr2250_data },
    { /* Sentinel */ }
};

U_BOOT_DRIVER(pinctrl_sxr2250) = {
    .name       = "pinctrl_sxr2250",
    .id     = UCLASS_NOP,
    .of_match   = msm_pinctrl_ids,
    .ops        = &msm_pinctrl_ops,
    .bind       = msm_pinctrl_bind,
};

