/*
 * Copyright (c) 2015-2019 Intel Corporation.
 * All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include "guest.h"
#include "gpm.h"
#include "hmm.h"
#include "event.h"
#include "heap.h"
#include "gcpu.h"
#include "vmm_objects.h"
#include "modules/vmcall.h"
#if MAX_CPU_NUM > 1
#include "modules/ipc.h"
#endif
#include "vmm_asm.h"
#include "ept.h"
#include "vmm_arch.h"
#include "vmx_cap.h"

/* Currently:
 * 1. If MODULE_EPT_UPDATE is enabled, EPT table of Guest1(LK) will only cover its self memory.
 * 2. If MULTI_GUEST_DMA is enabled and a device(CSE) is assigned to Guest1(LK), then the
 *    context-entry for the device will redirect to the new address translation table which
 *    followed the Guest1's EPT table.
 * So if the flags are enabled at the same time, the device(CSE) which assigned to Guest1 will
 * be only able to access Guest1(LK)'s memory by DMA, but actually CSE is also used in
 * Guest0(Android) and needs the access to Guest0(Android)'s memory. So the EPT_UPDATE should
 * NOT enabled when MODULE_VTD and MULTI_GUEST_DMA is set. */
#if defined MODULE_VTD && defined MULTI_GUEST_DMA
#error "EPT_UPDATE module should not enabled when MDULE_VTD && MULTI_GUEST_DMA is set"
#endif

#define VMCALL_EPT_UPDATE  0x65707501 //0x657075='ept'

#if MAX_CPU_NUM > 1
static void flush_ept(UNUSED guest_cpu_handle_t gcpu, void *arg)
{
	uint64_t eptp = (uint64_t)arg;

	asm_invept(eptp, INVEPT_TYPE_SINGLE_CONTEXT);
}
#endif

static void assert_mapping_status(guest_cpu_handle_t gcpu, uint64_t start, uint64_t size, boolean_t mapped)
{
	uint64_t tgt_addr;
	ept_attr_t attr;
	boolean_t present;
	uint64_t end = start + size;

	while (start < end) {
		present = gpm_gpa_to_hpa(gcpu->guest, start, &tgt_addr, &attr);

		VMM_ASSERT_EX((mapped == present),
			"ept mapping check fail at 0x%llx, mapped:%d, present:%d\n", start, mapped, present);

		start += 4096;
	}
}

static void trusty_vmcall_ept_update(guest_cpu_handle_t gcpu)
{
	enum {
		ADD,
		REMOVE,
	};

	uint64_t start = gcpu_get_gp_reg(gcpu, REG_RDI);
	uint64_t size = gcpu_get_gp_reg(gcpu, REG_RSI);
	uint32_t action = (uint32_t)gcpu_get_gp_reg(gcpu, REG_RDX);
#if MAX_CPU_NUM > 1
	uint32_t flush_all_cpus = (uint32_t)gcpu_get_gp_reg(gcpu, REG_RCX);
#endif
	ept_attr_t attr;

	switch (action) {
	case ADD:
		assert_mapping_status(gcpu, start, size, FALSE);

		attr.uint32 = 0x3;
		attr.bits.emt = CACHE_TYPE_WB;
		gpm_set_mapping(gcpu->guest, start, start, size, attr.uint32);
		break;
	case REMOVE:
		assert_mapping_status(gcpu, start, size, TRUE);

		gpm_remove_mapping(gcpu->guest, start, size);

		/* first flush ept TLB on local cpu */
		asm_invept(gcpu->guest->eptp, INVEPT_TYPE_SINGLE_CONTEXT);

		/* if with smp, flush ept TLB on remote cpus */
#if MAX_CPU_NUM > 1
		if ((host_cpu_num > 1) && flush_all_cpus)
			ipc_exec_on_all_other_cpus(flush_ept, (void*)gcpu->guest->eptp);
#endif
		break;
	default:
		print_panic("unknown action of EPT update\n");
		break;
	}

	return;
}

void ept_update_install(uint32_t guest_id)
{
	vmx_ept_vpid_cap_t ept_vpid;

	ept_vpid.uint64 = get_ept_vpid_cap();
	VMM_ASSERT_EX(ept_vpid.bits.invept,
			"invept is not support.\n");
	VMM_ASSERT_EX(ept_vpid.bits.invept_single_ctx,
			"single context invept type is not supported.\n");

	vmcall_register(guest_id, VMCALL_EPT_UPDATE, trusty_vmcall_ept_update);
}
