/*******************************************************************************
* Copyright (c) 2015 Intel Corporation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/
#include "vmm_asm.h"
#include "vmm_base.h"
#include "vmm_arch.h"
#include "evmm_desc.h"
#include "ldr_dbg.h"
#include "trusty_info.h"

#define TRUSTY_BOOT_NULL     (0x00)
#define TRUSTY_BOOT_CS       (0x08)
#define TRUSTY_BOOT_DS       (0x10)

void fill_code32_seg(segment_t *ss, uint16_t sel)
{
	ss->base = 0;
	ss->limit = 0xffffffff;
	ss->attributes = 0xc09b;
	ss->selector = sel;
}

void fill_code64_seg(segment_t *ss, uint16_t sel)
{
	ss->base = 0;
	ss->limit = 0xffffffff;
	ss->attributes = 0xa09b;
	ss->selector = sel;
}

void fill_data_seg(segment_t *ss, uint16_t sel)
{
	ss->base = 0;
	ss->limit = 0xffffffff;
	ss->attributes = 0xc093;
	ss->selector = sel;
}

void fill_tss_seg(segment_t *ss, uint16_t sel)
{
	ss->base = 0;
	ss->limit = 0xffffffff;
	ss->attributes = 0x808b;
	/* it is ok for TR to be NULL while the attribute is not 0.
	 * vmentry will not check it and guest OS will set the correct TR later */
	ss->selector = sel;
}

boolean_t trusty_gcpu_setup(evmm_desc_t *xd)
{
	trusty_desc_t *trusty_desc = &xd->trusty_desc;

	/* Guest OS will set the correct GDTR during boot stage */;
	trusty_desc->gcpu0_state.gdtr.base  = 0;
	trusty_desc->gcpu0_state.gdtr.limit = 0;

	trusty_desc->gcpu0_state.cr0 = 0x11;
	trusty_desc->gcpu0_state.cr3 = 0;
	trusty_desc->gcpu0_state.cr4 = 0;

	trusty_desc->gcpu0_state.msr_efer = 0x0;
	fill_code32_seg(&trusty_desc->gcpu0_state.segment[SEG_CS], TRUSTY_BOOT_CS);

	/* it is ok for idtr to be 0. vmentry will not check it
	 * and guest OS will set the correct idtr later */
	trusty_desc->gcpu0_state.idtr.base  = (uint64_t)0;
	trusty_desc->gcpu0_state.idtr.limit = (uint16_t)0;

	trusty_desc->gcpu0_state.segment[SEG_LDTR].attributes = 0x010000;

	fill_data_seg(&trusty_desc->gcpu0_state.segment[SEG_DS], TRUSTY_BOOT_DS);
	fill_data_seg(&trusty_desc->gcpu0_state.segment[SEG_ES], TRUSTY_BOOT_DS);
	fill_data_seg(&trusty_desc->gcpu0_state.segment[SEG_FS], TRUSTY_BOOT_DS);
	fill_data_seg(&trusty_desc->gcpu0_state.segment[SEG_GS], TRUSTY_BOOT_DS);
	fill_data_seg(&trusty_desc->gcpu0_state.segment[SEG_SS], TRUSTY_BOOT_DS);
	fill_tss_seg(&trusty_desc->gcpu0_state.segment[SEG_TR], TRUSTY_BOOT_NULL);

	/* The guest RIP will be set in core since the relocation is done in core */
	//trusty_desc->gcpu0_state.rip = 0;

	trusty_desc->gcpu0_state.gp_reg[REG_RAX] = 0;
	trusty_desc->gcpu0_state.gp_reg[REG_RBX] = 0;

	/* Trusty environment setup */
	/* Stack resides at top of IMR region */
	trusty_desc->gcpu0_state.gp_reg[REG_RSP] = trusty_desc->lk_file.runtime_addr + trusty_desc->lk_file.runtime_total_size;
	/*
	 * Trusty startup space just right after
	 * memory sapce of trusty_device_info_t in IMR
	 */
	trusty_desc->gcpu0_state.gp_reg[REG_RDI] = (uint64_t)(trusty_desc->lk_file.runtime_addr + sizeof(trusty_device_info_t));

	trusty_desc->gcpu0_state.gp_reg[REG_RSI] = 0;

	trusty_desc->gcpu0_state.rflags = 0x3002;

	return TRUE;
}
