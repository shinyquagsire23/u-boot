// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2020, The Linux Foundation. All rights reserved.
 * Copyright (C) 2023 Bhupesh Sharma <bhupesh.sharma@linaro.org>
 *
 * Based on Linux driver
 */

#define LOG_DEBUG

#include <dm.h>
#include <dm/device_compat.h>
#include <dm/devres.h>
#include <generic-phy.h>
#include <malloc.h>
#include <reset.h>

#include <asm/io.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/iopoll.h>


#define USB2_PHY_USB_PHY_UTMI_CTRL0		(0x3c)
#define SLEEPM					BIT(0)
#define OPMODE_MASK				GENMASK(4, 3)
#define OPMODE_NORMAL				(0x00)
#define OPMODE_NONDRIVING			BIT(3)
#define TERMSEL					BIT(5)

#define USB2_PHY_USB_PHY_UTMI_CTRL1		(0x40)
#define XCVRSEL					BIT(0)

#define USB2_PHY_USB_PHY_UTMI_CTRL5		(0x50)
#define POR					BIT(1)

#define USB2_PHY_USB_PHY_HS_PHY_CTRL_COMMON0	(0x54)
#define SIDDQ					BIT(2)
#define RETENABLEN				BIT(3)
#define FSEL_MASK				GENMASK(6, 4)
#define FSEL_DEFAULT				(0x3 << 4)

#define USB2_PHY_USB_PHY_HS_PHY_CTRL_COMMON1	(0x58)
#define VBUSVLDEXTSEL0				BIT(4)
#define PLLBTUNE				BIT(5)

#define USB2_PHY_USB_PHY_HS_PHY_CTRL_COMMON2	(0x5c)
#define VREGBYPASS				BIT(0)

#define USB2_PHY_USB_PHY_HS_PHY_CTRL1		(0x60)
#define VBUSVLDEXT0				BIT(0)

#define USB2_PHY_USB_PHY_HS_PHY_CTRL2		(0x64)
#define USB2_AUTO_RESUME			BIT(0)
#define USB2_SUSPEND_N				BIT(2)
#define USB2_SUSPEND_N_SEL			BIT(3)

#define USB2_PHY_USB_PHY_HS_PHY_OVERRIDE_X0		(0x6c)
#define USB2_PHY_USB_PHY_HS_PHY_OVERRIDE_X1		(0x70)
#define USB2_PHY_USB_PHY_HS_PHY_OVERRIDE_X2		(0x74)
#define USB2_PHY_USB_PHY_HS_PHY_OVERRIDE_X3		(0x78)
#define PARAM_OVRD_MASK				0xFF

#define USB2_PHY_USB_PHY_CFG0			(0x94)
#define UTMI_PHY_DATAPATH_CTRL_OVERRIDE_EN	BIT(0)
#define UTMI_PHY_CMN_CTRL_OVERRIDE_EN		BIT(1)

#define USB2_PHY_USB_PHY_REFCLK_CTRL		(0xa0)
#define REFCLK_SEL_MASK				GENMASK(1, 0)
#define REFCLK_SEL_DEFAULT			(0x2 << 0)

#define HS_DISCONNECT_MASK			GENMASK(2, 0)
#define SQUELCH_DETECTOR_MASK			GENMASK(7, 5)

#define HS_AMPLITUDE_MASK			GENMASK(3, 0)
#define PREEMPHASIS_DURATION_MASK		BIT(5)
#define PREEMPHASIS_AMPLITUDE_MASK		GENMASK(7, 6)

#define HS_RISE_FALL_MASK			GENMASK(1, 0)
#define HS_CROSSOVER_VOLTAGE_MASK		GENMASK(3, 2)
#define HS_OUTPUT_IMPEDANCE_MASK		GENMASK(5, 4)

#define LS_FS_OUTPUT_IMPEDANCE_MASK		GENMASK(3, 0)


struct override_param {
	s32	value;
	u8	reg_val;
};

struct override_param_map {
	const char *prop_name;
	const struct override_param *param_table;
	u8 table_size;
	u8 reg_offset;
	u8 param_mask;
};

struct phy_override_seq {
	bool	need_update;
	u8	offset;
	u8	value;
	u8	mask;
};

#define NUM_HSPHY_TUNING_PARAMS	(9)

struct qcom_snps_hsphy {
	void __iomem *base;
	struct reset_ctl_bulk resets;
	struct phy_override_seq update_seq_cfg[NUM_HSPHY_TUNING_PARAMS];
};

/*
 * We should just be able to use clrsetbits_le32() here, but this results
 * in crashes on some boards. This is suspected to be because of some bus
 * arbitration quirks with the PHY (i.e. it takes several bus clock cycles
 * for the write to actually go through). The readl_relaxed() at the end will
 * block until the write is completed (and all registers updated), and thus
 * ensure that we don't access the PHY registers when they're in an
 * undetermined state.
 */
static inline void qcom_snps_hsphy_write_mask(void __iomem *base, u32 offset,
					      u32 mask, u32 val)
{
	u32 reg;

	reg = readl_relaxed(base + offset);

	reg &= ~mask;
	reg |= val & mask;
	writel_relaxed(reg, base + offset);

	/* Ensure above write is completed */
	readl_relaxed(base + offset);
}


static const struct override_param hs_disconnect_sc7280[] = {
	{ -272, 0 },
	{ 0, 1 },
	{ 317, 2 },
	{ 630, 3 },
	{ 973, 4 },
	{ 1332, 5 },
	{ 1743, 6 },
	{ 2156, 7 },
};

static const struct override_param squelch_det_threshold_sc7280[] = {
	{ -2090, 7 },
	{ -1560, 6 },
	{ -1030, 5 },
	{ -530, 4 },
	{ 0, 3 },
	{ 530, 2 },
	{ 1060, 1 },
	{ 1590, 0 },
};

static const struct override_param hs_amplitude_sc7280[] = {
	{ -660, 0 },
	{ -440, 1 },
	{ -220, 2 },
	{ 0, 3 },
	{ 230, 4 },
	{ 440, 5 },
	{ 650, 6 },
	{ 890, 7 },
	{ 1110, 8 },
	{ 1330, 9 },
	{ 1560, 10 },
	{ 1780, 11 },
	{ 2000, 12 },
	{ 2220, 13 },
	{ 2430, 14 },
	{ 2670, 15 },
};

static const struct override_param preemphasis_duration_sc7280[] = {
	{ 10000, 1 },
	{ 20000, 0 },
};

static const struct override_param preemphasis_amplitude_sc7280[] = {
	{ 10000, 1 },
	{ 20000, 2 },
	{ 30000, 3 },
	{ 40000, 0 },
};

static const struct override_param hs_rise_fall_time_sc7280[] = {
	{ -4100, 3 },
	{ 0, 2 },
	{ 2810, 1 },
	{ 5430, 0 },
};

static const struct override_param hs_crossover_voltage_sc7280[] = {
	{ -31000, 1 },
	{ 0, 3 },
	{ 28000, 2 },
};

static const struct override_param hs_output_impedance_sc7280[] = {
	{ -2300000, 3 },
	{ 0, 2 },
	{ 2600000, 1 },
	{ 6100000, 0 },
};

static const struct override_param ls_fs_output_impedance_sc7280[] = {
	{ -1053, 15 },
	{ -557, 7 },
	{ 0, 3 },
	{ 612, 1 },
	{ 1310, 0 },
};

static const struct override_param_map sc7280_snps_7nm_phy[] = {
	{
		"qcom,hs-disconnect-bp",
		hs_disconnect_sc7280,
		ARRAY_SIZE(hs_disconnect_sc7280),
		USB2_PHY_USB_PHY_HS_PHY_OVERRIDE_X0,
		HS_DISCONNECT_MASK
	},
	{
		"qcom,squelch-detector-bp",
		squelch_det_threshold_sc7280,
		ARRAY_SIZE(squelch_det_threshold_sc7280),
		USB2_PHY_USB_PHY_HS_PHY_OVERRIDE_X0,
		SQUELCH_DETECTOR_MASK
	},
	{
		"qcom,hs-amplitude-bp",
		hs_amplitude_sc7280,
		ARRAY_SIZE(hs_amplitude_sc7280),
		USB2_PHY_USB_PHY_HS_PHY_OVERRIDE_X1,
		HS_AMPLITUDE_MASK
	},
	{
		"qcom,pre-emphasis-duration-bp",
		preemphasis_duration_sc7280,
		ARRAY_SIZE(preemphasis_duration_sc7280),
		USB2_PHY_USB_PHY_HS_PHY_OVERRIDE_X1,
		PREEMPHASIS_DURATION_MASK,
	},
	{
		"qcom,pre-emphasis-amplitude-bp",
		preemphasis_amplitude_sc7280,
		ARRAY_SIZE(preemphasis_amplitude_sc7280),
		USB2_PHY_USB_PHY_HS_PHY_OVERRIDE_X1,
		PREEMPHASIS_AMPLITUDE_MASK,
	},
	{
		"qcom,hs-rise-fall-time-bp",
		hs_rise_fall_time_sc7280,
		ARRAY_SIZE(hs_rise_fall_time_sc7280),
		USB2_PHY_USB_PHY_HS_PHY_OVERRIDE_X2,
		HS_RISE_FALL_MASK
	},
	{
		"qcom,hs-crossover-voltage-microvolt",
		hs_crossover_voltage_sc7280,
		ARRAY_SIZE(hs_crossover_voltage_sc7280),
		USB2_PHY_USB_PHY_HS_PHY_OVERRIDE_X2,
		HS_CROSSOVER_VOLTAGE_MASK
	},
	{
		"qcom,hs-output-impedance-micro-ohms",
		hs_output_impedance_sc7280,
		ARRAY_SIZE(hs_output_impedance_sc7280),
		USB2_PHY_USB_PHY_HS_PHY_OVERRIDE_X2,
		HS_OUTPUT_IMPEDANCE_MASK,
	},
	{
		"qcom,ls-fs-output-impedance-bp",
		ls_fs_output_impedance_sc7280,
		ARRAY_SIZE(ls_fs_output_impedance_sc7280),
		USB2_PHY_USB_PHY_HS_PHY_OVERRIDE_X3,
		LS_FS_OUTPUT_IMPEDANCE_MASK,
	},
	{},
};

static int qcom_snps_hsphy_usb_init(struct phy *phy)
{
	struct qcom_snps_hsphy *priv = dev_get_priv(phy->dev);
	int i;

	qcom_snps_hsphy_write_mask(priv->base, USB2_PHY_USB_PHY_CFG0,
				   UTMI_PHY_CMN_CTRL_OVERRIDE_EN,
				   UTMI_PHY_CMN_CTRL_OVERRIDE_EN);
	qcom_snps_hsphy_write_mask(priv->base, USB2_PHY_USB_PHY_UTMI_CTRL5, POR,
				   POR);
	qcom_snps_hsphy_write_mask(priv->base,
				   USB2_PHY_USB_PHY_HS_PHY_CTRL_COMMON0, FSEL_MASK, 0);
	qcom_snps_hsphy_write_mask(priv->base,
				   USB2_PHY_USB_PHY_HS_PHY_CTRL_COMMON1,
				   PLLBTUNE, PLLBTUNE);
	qcom_snps_hsphy_write_mask(priv->base, USB2_PHY_USB_PHY_REFCLK_CTRL,
				   REFCLK_SEL_DEFAULT, REFCLK_SEL_MASK);
	qcom_snps_hsphy_write_mask(priv->base,
				   USB2_PHY_USB_PHY_HS_PHY_CTRL_COMMON1,
				   VBUSVLDEXTSEL0, VBUSVLDEXTSEL0);
	qcom_snps_hsphy_write_mask(priv->base, USB2_PHY_USB_PHY_HS_PHY_CTRL1,
				   VBUSVLDEXT0, VBUSVLDEXT0);

	for (i = 0; i < ARRAY_SIZE(priv->update_seq_cfg); i++) {
		if (priv->update_seq_cfg[i].need_update)
			qcom_snps_hsphy_write_mask(priv->base,
					priv->update_seq_cfg[i].offset,
					priv->update_seq_cfg[i].mask,
					priv->update_seq_cfg[i].value);
	}

	qcom_snps_hsphy_write_mask(priv->base,
				   USB2_PHY_USB_PHY_HS_PHY_CTRL_COMMON2,
				   VREGBYPASS, VREGBYPASS);

	qcom_snps_hsphy_write_mask(priv->base, USB2_PHY_USB_PHY_HS_PHY_CTRL2,
				   USB2_SUSPEND_N_SEL | USB2_SUSPEND_N,
				   USB2_SUSPEND_N_SEL | USB2_SUSPEND_N);

	qcom_snps_hsphy_write_mask(priv->base, USB2_PHY_USB_PHY_UTMI_CTRL0,
				   SLEEPM, SLEEPM);

	qcom_snps_hsphy_write_mask(
		priv->base, USB2_PHY_USB_PHY_HS_PHY_CTRL_COMMON0, SIDDQ, 0);

	qcom_snps_hsphy_write_mask(priv->base, USB2_PHY_USB_PHY_UTMI_CTRL5, POR,
				   0);

	qcom_snps_hsphy_write_mask(priv->base, USB2_PHY_USB_PHY_HS_PHY_CTRL2,
				   USB2_SUSPEND_N_SEL, 0);

	qcom_snps_hsphy_write_mask(priv->base, USB2_PHY_USB_PHY_CFG0,
				   UTMI_PHY_CMN_CTRL_OVERRIDE_EN, 0);

	return 0;
}

static int qcom_snps_hsphy_power_on(struct phy *phy)
{
	struct qcom_snps_hsphy *priv = dev_get_priv(phy->dev);
	int ret;

	ret = reset_deassert_bulk(&priv->resets);
	if (ret)
		return ret;

	ret = qcom_snps_hsphy_usb_init(phy);
	if (ret)
		return ret;

	return 0;
}

static int qcom_snps_hsphy_power_off(struct phy *phy)
{
	struct qcom_snps_hsphy *priv = dev_get_priv(phy->dev);

	reset_assert_bulk(&priv->resets);

	return 0;
}


static void qcom_snps_hsphy_override_param_update_val(
			const struct override_param_map map,
			s32 dt_val, struct phy_override_seq *seq_entry)
{
	int i;

	/*
	 * Param table for each param is in increasing order
	 * of dt values. We need to iterate over the list to
	 * select the entry that matches the dt value and pick
	 * up the corresponding register value.
	 */
	for (i = 0; i < map.table_size - 1; i++) {
		if (map.param_table[i].value == dt_val)
			break;
	}

	seq_entry->need_update = true;
	seq_entry->offset = map.reg_offset;
	seq_entry->mask = map.param_mask;
	seq_entry->value = map.param_table[i].reg_val << __ffs(map.param_mask);
}

static void qcom_snps_hsphy_read_override_param_seq(struct udevice *dev)
{
	ofnode node = dev_ofnode(dev);
	s32 val;
	int ret, i;
	struct qcom_snps_hsphy *hsphy;
	const struct override_param_map *cfg = (struct override_param_map *)dev_get_driver_data(dev);

	if (!cfg)
		return;

	hsphy = dev_get_priv(dev);

	for (i = 0; cfg[i].prop_name != NULL; i++) {
		ret = ofnode_read_s32(node, cfg[i].prop_name, &val);
		if (ret)
			continue;

		qcom_snps_hsphy_override_param_update_val(cfg[i], val,
					&hsphy->update_seq_cfg[i]);
		debug("%s: Read param: %s dt_val: %d reg_val: 0x%x\n",
			dev->name,
			cfg[i].prop_name, val, hsphy->update_seq_cfg[i].value);

	}
}

static int qcom_snps_hsphy_phy_probe(struct udevice *dev)
{
	struct qcom_snps_hsphy *priv = dev_get_priv(dev);
	int ret;

	priv->base = dev_read_addr_ptr(dev);
	if (IS_ERR(priv->base))
		return PTR_ERR(priv->base);

	qcom_snps_hsphy_read_override_param_seq(dev);

	ret = reset_get_bulk(dev, &priv->resets);
	if (ret < 0) {
		printf("failed to get resets, ret = %d\n", ret);
		return ret;
	}

	reset_deassert_bulk(&priv->resets);

	return 0;
}

static struct phy_ops qcom_snps_hsphy_phy_ops = {
	.power_on = qcom_snps_hsphy_power_on,
	.power_off = qcom_snps_hsphy_power_off,
};

static const struct udevice_id qcom_snps_hsphy_phy_ids[] = {
	{ .compatible = "qcom,sm8150-usb-hs-phy" },
	{ .compatible = "qcom,usb-snps-hs-5nm-phy" },
	{ .compatible = "qcom,usb-snps-hs-7nm-phy",
		.data = (ulong)&sc7280_snps_7nm_phy, },
	{ .compatible = "qcom,usb-snps-femto-v2-phy" },
	{ .compatible = "qcom,usb-hsphy-snps-femto" },
	{}
};

U_BOOT_DRIVER(qcom_usb_qcom_snps_hsphy) = {
	.name = "qcom-snps-hsphy",
	.id = UCLASS_PHY,
	.of_match = qcom_snps_hsphy_phy_ids,
	.ops = &qcom_snps_hsphy_phy_ops,
	.probe = qcom_snps_hsphy_phy_probe,
	.priv_auto = sizeof(struct qcom_snps_hsphy),
};
