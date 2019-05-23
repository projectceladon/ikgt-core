/*
 * Copyright (c) 2015-2019 Intel Corporation.
 * All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include "gcpu.h"
#include "vmcs.h"
#include "gpm.h"
#include "nested_vt_internal.h"

#define VMENTRY_COPY_FROM_GVMCS 0
#define VMENTRY_NO_CHANGE       1
#define VMENTRY_OTHERS          2

typedef struct {
	uint32_t handle_type:      2;
	uint32_t need_save_hvmcs:  1;
	uint32_t rsvd:            29;
} vmentry_vmcs_handle_t;

static vmentry_vmcs_handle_t vmentry_vmcs_handle_array[] = {
	{ VMENTRY_COPY_FROM_GVMCS, TRUE,  0 }, //VMCS_VPID
	{ VMENTRY_OTHERS,          TRUE,  0 }, //VMCS_IO_BITMAP_A
	{ VMENTRY_OTHERS,          TRUE,  0 }, //VMCS_IO_BITMAP_B
	{ VMENTRY_OTHERS,          TRUE,  0 }, //VMCS_MSR_BITMAP
	{ VMENTRY_OTHERS,          TRUE,  0 }, //VMCS_TSC_OFFSET
	{ VMENTRY_OTHERS,          TRUE,  0 }, //VMCS_VIRTUAL_APIC_ADDR
	{ VMENTRY_COPY_FROM_GVMCS, TRUE,  0 }, //VMCS_APIC_ACCESS_ADDR
	{ VMENTRY_COPY_FROM_GVMCS, TRUE,  0 }, //VMCS_POST_INTR_NOTI_VECTOR
	{ VMENTRY_OTHERS,          TRUE,  0 }, //VMCS_POST_INTR_DESC_ADDR
	{ VMENTRY_COPY_FROM_GVMCS, TRUE,  0 }, //VMCS_EOI_EXIT_BITMAP0
	{ VMENTRY_COPY_FROM_GVMCS, TRUE,  0 }, //VMCS_EOI_EXIT_BITMAP1
	{ VMENTRY_COPY_FROM_GVMCS, TRUE,  0 }, //VMCS_EOI_EXIT_BITMAP2
	{ VMENTRY_COPY_FROM_GVMCS, TRUE,  0 }, //VMCS_EOI_EXIT_BITMAP3
	{ VMENTRY_COPY_FROM_GVMCS, TRUE,  0 }, //VMCS_TPR_THRESHOLD
	{ VMENTRY_OTHERS,          TRUE,  0 }, //VMCS_EPTP_ADDRESS
	{ VMENTRY_COPY_FROM_GVMCS, TRUE,  0 }, //VMCS_XSS_EXIT_BITMAP
	{ VMENTRY_OTHERS,          TRUE,  0 }, //VMCS_PIN_CTRL
	{ VMENTRY_OTHERS,          TRUE,  0 }, //VMCS_PROC_CTRL1
	{ VMENTRY_OTHERS,          TRUE,  0 }, //VMCS_PROC_CTRL2
	{ VMENTRY_NO_CHANGE,       FALSE, 0 }, //VMCS_EXIT_CTRL
	{ VMENTRY_COPY_FROM_GVMCS, TRUE,  0 }, //VMCS_CR0_MASK
	{ VMENTRY_COPY_FROM_GVMCS, TRUE,  0 }, //VMCS_CR4_MASK
	{ VMENTRY_COPY_FROM_GVMCS, TRUE,  0 }, //VMCS_CR0_SHADOW
	{ VMENTRY_COPY_FROM_GVMCS, TRUE,  0 }, //VMCS_CR4_SHADOW
	{ VMENTRY_COPY_FROM_GVMCS, TRUE,  0 }, //VMCS_CR3_TARGET0
	{ VMENTRY_COPY_FROM_GVMCS, TRUE,  0 }, //VMCS_CR3_TARGET1
	{ VMENTRY_COPY_FROM_GVMCS, TRUE,  0 }, //VMCS_CR3_TARGET2
	{ VMENTRY_COPY_FROM_GVMCS, TRUE,  0 }, //VMCS_CR3_TARGET3
	{ VMENTRY_OTHERS,          TRUE,  0 }, //VMCS_LINK_PTR
	{ VMENTRY_NO_CHANGE,       FALSE, 0 }, //VMCS_HOST_CR0
	{ VMENTRY_NO_CHANGE,       FALSE, 0 }, //VMCS_HOST_CR3
	{ VMENTRY_NO_CHANGE,       FALSE, 0 }, //VMCS_HOST_CR4
	{ VMENTRY_NO_CHANGE,       FALSE, 0 }, //VMCS_HOST_ES_SEL
	{ VMENTRY_NO_CHANGE,       FALSE, 0 }, //VMCS_HOST_CS_SEL
	{ VMENTRY_NO_CHANGE,       FALSE, 0 }, //VMCS_HOST_SS_SEL
	{ VMENTRY_NO_CHANGE,       FALSE, 0 }, //VMCS_HOST_DS_SEL
	{ VMENTRY_NO_CHANGE,       FALSE, 0 }, //VMCS_HOST_FS_SEL
	{ VMENTRY_NO_CHANGE,       FALSE, 0 }, //VMCS_HOST_FS_BASE
	{ VMENTRY_NO_CHANGE,       FALSE, 0 }, //VMCS_HOST_GS_SEL
	{ VMENTRY_NO_CHANGE,       FALSE, 0 }, //VMCS_HOST_GS_BASE
	{ VMENTRY_NO_CHANGE,       FALSE, 0 }, //VMCS_HOST_TR_SEL
	{ VMENTRY_NO_CHANGE,       FALSE, 0 }, //VMCS_HOST_TR_BASE
	{ VMENTRY_NO_CHANGE,       FALSE, 0 }, //VMCS_HOST_GDTR_BASE
	{ VMENTRY_NO_CHANGE,       FALSE, 0 }, //VMCS_HOST_IDTR_BASE
	{ VMENTRY_NO_CHANGE,       FALSE, 0 }, //VMCS_HOST_RSP
	{ VMENTRY_NO_CHANGE,       FALSE, 0 }, //VMCS_HOST_RIP
	{ VMENTRY_NO_CHANGE,       FALSE, 0 }, //VMCS_HOST_PAT
	{ VMENTRY_NO_CHANGE,       FALSE, 0 }, //VMCS_HOST_EFER
	{ VMENTRY_NO_CHANGE,       FALSE, 0 }, //VMCS_HOST_PERF_G_CTRL
	{ VMENTRY_NO_CHANGE,       FALSE, 0 }, //VMCS_EXIT_MSR_STORE_COUNT
	{ VMENTRY_NO_CHANGE,       FALSE, 0 }, //VMCS_EXIT_MSR_STORE_ADDR
	{ VMENTRY_NO_CHANGE,       FALSE, 0 }, //VMCS_EXIT_MSR_LOAD_COUNT
	{ VMENTRY_NO_CHANGE,       FALSE, 0 }, //VMCS_EXIT_MSR_LOAD_ADDR
	{ VMENTRY_OTHERS,          TRUE,  0 }, //VMCS_ENTRY_MSR_LOAD_COUNT
	{ VMENTRY_OTHERS,          TRUE,  0 }, //VMCS_ENTRY_MSR_LOAD_ADDR
	{ VMENTRY_COPY_FROM_GVMCS, TRUE,  0 }, //VMCS_EXCEPTION_BITMAP
	{ VMENTRY_NO_CHANGE,       FALSE, 0 }, //VMCS_HOST_SYSENTER_CS
	{ VMENTRY_NO_CHANGE,       FALSE, 0 }, //VMCS_HOST_SYSENTER_ESP
	{ VMENTRY_NO_CHANGE,       FALSE, 0 }, //VMCS_HOST_SYSENTER_EIP
	{ VMENTRY_COPY_FROM_GVMCS, TRUE,  0 }, //VMCS_CR3_TARGET_COUNT
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_ENTRY_INTR_INFO
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_DBGCTL
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_INTERRUPTIBILITY
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_INTERRUPT_STATUS
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_PEND_DBG_EXCEPTION
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_ENTRY_ERR_CODE
	{ VMENTRY_OTHERS,          TRUE,  0 }, //VMCS_ENTRY_CTRL
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_ENTRY_INSTR_LEN
	{ VMENTRY_OTHERS,          TRUE,  0 }, //VMCS_PREEMPTION_TIMER
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_PAT
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_EFER
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_PERF_G_CTRL
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_PDPTR0
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_PDPTR1
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_PDPTR2
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_PDPTR3
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_CR0
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_CR3
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_CR4
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_DR7
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_GDTR_BASE
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_GDTR_LIMIT
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_IDTR_BASE
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_IDTR_LIMIT
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_ACTIVITY_STATE
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_SYSENTER_CS
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_SYSENTER_ESP
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_SYSENTER_EIP
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_ES_SEL
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_ES_BASE
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_ES_LIMIT
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_ES_AR
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_CS_SEL
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_CS_BASE
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_CS_LIMIT
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_CS_AR
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_SS_SEL
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_SS_BASE
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_SS_LIMIT
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_SS_AR
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_DS_SEL
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_DS_BASE
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_DS_LIMIT
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_DS_AR
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_FS_SEL
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_FS_BASE
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_FS_LIMIT
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_FS_AR
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_GS_SEL
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_GS_BASE
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_GS_LIMIT
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_GS_AR
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_LDTR_SEL
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_LDTR_BASE
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_LDTR_LIMIT
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_LDTR_AR
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_TR_SEL
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_TR_BASE
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_TR_LIMIT
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_TR_AR
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_RSP
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_RIP
	{ VMENTRY_COPY_FROM_GVMCS, FALSE, 0 }, //VMCS_GUEST_RFLAGS
	{ VMENTRY_NO_CHANGE,       FALSE, 0 }, //VMCS_GUEST_PHY_ADDR
	{ VMENTRY_NO_CHANGE,       FALSE, 0 }, //VMCS_GUEST_LINEAR_ADDR
	{ VMENTRY_NO_CHANGE,       FALSE, 0 }, //VMCS_INSTR_ERROR
	{ VMENTRY_NO_CHANGE,       FALSE, 0 }, //VMCS_EXIT_REASON
	{ VMENTRY_NO_CHANGE,       FALSE, 0 }, //VMCS_EXIT_INT_INFO
	{ VMENTRY_NO_CHANGE,       FALSE, 0 }, //VMCS_EXIT_INT_ERR_CODE
	{ VMENTRY_NO_CHANGE,       FALSE, 0 }, //VMCS_IDT_VECTOR_INFO
	{ VMENTRY_NO_CHANGE,       FALSE, 0 }, //VMCS_IDT_VECTOR_ERR_CODE
	{ VMENTRY_NO_CHANGE,       FALSE, 0 }, //VMCS_EXIT_INSTR_LEN
	{ VMENTRY_NO_CHANGE,       FALSE, 0 }, //VMCS_EXIT_INSTR_INFO
	{ VMENTRY_NO_CHANGE,       FALSE, 0 }, //VMCS_EXIT_QUAL
};

_Static_assert(sizeof(vmentry_vmcs_handle_array)/sizeof(vmentry_vmcs_handle_t) == VMCS_FIELD_COUNT,
			"Nested VT: vmentry vmcs handle count not aligned with VMCS fields count!");

static void handle_vmcs_io_bitmap_a(guest_cpu_handle_t gcpu, uint64_t *gvmcs)
{
	/* TODO: merge with hvmcs */
	vmcs_write(gcpu->vmcs, VMCS_IO_BITMAP_A, gvmcs[VMCS_IO_BITMAP_A]);
}

static void handle_vmcs_io_bitmap_b(guest_cpu_handle_t gcpu, uint64_t *gvmcs)
{
	/* TODO: merge with hvmcs */
	vmcs_write(gcpu->vmcs, VMCS_IO_BITMAP_B, gvmcs[VMCS_IO_BITMAP_B]);
}

static void handle_vmcs_msr_bitmap(guest_cpu_handle_t gcpu, uint64_t *gvmcs)
{
	/* TODO: merge with hvmcs */
	vmcs_write(gcpu->vmcs, VMCS_MSR_BITMAP, gvmcs[VMCS_MSR_BITMAP]);
}

static void handle_vmcs_tsc_offset(guest_cpu_handle_t gcpu, uint64_t *gvmcs)
{
	vmcs_write(gcpu->vmcs, VMCS_TSC_OFFSET, gvmcs[VMCS_TSC_OFFSET] + vmcs_read(gcpu->vmcs, VMCS_TSC_OFFSET));
}

static void handle_vmcs_virtual_apic_addr(guest_cpu_handle_t gcpu, uint64_t *gvmcs)
{
	uint64_t hpa;

	VMM_ASSERT(gpm_gpa_to_hpa(gcpu->guest, gvmcs[VMCS_VIRTUAL_APIC_ADDR], &hpa, NULL));

	vmcs_write(gcpu->vmcs, VMCS_VIRTUAL_APIC_ADDR, hpa);
}

static void handle_vmcs_post_intr_desc_addr(guest_cpu_handle_t gcpu, uint64_t *gvmcs)
{
	uint64_t hpa;

	VMM_ASSERT(gpm_gpa_to_hpa(gcpu->guest, gvmcs[VMCS_POST_INTR_DESC_ADDR], &hpa, NULL));

	vmcs_write(gcpu->vmcs, VMCS_POST_INTR_DESC_ADDR, hpa);
}

static void handle_vmcs_eptp_addr(guest_cpu_handle_t gcpu, uint64_t *gvmcs)
{
	uint64_t hpa;

	VMM_ASSERT(gpm_gpa_to_hpa(gcpu->guest, gvmcs[VMCS_EPTP_ADDRESS], &hpa, NULL));

	/* TODO: merge with hvmcs */
	vmcs_write(gcpu->vmcs, VMCS_EPTP_ADDRESS, hpa);
}

static void handle_vmcs_pin_ctrl(guest_cpu_handle_t gcpu, uint64_t *gvmcs)
{
	/* TODO: merge with hvmcs */
	vmcs_write(gcpu->vmcs, VMCS_PIN_CTRL, gvmcs[VMCS_PIN_CTRL]);
}

static void handle_vmcs_proc_ctrl1(guest_cpu_handle_t gcpu, uint64_t *gvmcs)
{
	/* TODO: merge with hvmcs */
	vmcs_write(gcpu->vmcs, VMCS_PROC_CTRL1, gvmcs[VMCS_PROC_CTRL1]);
}

static void handle_vmcs_proc_ctrl2(guest_cpu_handle_t gcpu, uint64_t *gvmcs)
{
	/* TODO: merge with hvmcs */
	vmcs_write(gcpu->vmcs, VMCS_PROC_CTRL2, gvmcs[VMCS_PROC_CTRL2]);
}

static void handle_vmcs_link_ptr(guest_cpu_handle_t gcpu, uint64_t *gvmcs)
{
	uint64_t hpa;

	VMM_ASSERT(gpm_gpa_to_hpa(gcpu->guest, gvmcs[VMCS_LINK_PTR], &hpa, NULL));

	vmcs_write(gcpu->vmcs, VMCS_LINK_PTR, hpa);
}

static void handle_vmcs_entry_msr_load_count(guest_cpu_handle_t gcpu, uint64_t *gvmcs)
{
	/* TODO: merge with hvmcs */
	vmcs_write(gcpu->vmcs, VMCS_ENTRY_MSR_LOAD_COUNT, gvmcs[VMCS_ENTRY_MSR_LOAD_COUNT]);
}

static void handle_vmcs_entry_msr_load_addr(guest_cpu_handle_t gcpu, uint64_t *gvmcs)
{
	uint64_t hpa;

	VMM_ASSERT(gpm_gpa_to_hpa(gcpu->guest, gvmcs[VMCS_ENTRY_MSR_LOAD_ADDR], &hpa, NULL));

	/* TODO: merge with hvmcs */
	vmcs_write(gcpu->vmcs, VMCS_ENTRY_MSR_LOAD_ADDR, hpa);
}

static void handle_vmcs_entry_ctrl(guest_cpu_handle_t gcpu, uint64_t *gvmcs)
{
	/* TODO: merge with hvmcs */
	vmcs_write(gcpu->vmcs, VMCS_ENTRY_CTRL, gvmcs[VMCS_ENTRY_CTRL]);
}

static void handle_vmcs_preemption_timer(guest_cpu_handle_t gcpu, uint64_t *gvmcs)
{
	/* TODO: merge with hvmcs */
	vmcs_write(gcpu->vmcs, VMCS_PREEMPTION_TIMER, gvmcs[VMCS_PREEMPTION_TIMER]);
}

void emulate_vmentry(guest_cpu_handle_t gcpu)
{
	vmcs_obj_t vmcs;
	nestedvt_data_t *data;
	uint64_t *gvmcs;
	uint64_t *hvmcs;
	uint32_t i;

	vmcs = gcpu->vmcs;
	data = get_nestedvt_data(gcpu);
	gvmcs = data->gvmcs;
	hvmcs = data->hvmcs;

	/* Backup VMCS fields for Layer-0 */
	for (i = 0; i < VMCS_FIELD_COUNT; i++) {
		if (vmentry_vmcs_handle_array[i].need_save_hvmcs) {
			hvmcs[i] = vmcs_read(vmcs, i);
		}
	}

	/*
	 * Set VMCS fields according to handle type:
	 *     1. Copy from Layer-1's VMCS;
	 *     2. No change;
	 *     3. Others: will be handled specifically.
	 */
	for (i = 0; i < VMCS_FIELD_COUNT; i++) {
		switch (vmentry_vmcs_handle_array[i].handle_type) {
		case VMENTRY_COPY_FROM_GVMCS:
			vmcs_write(vmcs, i, gvmcs[i]);
			break;
		case VMENTRY_NO_CHANGE:
		case VMENTRY_OTHERS:
			break;
		default:
			print_panic("%s: Unknown handle type!\n", __func__);
			VMM_DEADLOOP();
			break;
		}
	}

	/* handle VMENTRY_OTHERS */
	handle_vmcs_io_bitmap_a(gcpu, gvmcs);
	handle_vmcs_io_bitmap_b(gcpu, gvmcs);
	handle_vmcs_msr_bitmap(gcpu, gvmcs);
	handle_vmcs_tsc_offset(gcpu, gvmcs);
	handle_vmcs_virtual_apic_addr(gcpu, gvmcs);
	handle_vmcs_post_intr_desc_addr(gcpu, gvmcs);
	handle_vmcs_eptp_addr(gcpu, gvmcs);
	handle_vmcs_pin_ctrl(gcpu, gvmcs);
	handle_vmcs_proc_ctrl1(gcpu, gvmcs);
	handle_vmcs_proc_ctrl2(gcpu, gvmcs);
	handle_vmcs_link_ptr(gcpu, gvmcs);
	handle_vmcs_entry_msr_load_count(gcpu, gvmcs);
	handle_vmcs_entry_msr_load_addr(gcpu, gvmcs);
	handle_vmcs_entry_ctrl(gcpu, gvmcs);
	handle_vmcs_preemption_timer(gcpu, gvmcs);
}
