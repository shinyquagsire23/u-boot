#ifndef _PINMUX_CONFIG_HAC_001_H_
#define _PINMUX_CONFIG_HAC_001_H_

#define GPIO_INIT(_port, _gpio, _init)			\
	{						\
		.gpio	= TEGRA_GPIO(_port, _gpio),	\
		.init	= TEGRA_GPIO_INIT_##_init,	\
	}

static const struct tegra_gpio_config hac_001_gpio_inits[] = {
	/*        port, pin, init_val */
	GPIO_INIT(Z,    4,   OUT0),
	GPIO_INIT(E,    4,   OUT0),	// Sdcard power
	GPIO_INIT(H,    4,   OUT0),	// BT reset
	GPIO_INIT(BB,   2,   IN),
	GPIO_INIT(BB,   3,   OUT0),	// GcAsic Power
	GPIO_INIT(BB,   4,   IN),
	GPIO_INIT(E,    5,   IN),
	GPIO_INIT(S,    0,   IN),	// DebugPadDriver
	GPIO_INIT(S,    1,   IN),
	GPIO_INIT(S,    6,   OUT0),	// bq24192 charge enable
	GPIO_INIT(S,    7,   IN),
	GPIO_INIT(E,    6,   IN),	// Joy-Con(L) insertion
	GPIO_INIT(A,    5,   OUT0),	// Fan (normal)
	GPIO_INIT(P,    0,   IN),
	GPIO_INIT(S,    3,   IN),	// GcAsic irq
	GPIO_INIT(P,    5,   IN),
	GPIO_INIT(P,    4,   IN),
	GPIO_INIT(P,    3,   IN),
	GPIO_INIT(P,    2,   IN),
	GPIO_INIT(X,    4,   IN),
	GPIO_INIT(V,    6,   IN),
	GPIO_INIT(X,    2,   IN),
	GPIO_INIT(X,    1,   IN),	// Touchscreen irq
	GPIO_INIT(X,    5,   IN),
	GPIO_INIT(X,    6,   IN),	// Vol Up
	GPIO_INIT(X,    7,   IN),	// Vol Down
	GPIO_INIT(Y,    0,   IN),	// max17050 irq
	GPIO_INIT(Y,    1,   IN),
	GPIO_INIT(V,    1,   OUT0),	// backlight
	GPIO_INIT(V,    2,   OUT0),	// backlight
	GPIO_INIT(K,    5,   OUT0),	// bq24192 OTG sharge select
	GPIO_INIT(V,    5,   OUT0),	// PD related
	GPIO_INIT(Z,    0,   IN),	// bq24192 irq
	GPIO_INIT(Z,    2,   IN),
	GPIO_INIT(Z,    3,   IN),
	GPIO_INIT(J,    7,   OUT0),	// touchscreen power
	GPIO_INIT(K,    0,   IN),
	GPIO_INIT(K,    1,   IN),
	GPIO_INIT(K,    2,   IN),
	GPIO_INIT(K,    4,   IN),	// bm92t36 irq
	GPIO_INIT(K,    6,   IN),
	GPIO_INIT(K,    7,   IN),
	GPIO_INIT(K,    3,   OUT0),	// Joy-Con(R) charge
	GPIO_INIT(CC,   3,   OUT0),	// Joy-Con(L) charge
	GPIO_INIT(H,    0,   IN),
	GPIO_INIT(H,    1,   OUT0),
	GPIO_INIT(H,    3,   OUT0),	// BT wake
	GPIO_INIT(H,    5,   IN),
	GPIO_INIT(H,    7,   IN),
	GPIO_INIT(I,    0,   OUT0),	// Backlight
	GPIO_INIT(I,    1,   OUT0),	// Backlight
	GPIO_INIT(H,    6,   IN),	// Joy-Con(R) insertion
	GPIO_INIT(CC,   2,   IN),
	GPIO_INIT(CC,   4,   OUT0),	// Fan (high power)
	GPIO_INIT(H,    2,   IN),
	GPIO_INIT(Z,    1,   IN),	// SDcard detect
	GPIO_INIT(J,    5,   OUT0),	// bq24192 OTG charge select
	GPIO_INIT(L,    0,   OUT0),	// bq24192 OTG charge select
	GPIO_INIT(H,    6,   IN),
	GPIO_INIT(E,    6,   IN),

	GPIO_INIT(G,    3,   IN),	// Joy-Con(R) monitor
	GPIO_INIT(D,    4,   IN),	// Joy-Con(L) monitor
	GPIO_INIT(BB,   1,   IN),
	GPIO_INIT(B,    4,   IN),
	GPIO_INIT(B,    5,   IN),
	GPIO_INIT(E,    1,   OUT0),	// USB power
	GPIO_INIT(E,    7,   IN),
	GPIO_INIT(S,    2,   IN),
	GPIO_INIT(S,    5,   OUT0),	// USB root port 4 power
	GPIO_INIT(T,    0,   OUT0),	// USB root port 3 power
	GPIO_INIT(C,    0,   OUT0),	// HDMI hotplug
	GPIO_INIT(C,    1,   OUT0),	// USB root port 2 power
	GPIO_INIT(C,    2,   OUT0),	// HDMI hotplug
	GPIO_INIT(I,    2,   IN),
	GPIO_INIT(CC,   6,   IN),
	GPIO_INIT(CC,   1,   IN),	// HDMI hotplug
};

#define PINCFG(_pingrp, _mux, _pull, _tri, _io, _od, _e_io_hv)	\
	{							\
		.pingrp		= PMUX_PINGRP_##_pingrp,	\
		.func		= PMUX_FUNC_##_mux,		\
		.pull		= PMUX_PULL_##_pull,		\
		.tristate	= PMUX_TRI_##_tri,		\
		.io		= PMUX_PIN_##_io,		\
		.od		= PMUX_PIN_OD_##_od,		\
		.e_io_hv	= PMUX_PIN_E_IO_HV_##_e_io_hv,	\
		.lock		= PMUX_PIN_LOCK_DEFAULT,	\
	}

static const struct pmux_pingrp_config hac_001_pingrps[] = {
	/*     pingrp,               mux,        pull,   tri,      e_input, od,      e_io_hv */
	PINCFG(PEX_L0_RST_N_PA0,     PE0,        NORMAL, NORMAL,   OUTPUT,  DISABLE, NORMAL),
	PINCFG(PEX_L0_CLKREQ_N_PA1,  PE0,        UP,     NORMAL,   INPUT,   DISABLE, NORMAL),
	PINCFG(PEX_WAKE_N_PA2,       PE,         UP,     NORMAL,   INPUT,   DISABLE, NORMAL),
	PINCFG(PEX_L1_RST_N_PA3,     PE1,        NORMAL, NORMAL,   OUTPUT,  DISABLE, NORMAL),
	PINCFG(PEX_L1_CLKREQ_N_PA4,  PE1,        UP,     NORMAL,   INPUT,   DISABLE, NORMAL),
	PINCFG(DAP1_FS_PB0,          I2S1,       NORMAL, NORMAL,   INPUT,   DISABLE, DEFAULT),
	PINCFG(DAP1_DIN_PB1,         I2S1,       NORMAL, NORMAL,   INPUT,   DISABLE, DEFAULT),
	PINCFG(DAP1_DOUT_PB2,        I2S1,       NORMAL, NORMAL,   INPUT,   DISABLE, DEFAULT),
	PINCFG(DAP1_SCLK_PB3,        I2S1,       NORMAL, NORMAL,   INPUT,   DISABLE, DEFAULT),
	PINCFG(UART1_TX_PU0,         UARTA,      NORMAL, NORMAL,   OUTPUT,  DISABLE, DEFAULT),
	PINCFG(UART1_RX_PU1,         UARTA,      NORMAL, NORMAL,   INPUT,   DISABLE, DEFAULT),
	PINCFG(UART1_RTS_PU2,        UARTA,      NORMAL, NORMAL,   OUTPUT,  DISABLE, DEFAULT),
	PINCFG(UART1_CTS_PU3,        UARTA,      NORMAL, NORMAL,   INPUT,   DISABLE, DEFAULT),
	PINCFG(UART2_TX_PG0,         UARTB,      NORMAL, NORMAL,   OUTPUT,  DISABLE, DEFAULT),
	PINCFG(UART2_RX_PG1,         UARTB,      NORMAL, NORMAL,   INPUT,   DISABLE, DEFAULT),
	PINCFG(UART2_RTS_PG2,        UARTB,      NORMAL, NORMAL,   OUTPUT,  DISABLE, DEFAULT),
	PINCFG(UART2_CTS_PG3,        UARTB,      NORMAL, NORMAL,   INPUT,   DISABLE, DEFAULT),
	PINCFG(UART3_TX_PD1,         UARTC,      NORMAL, NORMAL,   OUTPUT,  DISABLE, DEFAULT),
	PINCFG(UART3_RX_PD2,         UARTC,      NORMAL, NORMAL,   INPUT,   DISABLE, DEFAULT),
	PINCFG(UART3_RTS_PD3,        UARTC,      NORMAL, NORMAL,   OUTPUT,  DISABLE, DEFAULT),
	PINCFG(UART3_CTS_PD4,        UARTC,      NORMAL, NORMAL,   INPUT,   DISABLE, DEFAULT),
	PINCFG(UART4_TX_PI4,         UARTD,      NORMAL, NORMAL,   OUTPUT,  DISABLE, DEFAULT),
	PINCFG(UART4_RX_PI5,         UARTD,      NORMAL, NORMAL,   INPUT,   DISABLE, DEFAULT),
	PINCFG(UART4_RTS_PI6,        UARTD,      NORMAL, NORMAL,   OUTPUT,  DISABLE, DEFAULT),
	PINCFG(UART4_CTS_PI7,        UARTD,      NORMAL, NORMAL,   INPUT,   DISABLE, DEFAULT),
	PINCFG(PE6,                  DEFAULT,    DOWN,   NORMAL,   INPUT,   DISABLE, DEFAULT),
	PINCFG(GEN1_I2C_SDA_PJ0,     I2C1,       NORMAL, NORMAL,   INPUT,   DISABLE, NORMAL),
	PINCFG(GEN1_I2C_SCL_PJ1,     I2C1,       NORMAL, NORMAL,   INPUT,   DISABLE, NORMAL),
	PINCFG(GEN2_I2C_SCL_PJ2,     I2C2,       NORMAL, NORMAL,   INPUT,   DISABLE, NORMAL),
	PINCFG(GEN2_I2C_SDA_PJ3,     I2C2,       NORMAL, NORMAL,   INPUT,   DISABLE, NORMAL),
	PINCFG(GEN3_I2C_SCL_PF0,     I2C3,       NORMAL, NORMAL,   INPUT,   DISABLE, NORMAL),
	PINCFG(GEN3_I2C_SDA_PF1,     I2C3,       NORMAL, NORMAL,   INPUT,   DISABLE, NORMAL),
	PINCFG(SDMMC1_CLK_PM0,       SDMMC1,     NORMAL, NORMAL,   INPUT,   DISABLE, DEFAULT),
	PINCFG(SDMMC1_CMD_PM1,       SDMMC1,     UP,     NORMAL,   INPUT,   DISABLE, DEFAULT),
	PINCFG(SDMMC1_DAT3_PM2,      SDMMC1,     UP,     NORMAL,   INPUT,   DISABLE, DEFAULT),
	PINCFG(SDMMC1_DAT2_PM3,      SDMMC1,     UP,     NORMAL,   INPUT,   DISABLE, DEFAULT),
	PINCFG(SDMMC1_DAT1_PM4,      SDMMC1,     UP,     NORMAL,   INPUT,   DISABLE, DEFAULT),
	PINCFG(SDMMC1_DAT0_PM5,      SDMMC1,     UP,     NORMAL,   INPUT,   DISABLE, DEFAULT),
	PINCFG(LCD_BL_PWM_PV0,       PWM0,       NORMAL, NORMAL,   OUTPUT,  DISABLE, DEFAULT),
	PINCFG(LCD_BL_EN_PV1,        DEFAULT,    NORMAL, NORMAL,   OUTPUT,  DISABLE, DEFAULT),
	PINCFG(LCD_RST_PV2,          DEFAULT,    NORMAL, NORMAL,   OUTPUT,  DISABLE, DEFAULT),
	PINCFG(BUTTON_POWER_ON_PX5,  DEFAULT,    UP,     NORMAL,   INPUT,   DISABLE, DEFAULT),
	PINCFG(BUTTON_VOL_UP_PX6,    DEFAULT,    UP,     NORMAL,   INPUT,   DISABLE, DEFAULT),
	PINCFG(BUTTON_VOL_DOWN_PX7,  DEFAULT,    UP,     NORMAL,   INPUT,   DISABLE, DEFAULT),
	PINCFG(BUTTON_SLIDE_SW_PY0,  DEFAULT,    UP,     NORMAL,   INPUT,   DISABLE, DEFAULT),
	PINCFG(BUTTON_HOME_PY1,      DEFAULT,    UP,     NORMAL,   INPUT,   DISABLE, DEFAULT),	
	PINCFG(HDMI_INT_DP_HPD_PCC1, DEFAULT,    DOWN,   NORMAL,   INPUT,   DISABLE, NORMAL),
	PINCFG(WIFI_EN_PH0,          DEFAULT,    NORMAL, NORMAL,   OUTPUT,  DISABLE, DEFAULT),
	PINCFG(WIFI_RST_PH1,         DEFAULT,    NORMAL, NORMAL,   OUTPUT,  DISABLE, DEFAULT),
	PINCFG(WIFI_WAKE_AP_PH2,     DEFAULT,    UP,     NORMAL,   INPUT,   DISABLE, DEFAULT),
	PINCFG(AP_WAKE_BT_PH3,       DEFAULT,    NORMAL, NORMAL,   OUTPUT,  DISABLE, DEFAULT),
	PINCFG(BT_RST_PH4,           DEFAULT,    NORMAL, NORMAL,   OUTPUT,  DISABLE, DEFAULT),
	PINCFG(BT_WAKE_AP_PH5,       DEFAULT,    UP,     NORMAL,   INPUT,   DISABLE, DEFAULT),
	PINCFG(AP_WAKE_NFC_PH7,      DEFAULT,    NORMAL, NORMAL,   OUTPUT,  DISABLE, DEFAULT),
	PINCFG(AP_READY_PV5,         DEFAULT,    NORMAL, NORMAL,   OUTPUT,  DISABLE, DEFAULT),
	PINCFG(TOUCH_RST_PV6,        DEFAULT,    NORMAL, NORMAL,   OUTPUT,  DISABLE, DEFAULT),
	PINCFG(TOUCH_INT_PX1,        DEFAULT,    UP,     NORMAL,   INPUT,   DISABLE, DEFAULT),
	PINCFG(MOTION_INT_PX2,       DEFAULT,    UP,     NORMAL,   INPUT,   DISABLE, DEFAULT),
	PINCFG(TEMP_ALERT_PX4,       DEFAULT,    UP,     NORMAL,   INPUT,   DISABLE, DEFAULT),
	PINCFG(LCD_TE_PY2,           DISPLAYA,   DOWN,   NORMAL,   INPUT,   DISABLE, DEFAULT),
	PINCFG(PWR_I2C_SCL_PY3,      I2CPMU,     NORMAL, NORMAL,   INPUT,   DISABLE, NORMAL),
	PINCFG(PWR_I2C_SDA_PY4,      I2CPMU,     NORMAL, NORMAL,   INPUT,   DISABLE, NORMAL),
	PINCFG(CLK_32K_OUT_PY5,      SOC,        UP,     NORMAL,   INPUT,   DISABLE, DEFAULT),
	PINCFG(PZ0,                  DEFAULT,    UP,     NORMAL,   INPUT,   DISABLE, DEFAULT),
	PINCFG(DAP2_FS_PAA0,         I2S2,       NORMAL, NORMAL,   INPUT,   DISABLE, DEFAULT),
	PINCFG(DAP2_SCLK_PAA1,       I2S2,       NORMAL, NORMAL,   INPUT,   DISABLE, DEFAULT),
	PINCFG(DAP2_DIN_PAA2,        I2S2,       NORMAL, NORMAL,   INPUT,   DISABLE, DEFAULT),
	PINCFG(DAP2_DOUT_PAA3,       I2S2,       NORMAL, NORMAL,   INPUT,   DISABLE, DEFAULT),
	PINCFG(AUD_MCLK_PBB0,        AUD,        NORMAL, NORMAL,   OUTPUT,  DISABLE, DEFAULT),
	PINCFG(DVFS_PWM_PBB1,        CLDVFS,     NORMAL, TRISTATE, OUTPUT,  DISABLE, DEFAULT),
	PINCFG(DVFS_CLK_PBB2,        DEFAULT,    NORMAL, NORMAL,   OUTPUT,  DISABLE, DEFAULT),
	PINCFG(GPIO_X3_AUD_PBB4,     DEFAULT,    UP,     NORMAL,   INPUT,   DISABLE, DEFAULT),
	PINCFG(HDMI_CEC_PCC0,        CEC,        NORMAL, NORMAL,   INPUT,   DISABLE, HIGH),
	PINCFG(DMIC1_DAT_PE1,        USB,        NORMAL, NORMAL,   INPUT,   DISABLE, HIGH),
	PINCFG(USB_VBUS_EN1_PCC5,    DEFAULT,    NORMAL, NORMAL,   OUTPUT,  DISABLE, NORMAL),
	PINCFG(DP_HPD0_PCC6,         DEFAULT,    DOWN,   NORMAL,   INPUT,   DISABLE, DEFAULT),
	PINCFG(CORE_PWR_REQ,         CORE,       NORMAL, NORMAL,   OUTPUT,  DISABLE, DEFAULT),
	PINCFG(CPU_PWR_REQ,          CPU,        NORMAL, NORMAL,   OUTPUT,  DISABLE, DEFAULT),
	PINCFG(PWR_INT_N,            PMI,        UP,     NORMAL,   INPUT,   DISABLE, DEFAULT),
	PINCFG(CLK_32K_IN,           CLK,        NORMAL, NORMAL,   INPUT,   DISABLE, DEFAULT),
	PINCFG(JTAG_RTCK,            JTAG,       NORMAL, NORMAL,   OUTPUT,  DISABLE, DEFAULT),
	PINCFG(CLK_REQ,              SYS,        NORMAL, NORMAL,   OUTPUT,  DISABLE, DEFAULT),
	PINCFG(SHUTDOWN,             SHUTDOWN,   NORMAL, NORMAL,   OUTPUT,  DISABLE, DEFAULT),

};

#define DRVCFG(_drvgrp, _slwf, _slwr, _drvup, _drvdn, _lpmd, _schmt, _hsm) \
	{						\
		.drvgrp = PMUX_DRVGRP_##_drvgrp,	\
		.slwf   = _slwf,			\
		.slwr   = _slwr,			\
		.drvup  = _drvup,			\
		.drvdn  = _drvdn,			\
		.lpmd   = PMUX_LPMD_##_lpmd,		\
		.schmt  = PMUX_SCHMT_##_schmt,		\
		.hsm    = PMUX_HSM_##_hsm,		\
	}

static const struct pmux_drvgrp_config hac_001_drvgrps[] = {
};

#endif /* PINMUX_CONFIG_HAC_001_H */
