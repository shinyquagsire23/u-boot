// SPDX-License-Identifier: GPL-2.0+
/*
 * Common initialisation for Qualcomm Snapdragon boards.
 *
 * Copyright (c) 2024 Linaro Ltd.
 * Author: Caleb Connolly <caleb.connolly@linaro.org>
 */

#define LOG_CATEGORY LOGC_BOARD
#define pr_fmt(fmt) "Snapdragon: " fmt

#define LOG_DEBUG

#include <asm/armv8/mmu.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <asm/psci.h>
#include <asm/system.h>
#include <dm/device.h>
#include <dm/pinctrl.h>
#include <dm/uclass-internal.h>
#include <dm/read.h>
#include <power/regulator.h>
#include <env.h>
#include <fdt_support.h>
#include <init.h>
#include <linux/arm-smccc.h>
#include <linux/bug.h>
#include <linux/psci.h>
#include <linux/sizes.h>
#include <lmb.h>
#include <malloc.h>
#include <fdt_support.h>
#include <usb.h>
#include <sort.h>
#include <asm/ptrace.h>
#include <time.h>

#include "qcom-priv.h"

#define GUNYAH_CALL_IDENTIFY (0x6000)
#define GUNYAH_CALL_PART_CREATE_PARTITION (0x6001)

#define _GUNYAH_API_FEATURE_FLAGS(X) \
	X(GUNYAH_API_FEATURE_CSPACE, 0) \
	X(GUNYAH_API_FEATURE_DOORBELL, 1) \
	X(GUNYAH_API_FEATURE_MESSAGE_QUEUE, 2) \
	X(GUNYAH_API_FEATURE_VIQ, 3) \
	X(GUNYAH_API_FEATURE_VCPU, 4) \
	X(GUNYAH_API_FEATURE_MEM_EXTENT, 5) \
	X(GUNYAH_API_FEATURE_TRACING, 6)

#define X(name, val) name = BIT(val),
enum {
	_GUNYAH_API_FEATURE_FLAGS(X)
};
#undef X


#define X(name, val) #name,
static const char *gunyah_api_feature_names[] = {
	_GUNYAH_API_FEATURE_FLAGS(X)
};
#undef X

/*
 * Issue the hypervisor call
 *
 * x0~x7: input arguments
 * x0~x3: output arguments
 */
#define hvc_call(callnum, args) \
	asm volatile( \
		"ldr x0, %0\n" \
		"ldr x1, %1\n" \
		"ldr x2, %2\n" \
		"ldr x3, %3\n" \
		"ldr x4, %4\n" \
		"ldr x5, %5\n" \
		"ldr x6, %6\n" \
		"ldr x7, %7\n" \
		"hvc %[call]\n" \
		"str x0, %0\n" \
		"str x1, %1\n" \
		"str x2, %2\n" \
		"str x3, %3\n" \
		"str x4, %4\n" \
		"str x5, %5\n" \
		"str x6, %6\n" \
		"str x7, %7\n" \
		: "+m" ((args)->regs[0]), "+m" ((args)->regs[1]), \
		  "+m" ((args)->regs[2]), "+m" ((args)->regs[3]), \
		  "+m" ((args)->regs[4]), "+m" ((args)->regs[5]), \
		  "+m" ((args)->regs[6]), "+m" ((args)->regs[7]) \
		: "m" ((args)->regs[0]), "m" ((args)->regs[1]), \
		  "m" ((args)->regs[2]), "m" ((args)->regs[3]), \
		  "m" ((args)->regs[4]), "m" ((args)->regs[5]), \
		  "m" ((args)->regs[6]), "m" ((args)->regs[7]), \
		  [call] "n" ((callnum)) \
		: "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7", \
		  "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15", \
		  "x16", "x17");

struct gunyah_api_info {
	union {
		u64 api_info; /* x0 */
		struct {
			u32 api_version : 14;
			u32 big_endian : 1;
			u32 is_64bit : 1;
			u64 _res_x0 : 40;
			u32 variant : 8;
		};
	};
	union {
		u64 flags0; /* x1 */
		struct {
			/* supported APIs */
			u32 support_cspace : 1;
			u32 support_doorbell : 1;
			u32 support_message_queue : 1;
			u32 support_viq : 1;
			u32 support_vcpu : 1;
			u32 support_mem_extent : 1;
			u32 support_tracing : 1;
			u32 _support_reserved : 8;

			u32 _res_x1_0 : 1;
			u64 _res_x1_1 : 46;
		};
	};
	union {
		u64 flags1; /* x2 */
		struct {
			u32 support_amv8_2_sve : 1;
		};
	};
	union {
		u64 flags2; /* x3 - reserved */
	};
};

struct gunyah_create_partition_result {
	u64 error;
	u64 new_cap;
};

static void gunyah_print_info(struct gunyah_api_info *info)
{
	debug("API Info: %#018llx\n", info->api_info);
	debug(" Flags 0: %#018llx\n", info->flags0);
	debug(" Flags 1: %#018llx\n", info->flags1);
	debug(" Flags 2: %#018llx\n", info->flags2);
	debug(" Variant: %#010x\n", info->variant);

	printf("Gunyah [v%u %s] %s endian, %s ARMv8.2 SVE support\n",
		info->api_version,
		info->variant == 0x48 ? "Haven" : "Unknown",
		info->big_endian ? "big" : "little",
		info->support_amv8_2_sve ? "with" : "without");
	
	printf("  Supported features:\n");
	for (int i = 0; i < ARRAY_SIZE(gunyah_api_feature_names); i++) {
		if (info->flags0 & BIT(i))
			printf("  * %s\n", gunyah_api_feature_names[i]);
	}
}

static void gunyah_api_info(struct gunyah_api_info *info)
{
	struct pt_regs args = { 0 };

	hvc_call(GUNYAH_CALL_IDENTIFY, &args);

	info->api_info = args.regs[0];
	info->flags0 = args.regs[1];
	info->flags1 = args.regs[2];
	info->flags2 = args.regs[3];

	gunyah_print_info(info);
}

static struct gunyah_create_partition_result __maybe_unused gunyah_create_partition(u64 part_capid, u64 cspace_capid)
{
	struct pt_regs args = { 0 };
	struct gunyah_create_partition_result res;

	args.regs[0] = part_capid;
	args.regs[1] = cspace_capid;

	hvc_call(GUNYAH_CALL_PART_CREATE_PARTITION, &args);

	res.error = args.regs[0];
	res.new_cap = args.regs[1];

	

	return res;
}

void gunyah_call(u64 callnum, struct pt_regs *args)
{
	switch (callnum) {
	case 0x6001: hvc_call(0x6001, args); break;
	case 0x6002: hvc_call(0x6002, args); break;
	case 0x6003: hvc_call(0x6003, args); break;
	case 0x6004: hvc_call(0x6004, args); break;
	case 0x6005: hvc_call(0x6005, args); break;
	case 0x6006: hvc_call(0x6006, args); break;
	case 0x6007: hvc_call(0x6007, args); break;
	case 0x6008: hvc_call(0x6008, args); break;
	case 0x6009: hvc_call(0x6009, args); break;
	case 0x600a: hvc_call(0x600a, args); break;
	case 0x600b: hvc_call(0x600b, args); break;
	case 0x600c: hvc_call(0x600c, args); break;
	case 0x600d: hvc_call(0x600d, args); break;
	case 0x600e: hvc_call(0x600e, args); break;
	case 0x600f: hvc_call(0x600f, args); break;
	case 0x6010: hvc_call(0x6010, args); break;
	case 0x6011: hvc_call(0x6011, args); break;
	case 0x6012: hvc_call(0x6012, args); break;
	case 0x6013: hvc_call(0x6013, args); break;
	case 0x6014: hvc_call(0x6014, args); break;
	case 0x6015: hvc_call(0x6015, args); break;
	case 0x6016: hvc_call(0x6016, args); break;
	case 0x6017: hvc_call(0x6017, args); break;
	case 0x6018: hvc_call(0x6018, args); break;
	case 0x6019: hvc_call(0x6019, args); break;
	case 0x601a: hvc_call(0x601a, args); break;
	case 0x601b: hvc_call(0x601b, args); break;
	case 0x601c: hvc_call(0x601c, args); break;
	case 0x601d: hvc_call(0x601d, args); break;
	case 0x601e: hvc_call(0x601e, args); break;
	case 0x601f: hvc_call(0x601f, args); break;
	case 0x6020: hvc_call(0x6020, args); break;
	case 0x6021: hvc_call(0x6021, args); break;
	case 0x6022: hvc_call(0x6022, args); break;
	case 0x6023: hvc_call(0x6023, args); break;
	case 0x6024: hvc_call(0x6024, args); break;
	case 0x6025: hvc_call(0x6025, args); break;
	case 0x6026: hvc_call(0x6026, args); break;
	case 0x6027: hvc_call(0x6027, args); break;
	case 0x6028: hvc_call(0x6028, args); break;
	case 0x6029: hvc_call(0x6029, args); break;
	case 0x602a: hvc_call(0x602a, args); break;
	case 0x602b: hvc_call(0x602b, args); break;
	case 0x602c: hvc_call(0x602c, args); break;
	case 0x602d: hvc_call(0x602d, args); break;
	case 0x602e: hvc_call(0x602e, args); break;
	case 0x602f: hvc_call(0x602f, args); break;
	case 0x6030: hvc_call(0x6030, args); break;
	case 0x6031: hvc_call(0x6031, args); break;
	case 0x6032: hvc_call(0x6032, args); break;
	case 0x6033: hvc_call(0x6033, args); break;
	case 0x6034: hvc_call(0x6034, args); break;
	case 0x6035: hvc_call(0x6035, args); break;
	case 0x6036: hvc_call(0x6036, args); break;
	case 0x6037: hvc_call(0x6037, args); break;
	case 0x6038: hvc_call(0x6038, args); break;
	case 0x6039: hvc_call(0x6039, args); break;
	case 0x603a: hvc_call(0x603a, args); break;
	case 0x603b: hvc_call(0x603b, args); break;
	case 0x603c: hvc_call(0x603c, args); break;
	case 0x603d: hvc_call(0x603d, args); break;
	case 0x603e: hvc_call(0x603e, args); break;
	case 0x603f: hvc_call(0x603f, args); break;
	case 0x6040: hvc_call(0x6040, args); break;
	case 0x6041: hvc_call(0x6041, args); break;
	case 0x6042: hvc_call(0x6042, args); break;
	case 0x6043: hvc_call(0x6043, args); break;
	case 0x6044: hvc_call(0x6044, args); break;
	case 0x6045: hvc_call(0x6045, args); break;
	case 0x6046: hvc_call(0x6046, args); break;
	case 0x6047: hvc_call(0x6047, args); break;
	case 0x6048: hvc_call(0x6048, args); break;
	case 0x6049: hvc_call(0x6049, args); break;
	case 0x604a: hvc_call(0x604a, args); break;
	case 0x604b: hvc_call(0x604b, args); break;
	case 0x604c: hvc_call(0x604c, args); break;
	case 0x604d: hvc_call(0x604d, args); break;
	case 0x604e: hvc_call(0x604e, args); break;
	case 0x604f: hvc_call(0x604f, args); break;
	case 0x6050: hvc_call(0x6050, args); break;
	case 0x6051: hvc_call(0x6051, args); break;
	case 0x6052: hvc_call(0x6052, args); break;
	case 0x6053: hvc_call(0x6053, args); break;
	case 0x6054: hvc_call(0x6054, args); break;
	case 0x6055: hvc_call(0x6055, args); break;
	case 0x6056: hvc_call(0x6056, args); break;
	case 0x6057: hvc_call(0x6057, args); break;
	case 0x6058: hvc_call(0x6058, args); break;
	case 0x6059: hvc_call(0x6059, args); break;
	case 0x605a: hvc_call(0x605a, args); break;
	case 0x605b: hvc_call(0x605b, args); break;
	case 0x605c: hvc_call(0x605c, args); break;
	case 0x605d: hvc_call(0x605d, args); break;
	case 0x605e: hvc_call(0x605e, args); break;
	case 0x605f: hvc_call(0x605f, args); break;
	case 0x6060: hvc_call(0x6060, args); break;
	case 0x6061: hvc_call(0x6061, args); break;
	case 0x6062: hvc_call(0x6062, args); break;
	case 0x6063: hvc_call(0x6063, args); break;
	case 0x6064: hvc_call(0x6064, args); break;
	case 0x6065: hvc_call(0x6065, args); break;
	case 0x6066: hvc_call(0x6066, args); break;
	case 0x6067: hvc_call(0x6067, args); break;
	case 0x6068: hvc_call(0x6068, args); break;
	case 0x6069: hvc_call(0x6069, args); break;
	case 0x606a: hvc_call(0x606a, args); break;
	case 0x606b: hvc_call(0x606b, args); break;
	case 0x606c: hvc_call(0x606c, args); break;
	case 0x606d: hvc_call(0x606d, args); break;
	case 0x606e: hvc_call(0x606e, args); break;
	case 0x606f: hvc_call(0x606f, args); break;
	case 0x6070: hvc_call(0x6070, args); break;
	case 0x6071: hvc_call(0x6071, args); break;
	case 0x6072: hvc_call(0x6072, args); break;
	case 0x6073: hvc_call(0x6073, args); break;
	case 0x6074: hvc_call(0x6074, args); break;
	case 0x6075: hvc_call(0x6075, args); break;
	case 0x6076: hvc_call(0x6076, args); break;
	case 0x6077: hvc_call(0x6077, args); break;
	case 0x6078: hvc_call(0x6078, args); break;
	case 0x6079: hvc_call(0x6079, args); break;
	case 0x607a: hvc_call(0x607a, args); break;
	case 0x607b: hvc_call(0x607b, args); break;
	case 0x607c: hvc_call(0x607c, args); break;
	case 0x607d: hvc_call(0x607d, args); break;
	case 0x607e: hvc_call(0x607e, args); break;
	case 0x607f: hvc_call(0x607f, args); break;
	case 0x6080: hvc_call(0x6080, args); break;
	case 0x6081: hvc_call(0x6081, args); break;
	case 0x6082: hvc_call(0x6082, args); break;
	case 0x6083: hvc_call(0x6083, args); break;
	case 0x6084: hvc_call(0x6084, args); break;
	case 0x6085: hvc_call(0x6085, args); break;
	case 0x6086: hvc_call(0x6086, args); break;
	case 0x6087: hvc_call(0x6087, args); break;
	case 0x6088: hvc_call(0x6088, args); break;
	case 0x6089: hvc_call(0x6089, args); break;
	case 0x608a: hvc_call(0x608a, args); break;
	case 0x608b: hvc_call(0x608b, args); break;
	case 0x608c: hvc_call(0x608c, args); break;
	case 0x608d: hvc_call(0x608d, args); break;
	case 0x608e: hvc_call(0x608e, args); break;
	case 0x608f: hvc_call(0x608f, args); break;
	case 0x6090: hvc_call(0x6090, args); break;
	case 0x6091: hvc_call(0x6091, args); break;
	case 0x6092: hvc_call(0x6092, args); break;
	case 0x6093: hvc_call(0x6093, args); break;
	case 0x6094: hvc_call(0x6094, args); break;
	case 0x6095: hvc_call(0x6095, args); break;
	case 0x6096: hvc_call(0x6096, args); break;
	case 0x6097: hvc_call(0x6097, args); break;
	case 0x6098: hvc_call(0x6098, args); break;
	case 0x6099: hvc_call(0x6099, args); break;
	case 0x609a: hvc_call(0x609a, args); break;
	case 0x609b: hvc_call(0x609b, args); break;
	case 0x609c: hvc_call(0x609c, args); break;
	case 0x609d: hvc_call(0x609d, args); break;
	case 0x609e: hvc_call(0x609e, args); break;
	case 0x609f: hvc_call(0x609f, args); break;
	case 0x60a0: hvc_call(0x60a0, args); break;
	case 0x60a1: hvc_call(0x60a1, args); break;
	case 0x60a2: hvc_call(0x60a2, args); break;
	case 0x60a3: hvc_call(0x60a3, args); break;
	case 0x60a4: hvc_call(0x60a4, args); break;
	case 0x60a5: hvc_call(0x60a5, args); break;
	case 0x60a6: hvc_call(0x60a6, args); break;
	case 0x60a7: hvc_call(0x60a7, args); break;
	case 0x60a8: hvc_call(0x60a8, args); break;
	case 0x60a9: hvc_call(0x60a9, args); break;
	case 0x60aa: hvc_call(0x60aa, args); break;
	case 0x60ab: hvc_call(0x60ab, args); break;
	case 0x60ac: hvc_call(0x60ac, args); break;
	case 0x60ad: hvc_call(0x60ad, args); break;
	case 0x60ae: hvc_call(0x60ae, args); break;
	case 0x60af: hvc_call(0x60af, args); break;
	case 0x60b0: hvc_call(0x60b0, args); break;
	case 0x60b1: hvc_call(0x60b1, args); break;
	case 0x60b2: hvc_call(0x60b2, args); break;
	case 0x60b3: hvc_call(0x60b3, args); break;
	case 0x60b4: hvc_call(0x60b4, args); break;
	case 0x60b5: hvc_call(0x60b5, args); break;
	case 0x60b6: hvc_call(0x60b6, args); break;
	case 0x60b7: hvc_call(0x60b7, args); break;
	case 0x60b8: hvc_call(0x60b8, args); break;
	case 0x60b9: hvc_call(0x60b9, args); break;
	case 0x60ba: hvc_call(0x60ba, args); break;
	case 0x60bb: hvc_call(0x60bb, args); break;
	case 0x60bc: hvc_call(0x60bc, args); break;
	case 0x60bd: hvc_call(0x60bd, args); break;
	case 0x60be: hvc_call(0x60be, args); break;
	case 0x60bf: hvc_call(0x60bf, args); break;
	case 0x60c0: hvc_call(0x60c0, args); break;
	case 0x60c1: hvc_call(0x60c1, args); break;
	case 0x60c2: hvc_call(0x60c2, args); break;
	case 0x60c3: hvc_call(0x60c3, args); break;
	case 0x60c4: hvc_call(0x60c4, args); break;
	case 0x60c5: hvc_call(0x60c5, args); break;
	case 0x60c6: hvc_call(0x60c6, args); break;
	case 0x60c7: hvc_call(0x60c7, args); break;
	case 0x60c8: hvc_call(0x60c8, args); break;
	case 0x60c9: hvc_call(0x60c9, args); break;
	case 0x60ca: hvc_call(0x60ca, args); break;
	case 0x60cb: hvc_call(0x60cb, args); break;
	case 0x60cc: hvc_call(0x60cc, args); break;
	case 0x60cd: hvc_call(0x60cd, args); break;
	case 0x60ce: hvc_call(0x60ce, args); break;
	case 0x60cf: hvc_call(0x60cf, args); break;
	case 0x60d0: hvc_call(0x60d0, args); break;
	case 0x60d1: hvc_call(0x60d1, args); break;
	case 0x60d2: hvc_call(0x60d2, args); break;
	case 0x60d3: hvc_call(0x60d3, args); break;
	case 0x60d4: hvc_call(0x60d4, args); break;
	case 0x60d5: hvc_call(0x60d5, args); break;
	case 0x60d6: hvc_call(0x60d6, args); break;
	case 0x60d7: hvc_call(0x60d7, args); break;
	case 0x60d8: hvc_call(0x60d8, args); break;
	case 0x60d9: hvc_call(0x60d9, args); break;
	case 0x60da: hvc_call(0x60da, args); break;
	case 0x60db: hvc_call(0x60db, args); break;
	case 0x60dc: hvc_call(0x60dc, args); break;
	case 0x60dd: hvc_call(0x60dd, args); break;
	case 0x60de: hvc_call(0x60de, args); break;
	case 0x60df: hvc_call(0x60df, args); break;
	case 0x60e0: hvc_call(0x60e0, args); break;
	case 0x60e1: hvc_call(0x60e1, args); break;
	case 0x60e2: hvc_call(0x60e2, args); break;
	case 0x60e3: hvc_call(0x60e3, args); break;
	case 0x60e4: hvc_call(0x60e4, args); break;
	case 0x60e5: hvc_call(0x60e5, args); break;
	case 0x60e6: hvc_call(0x60e6, args); break;
	case 0x60e7: hvc_call(0x60e7, args); break;
	case 0x60e8: hvc_call(0x60e8, args); break;
	case 0x60e9: hvc_call(0x60e9, args); break;
	case 0x60ea: hvc_call(0x60ea, args); break;
	case 0x60eb: hvc_call(0x60eb, args); break;
	case 0x60ec: hvc_call(0x60ec, args); break;
	case 0x60ed: hvc_call(0x60ed, args); break;
	case 0x60ee: hvc_call(0x60ee, args); break;
	case 0x60ef: hvc_call(0x60ef, args); break;
	case 0x60f0: hvc_call(0x60f0, args); break;
	case 0x60f1: hvc_call(0x60f1, args); break;
	case 0x60f2: hvc_call(0x60f2, args); break;
	case 0x60f3: hvc_call(0x60f3, args); break;
	case 0x60f4: hvc_call(0x60f4, args); break;
	case 0x60f5: hvc_call(0x60f5, args); break;
	case 0x60f6: hvc_call(0x60f6, args); break;
	case 0x60f7: hvc_call(0x60f7, args); break;
	case 0x60f8: hvc_call(0x60f8, args); break;
	case 0x60f9: hvc_call(0x60f9, args); break;
	case 0x60fa: hvc_call(0x60fa, args); break;
	case 0x60fb: hvc_call(0x60fb, args); break;
	case 0x60fc: hvc_call(0x60fc, args); break;
	case 0x60fd: hvc_call(0x60fd, args); break;
	case 0x60fe: hvc_call(0x60fe, args); break;
	case 0x60ff: hvc_call(0x60ff, args); break;
	}
}

void gunyah_init(void)
{
	struct gunyah_api_info api_info;

	printf("Waving to Gunyah...\n");

	gunyah_api_info(&api_info);
}
