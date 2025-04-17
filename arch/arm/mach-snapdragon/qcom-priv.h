// SPDX-License-Identifier: GPL-2.0

#ifndef __QCOM_PRIV_H__
#define __QCOM_PRIV_H__

#include <stdbool.h>

#if IS_ENABLED(CONFIG_EFI_HAVE_CAPSULE_SUPPORT)
void qcom_configure_capsule_updates(void);
#else
static void __maybe_unused qcom_configure_capsule_updates(void) {}
#endif /* EFI_HAVE_CAPSULE_SUPPORT */

#if CONFIG_IS_ENABLED(OF_LIVE)
/**
 * qcom_of_fixup_nodes() - Fixup Qualcomm DT nodes
 *
 * Adjusts nodes in the live tree to improve compatibility with U-Boot.
 */
void qcom_of_fixup_nodes(void);
#else
static inline void qcom_of_fixup_nodes(void)
{
	log_debug("Unable to dynamically fixup USB nodes, please enable CONFIG_OF_LIVE\n");
}
#endif /* OF_LIVE */

void gunyah_init(void);

void qcom_parse_memory(const void *fdt, bool fdt_is_internal);

#endif /* __QCOM_PRIV_H__ */
