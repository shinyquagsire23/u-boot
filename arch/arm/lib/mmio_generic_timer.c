// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2025
 * Max Thomas <mtinc2@gmail.com>
 */

#include <bootstage.h>
#include <command.h>
#include <time.h>
#include <asm/global_data.h>
#include <asm/system.h>
#include <linux/bitops.h>
#include <linux/ioport.h>
#include <asm/io.h>
#include <dm/read.h>

DECLARE_GLOBAL_DATA_PTR;

#define REG_CNTPCT    (0x0)
#define REG_CNTPCT_LO (0x0)
#define REG_CNTPCT_HI (0x4)
#define REG_CNTFRQ    (0x10)

int timer_init(void)
{
    ofnode timer;
    ofnode frame;

    /* Use this as the current monotonic time in us */
    gd->arch.timer_reset_value = 0;

    /* Find the timer node */
    timer = ofnode_by_compatible(ofnode_null(), "arm,armv7-timer-mem");
    if (!ofnode_valid(timer)) {
        log_err("mmio_generic_timer: failed to find armv7-timer-mem node\n");
        goto fail;
    }

    /* Get the first frame child node */
    frame = ofnode_first_subnode(timer);
    if (!ofnode_valid(frame)) {
        log_err("mmio_generic_timer: no child frame\n");
        goto fail;
    }

    /* Grab the register base address for the frame */
    gd->arch.timer_base = ofnode_get_addr_index(frame, 0);
    if (gd->arch.timer_base != FDT_ADDR_T_NONE) {
        return 0;
    }

fail:
    // TODO(shinyquagsire23): Kconfig fallback override
    gd->arch.timer_base = CONFIG_ARM_MMIO_TIMER_FALLBACK_ADDR;
    if (!gd->arch.timer_base) {
        return -ENOENT;    
    }

    return 0;
}

/*
 * Generic timer implementation of get_tbclk()
 */
unsigned long notrace get_tbclk(void)
{
    if (!gd->arch.timer_base) {
        return 1;
    }
    return readl(gd->arch.timer_base + REG_CNTFRQ);
}

/*
 * timer_read_counter() using the Arm Generic Timer (aka arch timer).
 */
unsigned long notrace timer_read_counter(void)
{
    if (!gd->arch.timer_base) {
        return gd->arch.tbl++;
    }
    return readq(gd->arch.timer_base + REG_CNTPCT);
}

uint64_t notrace get_ticks(void)
{
    unsigned long ticks = timer_read_counter();

    gd->arch.tbl = ticks;

    return ticks;
}

unsigned long usec2ticks(unsigned long usec)
{
    if (!gd->arch.timer_base) {
        return 1;
    }

    ulong ticks;
    if (usec < 1000)
        ticks = ((usec * (get_tbclk()/1000)) + 500) / 1000;
    else
        ticks = ((usec / 10) * (get_tbclk() / 100000));

    return ticks;
}

ulong timer_get_boot_us(void)
{
    if (!gd->arch.timer_base) {
        return gd->arch.tbl++;
    }

    u64 val = get_ticks() * 1000000;

    return val / get_tbclk();
}

void __udelay(unsigned long usec)
{
    if (!gd->arch.timer_base) {
        return;
    }

    u64 target = get_ticks() + usec_to_tick(usec);
    while (get_ticks() <= target)
        ;
}
