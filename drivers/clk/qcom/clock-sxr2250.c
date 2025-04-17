// SPDX-License-Identifier: BSD-3-Clause
/*
 * Clock drivers for Qualcomm sm8250
 *
 * (C) Copyright 2024 Linaro Ltd.
 */
// based on 8250

#include <clk-uclass.h>
#include <dm.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <linux/bug.h>
#include <linux/bitops.h>
#include <dt-bindings/clock/qcom,gcc-anorak.h>

#include "clock-qcom.h"

#define GCC_SDCC2_APPS_CLK_SRC_REG 0x2400c // DONE

#define APCS_GPLL9_STATUS 0x9000 // DONE
#define APCS_GPLLX_ENA_REG 0x62018 // DONE

#define USB30_PRIM_MASTER_CLK_CMD_RCGR 0x49028 // DONE
#define USB30_PRIM_MOCK_UTMI_CLK_CMD_RCGR 0x49040 // DONE
#define USB3_PRIM_PHY_AUX_CMD_RCGR 0x4906c // DONE

#define GCC_QUPV3_WRAP0_S6_RCG_REG (0x27754) // DONE
#define GCC_UFS_PHY_AXI_RCG_REG (0x8702c)
#define GCC_UFS_PHY_ICE_CORE_RCG_REG (0x87074)
#define GCC_UFS_PHY_UNIPRO_CORE_RCG_REG (0x8708c)

// DONE
static const struct freq_tbl ftbl_gcc_qupv3_wrap0_s2_clk_src[] = {
    F(7372800, CFG_CLK_SRC_GPLL0_EVEN, 1, 384, 15625),
    F(14745600, CFG_CLK_SRC_GPLL0_EVEN, 1, 768, 15625),
    F(19200000, CFG_CLK_SRC_CXO, 1, 0, 0),
    F(29491200, CFG_CLK_SRC_GPLL0_EVEN, 1, 1536, 15625),
    F(32000000, CFG_CLK_SRC_GPLL0_EVEN, 1, 8, 75),
    F(48000000, CFG_CLK_SRC_GPLL0_EVEN, 1, 4, 25),
    F(50000000, CFG_CLK_SRC_GPLL0_EVEN, 6, 0, 0),
    F(64000000, CFG_CLK_SRC_GPLL0_EVEN, 1, 16, 75),
    F(75000000, CFG_CLK_SRC_GPLL0_EVEN, 4, 0, 0),
    F(80000000, CFG_CLK_SRC_GPLL0_EVEN, 1, 4, 15),
    F(96000000, CFG_CLK_SRC_GPLL0_EVEN, 1, 8, 25),
    F(100000000, CFG_CLK_SRC_GPLL0, 6, 0, 0),
    {}
};

// DONE
static const struct freq_tbl ftbl_gcc_sdcc2_apps_clk_src[] = {
    F(400000, CFG_CLK_SRC_CXO, 12, 1, 4),
    F(25000000, CFG_CLK_SRC_GPLL0_EVEN, 12, 0, 0),
    F(50000000, CFG_CLK_SRC_GPLL0_EVEN, 6, 0, 0),
    F(100000000, CFG_CLK_SRC_GPLL0, 3, 0, 0),
    F(202000000, CFG_CLK_SRC_GPLL9, 4, 0, 0),
    {}
};

static const struct freq_tbl ftbl_gcc_ufs_phy_axi_clk_src[] = {
    F(25000000, CFG_CLK_SRC_GPLL0_EVEN, 12, 0, 0),
    F(75000000, CFG_CLK_SRC_GPLL0_EVEN, 4, 0, 0),
    F(150000000, CFG_CLK_SRC_GPLL0, 4, 0, 0),
    F(300000000, CFG_CLK_SRC_GPLL0, 2, 0, 0),
    { }
};

static const struct freq_tbl ftbl_gcc_ufs_phy_ice_core_clk_src[] = {
    F(100000000, CFG_CLK_SRC_GPLL0_EVEN, 3, 0, 0),
    F(201500000, CFG_CLK_SRC_GPLL0, 4, 0, 0),
    F(403000000, CFG_CLK_SRC_GPLL0, 2, 0, 0),
    { }
};

static const struct freq_tbl ftbl_gcc_ufs_phy_unipro_core_clk_src[] = {
    F(75000000, CFG_CLK_SRC_GPLL0_EVEN, 4, 0, 0),
    F(150000000, CFG_CLK_SRC_GPLL0, 4, 0, 0),
    F(300000000, CFG_CLK_SRC_GPLL0, 2, 0, 0),
    { }
};

// DONE
static struct pll_vote_clk gpll9_vote_clk = {
    .status = APCS_GPLL9_STATUS,
    .status_bit = BIT(31),
    .ena_vote = APCS_GPLLX_ENA_REG,
    .vote_bit = BIT(9),
};

static ulong sxr2250_set_rate(struct clk *clk, ulong rate)
{
    struct msm_clk_priv *priv = dev_get_priv(clk->dev);
    const struct freq_tbl *freq;

    if (clk->id < priv->data->num_clks)
        printf("%s: %s, requested rate=%ld\n", __func__,
              priv->data->clks[clk->id].name, rate);

    /*
     * Basically just scaffolding for .cmd_rcgr in the Linux gcc code.
     */
    switch (clk->id) {
    case GCC_UFS_PHY_AXI_CLK:
        freq = qcom_find_freq(ftbl_gcc_ufs_phy_axi_clk_src, rate);
        clk_rcg_set_rate_mnd(priv->base, GCC_UFS_PHY_AXI_RCG_REG,
                     freq->pre_div, freq->m, freq->n, freq->src,
                     16);
        return freq->freq;

    case GCC_UFS_PHY_ICE_CORE_CLK:
        freq = qcom_find_freq(ftbl_gcc_ufs_phy_ice_core_clk_src, rate);
        clk_rcg_set_rate_mnd(priv->base, GCC_UFS_PHY_ICE_CORE_RCG_REG,
                     freq->pre_div, freq->m, freq->n, freq->src,
                     16);
        return freq->freq;

    case GCC_UFS_PHY_UNIPRO_CORE_CLK:
        freq = qcom_find_freq(ftbl_gcc_ufs_phy_unipro_core_clk_src, rate);
        clk_rcg_set_rate_mnd(priv->base, GCC_UFS_PHY_UNIPRO_CORE_RCG_REG,
                     freq->pre_div, freq->m, freq->n, freq->src,
                     16);
        return freq->freq;

    case GCC_QUPV3_WRAP1_S6_CLK: /* debug uart */
        freq = qcom_find_freq(ftbl_gcc_qupv3_wrap0_s2_clk_src, rate);
        clk_rcg_set_rate_mnd(priv->base, GCC_QUPV3_WRAP0_S6_RCG_REG,
                     freq->pre_div, freq->m, freq->n, freq->src,
                     16);
        return freq->freq;

    case GCC_SDCC2_APPS_CLK:
        /* Enable GPLL9 so that we can point SDCC2_APPS_CLK_SRC at it */
        clk_enable_gpll0(priv->base, &gpll9_vote_clk);
        freq = qcom_find_freq(ftbl_gcc_sdcc2_apps_clk_src, rate);
        WARN(freq->src != CFG_CLK_SRC_GPLL9,
             "SDCC2_APPS_CLK_SRC not set to GPLL9, requested rate %lu\n",
             rate);
        clk_rcg_set_rate_mnd(priv->base, GCC_SDCC2_APPS_CLK_SRC_REG,
                     freq->pre_div, freq->m, freq->n,
                     CFG_CLK_SRC_GPLL9, 8);
        return rate;
    default:
        return 0;
    }
}

// WIP
static const struct gate_clk sxr2250_clks[] = {
    //GCC_GPLL0
    //GCC_GPLL0_OUT_EVEN
    //GCC_GPLL4
    //GCC_GPLL9
    //GCC_AGGRE_NOC_PCIE_AXI_CLK
    //GCC_AGGRE_NOC_PCIE_SF_AXI_CLK
    GATE_CLK(GCC_AGGRE_UFS_PHY_AXI_CLK, 0x870d4, BIT(0)),
    GATE_CLK(GCC_AGGRE_USB3_PRIM_AXI_CLK, 0x49088, BIT(0)),
    GATE_CLK(GCC_BOOT_ROM_AHB_CLK, 0x48004, BIT(10)),
    GATE_CLK(GCC_CAMERA_AHB_CLK, 0x36004, BIT(0)),
    //GCC_CAMERA_HF_AXI_CLK
    //GCC_CAMERA_SF_AXI_CLK
    GATE_CLK(GCC_CAMERA_XO_CLK, 0x3601C, BIT(0)),
    //GCC_CFG_NOC_PCIE_ANOC_AHB_CLK
    GATE_CLK(GCC_CFG_NOC_USB3_PRIM_AXI_CLK, 0x49084, BIT(0)),
    //GCC_DDRSS_GPU_AXI_CLK
    //GCC_DDRSS_PCIE_SF_TBU_CLK
    GATE_CLK(GCC_DISP1_AHB_CLK, 0x2E004, BIT(0)),
    //GCC_DISP1_HF_AXI_CLK
    GATE_CLK(GCC_DISP_AHB_CLK, 0x37004, BIT(0)),
    //GCC_DISP_HF_AXI_CLK
    // GP1 ... GP11
    GATE_CLK(GCC_GPU_CFG_AHB_CLK, 0x81004, BIT(0)),
    //GCC_GPU_GPLL0_CLK_SRC
    //GCC_GPU_GPLL0_DIV_CLK_SRC
    //GCC_GPU_MEMNOC_GFX_CLK
    //GCC_GPU_SNOC_DVM_GFX_CLK
    // PCIE0 ... PCIE2
    //GCC_PDM2_CLK
    //GCC_PDM2_CLK_SRC
    //GCC_PDM_AHB_CLK
    //GCC_PDM_XO4_CLK
    //GCC_PWM0_XO512_CLK
    GATE_CLK(GCC_QMIP_CAMERA_NRT_AHB_CLK, 0x36008, BIT(0)),
    GATE_CLK(GCC_QMIP_CAMERA_RT_AHB_CLK, 0x3600c, BIT(0)),
    //GATE_CLK(GCC_QMIP_DISP_AHB_CLK, 0x37004, BIT(0)), // always on
    GATE_CLK(GCC_QMIP_VIDEO_CVP_AHB_CLK, 0x42008, BIT(0)),
    GATE_CLK(GCC_QMIP_VIDEO_VCODEC_AHB_CLK, 0x4200c, BIT(0)),
    GATE_CLK(GCC_QUPV3_WRAP0_CORE_2X_CLK, 0x62008, BIT(9)),
    GATE_CLK(GCC_QUPV3_WRAP0_CORE_CLK, 0x62008, BIT(8)),
    GATE_CLK(GCC_QUPV3_WRAP0_S0_CLK, 0x62008, BIT(10)),
    GATE_CLK(GCC_QUPV3_WRAP0_S1_CLK, 0x62008, BIT(11)),
    GATE_CLK(GCC_QUPV3_WRAP0_S2_CLK, 0x62008, BIT(12)),
    GATE_CLK(GCC_QUPV3_WRAP0_S3_CLK, 0x62008, BIT(13)),
    GATE_CLK(GCC_QUPV3_WRAP0_S4_CLK, 0x62008, BIT(14)),
    GATE_CLK(GCC_QUPV3_WRAP0_S5_CLK, 0x62008, BIT(15)),
    GATE_CLK(GCC_QUPV3_WRAP0_S6_CLK, 0x62008, BIT(16)),
    GATE_CLK(GCC_QUPV3_WRAP1_CORE_2X_CLK, 0x62008, BIT(18)),
    GATE_CLK(GCC_QUPV3_WRAP1_CORE_CLK, 0x62008, BIT(19)),
    GATE_CLK(GCC_QUPV3_WRAP1_S0_CLK, 0x62008, BIT(22)),
    GATE_CLK(GCC_QUPV3_WRAP1_S1_CLK, 0x62008, BIT(23)),
    GATE_CLK(GCC_QUPV3_WRAP1_S2_CLK, 0x62008, BIT(24)),
    GATE_CLK(GCC_QUPV3_WRAP1_S3_CLK, 0x62008, BIT(25)),
    GATE_CLK(GCC_QUPV3_WRAP1_S4_CLK, 0x62008, BIT(26)),
    GATE_CLK(GCC_QUPV3_WRAP1_S5_CLK, 0x62008, BIT(27)),
    GATE_CLK(GCC_QUPV3_WRAP_0_M_AHB_CLK, 0x62008, BIT(6)),
    GATE_CLK(GCC_QUPV3_WRAP_0_S_AHB_CLK, 0x62008, BIT(7)),
    GATE_CLK(GCC_QUPV3_WRAP_1_M_AHB_CLK, 0x62008, BIT(20)),
    GATE_CLK(GCC_QUPV3_WRAP_1_S_AHB_CLK, 0x62008, BIT(21)),
    
    GATE_CLK(GCC_SDCC2_AHB_CLK, 0x2400c, BIT(0)),
    GATE_CLK(GCC_SDCC2_APPS_CLK, 0x24004, BIT(0)),
    //GCC_SDCC2_APPS_CLK_SRC

    GATE_CLK(GCC_UFS_0_CLKREF_EN, 0x9c000, BIT(0)),
    GATE_CLK(GCC_UFS_PHY_AHB_CLK, 0x87020, BIT(0)),
    GATE_CLK(GCC_UFS_PHY_AXI_CLK, 0x87018, BIT(0)),
    //GATE_CLK(GCC_UFS_PHY_AXI_CLK_SRC, 0x8702c, BIT(0)), // ?
    GATE_CLK(GCC_UFS_PHY_ICE_CORE_CLK, 0x8706c, BIT(0)),
    //GCC_UFS_PHY_ICE_CORE_CLK_SRC
    GATE_CLK(GCC_UFS_PHY_PHY_AUX_CLK, 0x870a4, BIT(0)),
    //GCC_UFS_PHY_PHY_AUX_CLK_SRC
    GATE_CLK(GCC_UFS_PHY_RX_SYMBOL_0_CLK, 0x87028, BIT(0)),
    //GCC_UFS_PHY_RX_SYMBOL_0_CLK_SRC
    GATE_CLK(GCC_UFS_PHY_RX_SYMBOL_1_CLK, 0x870c0, BIT(0)),
    //GCC_UFS_PHY_RX_SYMBOL_1_CLK_SRC
    GATE_CLK(GCC_UFS_PHY_TX_SYMBOL_0_CLK, 0x87024, BIT(0)),
    //GCC_UFS_PHY_TX_SYMBOL_0_CLK_SRC
    GATE_CLK(GCC_UFS_PHY_UNIPRO_CORE_CLK, 0x87064, BIT(0)),
    //GCC_UFS_PHY_UNIPRO_CORE_CLK_SRC
    //GCC_USB2_0_CLKREF_EN
    GATE_CLK(GCC_USB30_PRIM_MASTER_CLK, 0x49018, BIT(0)),
    //GCC_USB30_PRIM_MASTER_CLK_SRC
    GATE_CLK(GCC_USB30_PRIM_MOCK_UTMI_CLK, 0x49024, BIT(0)),
    //GCC_USB30_PRIM_MOCK_UTMI_CLK_SRC
    //GCC_USB30_PRIM_MOCK_UTMI_POSTDIV_CLK_SRC
    GATE_CLK(GCC_USB30_PRIM_SLEEP_CLK, 0x49020, BIT(0)),
    //GCC_USB3_0_CLKREF_EN
    GATE_CLK(GCC_USB3_PRIM_PHY_AUX_CLK, 0x4905c, BIT(0)),
    //GCC_USB3_PRIM_PHY_AUX_CLK_SRC
    GATE_CLK(GCC_USB3_PRIM_PHY_COM_AUX_CLK, 0x49060, BIT(0)),
    GATE_CLK(GCC_USB3_PRIM_PHY_PIPE_CLK, 0x49064, BIT(0)),
    //GCC_USB3_PRIM_PHY_PIPE_CLK_SRC
    GATE_CLK(GCC_VIDEO_AHB_CLK, 0x42004, BIT(0)),
    //GCC_VIDEO_AXI0_CLK
    //GCC_VIDEO_AXI1_CLK
    GATE_CLK(GCC_VIDEO_XO_CLK, 0x42028, BIT(0)),
    GATE_CLK(GCC_AGGRE_UFS_PHY_AXI_HW_CTL_CLK, 0x870d4, BIT(1)),
    GATE_CLK(GCC_UFS_PHY_AXI_HW_CTL_CLK, 0x87018, BIT(1)),
    GATE_CLK(GCC_UFS_PHY_ICE_CORE_HW_CTL_CLK, 0x8706c, BIT(1)),
    GATE_CLK(GCC_UFS_PHY_PHY_AUX_HW_CTL_CLK, 0x870a4, BIT(0)),
    GATE_CLK(GCC_UFS_PHY_UNIPRO_CORE_HW_CTL_CLK, 0x870a4, BIT(1)),
    //GCC_EDP_0_CLKREF_EN
    //GCC_EDP_1_CLKREF_EN
    //GCC_HLOS1_VOTE_AGGRE_NOC_MMU_AUDIO_TBU_CLK
    //GCC_HLOS1_VOTE_AGGRE_NOC_MMU_PCIE_TBU_CLK
    //GCC_HLOS1_VOTE_AGGRE_NOC_MMU_TBU1_CLK     
    //GCC_HLOS1_VOTE_AGGRE_NOC_MMU_TBU2_CLK     
    //GCC_HLOS1_VOTE_MMNOC_MMU_TBU_HF0_CLK      
    //GCC_HLOS1_VOTE_MMNOC_MMU_TBU_HF1_CLK      
    //GCC_HLOS1_VOTE_MMNOC_MMU_TBU_HF2_CLK      
    //GCC_HLOS1_VOTE_MMNOC_MMU_TBU_HF3_CLK      
    //GCC_HLOS1_VOTE_MMNOC_MMU_TBU_HF4_CLK      
    //GCC_HLOS1_VOTE_MMNOC_MMU_TBU_HF5_CLK      
    //GCC_HLOS1_VOTE_MMNOC_MMU_TBU_SF0_CLK      
    //GCC_HLOS1_VOTE_MMNOC_MMU_TBU_SF1_CLK      
    //GCC_HLOS1_VOTE_MMU_TCU_CLK        
    //GCC_HLOS1_VOTE_TURING_MMU_TBU0_CLK    
    //GCC_HLOS1_VOTE_TURING_MMU_TBU1_CLK    
    //GCC_PWM0_XO512_DIV_CLK_SRC        
};

static int sxr2250_enable(struct clk *clk)
{
    struct msm_clk_priv *priv = dev_get_priv(clk->dev);

    if (priv->data->num_clks < clk->id) {
        debug("%s: unknown clk id %lu\n", __func__, clk->id);
        return 0;
    }

    debug("%s: clk %s\n", __func__, sxr2250_clks[clk->id].name);

    switch (clk->id) {
    case GCC_USB30_PRIM_MASTER_CLK:
        qcom_gate_clk_en(priv, GCC_USB3_PRIM_PHY_AUX_CLK);
        qcom_gate_clk_en(priv, GCC_USB3_PRIM_PHY_COM_AUX_CLK);
        break;
    }

    qcom_gate_clk_en(priv, clk->id);

    return 0;
}

static int sxr2250_probe(struct udevice *dev) {
    //struct msm_clk_data *data = (struct msm_clk_data *)dev_get_driver_data(dev);
    //struct msm_clk_priv *priv = dev_get_priv(dev);

    /*
     * From Linux? TODO?
     * Can't actually uncomment, it needs the parent clock-qcom device.
     *
     * Keep the clocks always-ON
     * GCC_CAMERA_AHB_CLK, GCC_CAMERA_XO_CLK, GCC_DISP_AHB_CLK
     * GCC_VIDEO_AHB_CLK, GCC_VIDEO_XO_CLK, GCC_GPU_CFG_AHB_CLK
     * GCC_DISP1_AHB_CLK
     */
    /*qcom_gate_clk_en(priv, GCC_CAMERA_AHB_CLK);
    qcom_gate_clk_en(priv, GCC_CAMERA_XO_CLK);
    qcom_gate_clk_en(priv, GCC_DISP_AHB_CLK);
    qcom_gate_clk_en(priv, GCC_VIDEO_AHB_CLK);
    qcom_gate_clk_en(priv, GCC_VIDEO_XO_CLK);
    qcom_gate_clk_en(priv, GCC_GPU_CFG_AHB_CLK);
    qcom_gate_clk_en(priv, GCC_DISP1_AHB_CLK);*/

    return 0;
}

// DONE
static const struct qcom_reset_map sxr2250_gcc_resets[] = {
    [GCC_CAMERA_BCR] = { 0x36000 },
    [GCC_DISPLAY1_BCR] = { 0x2e000 },
    [GCC_DISPLAY_BCR] = { 0x37000 },
    [GCC_GPU_BCR] = { 0x81000 },
    [GCC_PCIE_0_BCR] = { 0x7b000 },
    [GCC_PCIE_0_LINK_DOWN_BCR] = { 0x7c014 },
    [GCC_PCIE_0_NOCSR_COM_PHY_BCR] = { 0x7c020 },
    [GCC_PCIE_0_PHY_BCR] = { 0x7c01c },
    [GCC_PCIE_0_PHY_NOCSR_COM_PHY_BCR] = { 0x7c028 },
    [GCC_PCIE_1_BCR] = { 0xad000 },
    [GCC_PCIE_1_LINK_DOWN_BCR] = { 0xae014 },
    [GCC_PCIE_1_NOCSR_COM_PHY_BCR] = { 0xae020 },
    [GCC_PCIE_1_PHY_BCR] = { 0xae01c },
    [GCC_PCIE_1_PHY_NOCSR_COM_PHY_BCR] = { 0xae028 },
    [GCC_PCIE_2_BCR] = { 0x9d000 },
    [GCC_PCIE_2_LINK_DOWN_BCR] = { 0x9e014 },
    [GCC_PCIE_2_NOCSR_COM_PHY_BCR] = { 0x9e020 },
    [GCC_PCIE_2_PHY_BCR] = { 0x9e01c },
    [GCC_PCIE_2_PHY_NOCSR_COM_PHY_BCR] = { 0x9e000 },
    [GCC_PCIE_PHY_BCR] = { 0x7f000 },
    [GCC_PCIE_PHY_CFG_AHB_BCR] = { 0x7f00c },
    [GCC_PCIE_PHY_COM_BCR] = { 0x7f010 },
    [GCC_PDM_BCR] = { 0x43000 },
    [GCC_QUPV3_WRAPPER_0_BCR] = { 0x27000 },
    [GCC_QUPV3_WRAPPER_1_BCR] = { 0x28000 },
    [GCC_QUSB2PHY_PRIM_BCR] = { 0x22000 },
    [GCC_QUSB2PHY_SEC_BCR] = { 0x22004 },
    [GCC_SDCC2_BCR] = { 0x24000 },
    [GCC_UFS_PHY_BCR] = { 0x87000 },
    [GCC_USB30_PRIM_BCR] = { 0x49000 },
    [GCC_USB3_DP_PHY_PRIM_BCR] = { 0x60008 },
    [GCC_USB3_DP_PHY_SEC_BCR] = { 0x60014 },
    [GCC_USB3_PHY_PRIM_BCR] = { 0x60000 },
    [GCC_USB3_PHY_SEC_BCR] = { 0x6000c },
    [GCC_USB3PHY_PHY_PRIM_BCR] = { 0x60004 },
    [GCC_USB3PHY_PHY_SEC_BCR] = { 0x60010 },
    [GCC_USB_PHY_CFG_AHB2PHY_BCR] = { 0x7a000 },
    [GCC_VIDEO_AXI0_CLK_ARES] = { 0x42018, 2 },
    [GCC_VIDEO_AXI1_CLK_ARES] = { 0x42020, 2 },
    [GCC_VIDEO_BCR] = { 0x42000 },
};

// diwali-gdsc.dtsi
// GOOD
static const struct qcom_power_map sxr2250_gdscs[] = {
    [GCC_PCIE_0_GDSC] = { 0x7b000 },    // good?
    //[GCC_PCIE_0_PHY_GDSC] = { 0x7c000 },
    //[GCC_PCIE_1_GDSC] = { 0x9d004 },
    //[GCC_PCIE_1_PHY_GDSC] = { 0x9e000 },
    [GCC_UFS_PHY_GDSC] = { 0x87000 },    // good?
    [GCC_USB30_PRIM_GDSC] = { 0x49000 }, // good?
    //[GCC_USB3_PHY_GDSC] = { 0x60018 },
};

// IDK
static const phys_addr_t sxr2250_gpll_addrs[] = {
    0x00100000, // GCC_GPLL0_MODE
    //0x00101000, // GCC_GPLL1_MODE
    //0x00102000, // GCC_GPLL2_MODE
    //0x00103000, // GCC_GPLL3_MODE
    0x00104000, // GCC_GPLL4_MODE
    //0x00174000, // GCC_GPLL5_MODE
    //0x00113000, // GCC_GPLL6_MODE
    //0x0011a000, // GCC_GPLL7_MODE
    //0x0011b000, // GCC_GPLL8_MODE
    0x00109000, // GCC_GPLL9_MODE
    //0x0011d000, // GCC_GPLL10_MODE
    //0x0014a000, // GCC_GPLL11_MODE
};

// .cmd_rcgr
// WIP
static const phys_addr_t sxr2250_rcg_addrs[] = {
    0x00149028, // GCC_USB30_PRIM_MASTER_CMD_RCGR
    0x00149040, // GCC_USB30_PRIM_MOCK_UTMI_CMD_RCGR
    0x0014906c, // GCC_USB3_PRIM_PHY_AUX_CMD_RCGR
    0x00124014, // GCC_SDCC2_APPS_CMD_RCGR
    //0x0012300c, // GCC_QUPV3_WRAP0_CORE_2X_CMD_RCGR
    0x00127014, // GCC_QUPV3_WRAP0_S0_CMD_RCGR
    0x00127148, // GCC_QUPV3_WRAP0_S1_CMD_RCGR
    0x0012727c, // GCC_QUPV3_WRAP0_S2_CMD_RCGR
    0x001273b0, // GCC_QUPV3_WRAP0_S3_CMD_RCGR
    0x001274e4, // GCC_QUPV3_WRAP0_S4_CMD_RCGR
    0x00127620, // GCC_QUPV3_WRAP0_S5_CMD_RCGR
    0x00127754, // GCC_QUPV3_WRAP0_S6_CMD_RCGR
    //0x00123144, // GCC_QUPV3_WRAP1_CORE_2X_CMD_RCGR
    0x00128014, // GCC_QUPV3_WRAP1_S0_CMD_RCGR
    0x00128148, // GCC_QUPV3_WRAP1_S1_CMD_RCGR
    0x0012827c, // GCC_QUPV3_WRAP1_S2_CMD_RCGR
    0x001283b0, // GCC_QUPV3_WRAP1_S3_CMD_RCGR
    0x001284e4, // GCC_QUPV3_WRAP1_S4_CMD_RCGR
    0x00128620, // GCC_QUPV3_WRAP1_S5_CMD_RCGR
    0x00128754, // GCC_QUPV3_WRAP1_S6_CMD_RCGR
    0x0017b070, // GCC_PCIE_0_AUX_CMD_RCGR
    0x0017b050, // GCC_PCIE_0_PHY_RCHNG_CMD_RCGR
    0x001ad06c, // GCC_PCIE_1_AUX_CMD_RCGR
    0x001ad04c, // GCC_PCIE_1_PHY_RCHNG_CMD_RCGR
    0x0018702c, // GCC_UFS_PHY_AXI_CMD_RCGR
    0x00187074, // GCC_UFS_PHY_ICE_CORE_CMD_RCGR
    0x0018708c, // GCC_UFS_PHY_UNIPRO_CORE_CMD_RCGR
    0x001870a8, // GCC_UFS_PHY_PHY_AUX_CMD_RCGR
    //0x0010d00c, // GCC_RBCPR_MMCX_CMD_RCGR
    //0x00106038, // GCC_PCIE_2_AUX_CMD_RCGR
};

// WIP
static const char *const sxr2250_rcg_names[] = {
    "GCC_USB30_PRIM_MASTER_CMD_RCGR",
    "GCC_USB30_PRIM_MOCK_UTMI_CMD_RCGR",
    "GCC_USB3_PRIM_PHY_AUX_CMD_RCGR",
    "GCC_SDCC2_APPS_CMD_RCGR",
    //"GCC_QUPV3_WRAP0_CORE_2X_CMD_RCGR",
    "GCC_QUPV3_WRAP0_S0_CMD_RCGR",
    "GCC_QUPV3_WRAP0_S1_CMD_RCGR",
    "GCC_QUPV3_WRAP0_S2_CMD_RCGR",
    "GCC_QUPV3_WRAP0_S3_CMD_RCGR",
    "GCC_QUPV3_WRAP0_S4_CMD_RCGR",
    "GCC_QUPV3_WRAP0_S5_CMD_RCGR",
    "GCC_QUPV3_WRAP0_S6_CMD_RCGR",
    //"GCC_QUPV3_WRAP1_CORE_2X_CMD_RCGR",
    "GCC_QUPV3_WRAP1_S0_CMD_RCGR",
    "GCC_QUPV3_WRAP1_S1_CMD_RCGR",
    "GCC_QUPV3_WRAP1_S2_CMD_RCGR",
    "GCC_QUPV3_WRAP1_S3_CMD_RCGR",
    "GCC_QUPV3_WRAP1_S4_CMD_RCGR",
    "GCC_QUPV3_WRAP1_S5_CMD_RCGR",
    "GCC_QUPV3_WRAP1_S6_CMD_RCGR",
    "GCC_PCIE_0_AUX_CMD_RCGR",
    "GCC_PCIE_0_PHY_RCHNG_CMD_RCGR",
    "GCC_PCIE_1_AUX_CMD_RCGR",
    "GCC_PCIE_1_PHY_RCHNG_CMD_RCGR",
    "GCC_UFS_PHY_AXI_CMD_RCGR",
    "GCC_UFS_PHY_ICE_CORE_CMD_RCGR",
    "GCC_UFS_PHY_UNIPRO_CORE_CMD_RCGR",
    "GCC_UFS_PHY_PHY_AUX_CMD_RCGR",
    //"GCC_RBCPR_MMCX_CMD_RCGR",
    //"GCC_PCIE_2_AUX_CMD_RCGR",
};

static struct msm_clk_data sxr2250_gcc_data = {
    .resets = sxr2250_gcc_resets,
    .num_resets = ARRAY_SIZE(sxr2250_gcc_resets),
    .clks = sxr2250_clks,
    .num_clks = ARRAY_SIZE(sxr2250_clks),
    .power_domains = sxr2250_gdscs,
    .num_power_domains = ARRAY_SIZE(sxr2250_gdscs),

    .enable = sxr2250_enable,
    .set_rate = sxr2250_set_rate,

    .dbg_pll_addrs = sxr2250_gpll_addrs,
    .num_plls = ARRAY_SIZE(sxr2250_gpll_addrs),
    .dbg_rcg_addrs = sxr2250_rcg_addrs,
    .num_rcgs = ARRAY_SIZE(sxr2250_rcg_addrs),
    .dbg_rcg_names = sxr2250_rcg_names,
};

// Spray and pray on .compatible
static const struct udevice_id gcc_sxr2250_of_match[] = {
    {
        .compatible = "qcom,gcc-sxr2230",
        .data = (ulong)&sxr2250_gcc_data,
    },
    {
        .compatible = "qcom,gcc-sxr2250",
        .data = (ulong)&sxr2250_gcc_data,
    },
    {
        .compatible = "qcom,gcc-anorak",
        .data = (ulong)&sxr2250_gcc_data,
    },
    {
        .compatible = "qcom,anorak-gcc",
        .data = (ulong)&sxr2250_gcc_data,
    },
    {}
};

U_BOOT_DRIVER(gcc_sxr2250) = {
    .name = "gcc_sxr2250",
    .id = UCLASS_NOP,
    .of_match = gcc_sxr2250_of_match,
    .bind = qcom_cc_bind,
    .flags = DM_FLAG_PRE_RELOC,
    .probe = sxr2250_probe,
};
