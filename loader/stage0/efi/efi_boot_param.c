/*
 * Copyright (c) 2015-2019 Intel Corporation.
 * All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include "vmm_asm.h"
#include "vmm_base.h"
#include "stage0_asm.h"
#include "ldr_dbg.h"
#include "efi_boot_param.h"
#include "stage0_lib.h"
#include "device_sec_info.h"

#include "lib/util.h"

typedef struct {
	evmm_desc_t xd;
	device_sec_info_v0_t dev_sec_info;
} evmm_payload_t;

typedef struct {
	uint8_t image_load[EVMM_PKG_BIN_SIZE];
	uint8_t stage1[STAGE1_IMG_SIZE];
	evmm_payload_t payload;
} memory_layout_t;

_Static_assert(sizeof(evmm_payload_t) <= EVMM_PAYLOAD_SIZE,
	       "EVMM_PAYLOAD_SIZE is not big enough to hold evmm_payload_t!");

static boolean_t fill_device_sec_info(device_sec_info_v0_t *dev_sec_info, tos_startup_info_t *p_startup_info)
{
	uint32_t i;

	if (p_startup_info->num_seeds > BOOTLOADER_SEED_MAX_ENTRIES) {
		print_panic("Number of seeds exceeds predefined max number!\n");
		return FALSE;
	}
	dev_sec_info->num_seeds = p_startup_info->num_seeds;

	dev_sec_info->size_of_this_struct = sizeof(device_sec_info_v0_t);
	dev_sec_info->version = 0;

	/* in manufacturing mode | secure boot disabled | production seed */
	dev_sec_info->flags = 0x1 | 0x0 | 0x0;
	dev_sec_info->platform = 4; /* Brillo platform */

	/* copy seed_list from startup_info to dev_sec_info */
	memset(dev_sec_info->dseed_list, 0, sizeof(dev_sec_info->dseed_list));
	for (i = 0; i < (p_startup_info->num_seeds); i++) {
		dev_sec_info->dseed_list[i].cse_svn = p_startup_info->seed_list[i].cse_svn;
		memcpy(dev_sec_info->dseed_list[i].seed,
				p_startup_info->seed_list[i].seed,
				BUP_MKHI_BOOTLOADER_SEED_LEN);
	}

	/* copy serial info from startup_info to dev_sec_info */
	memcpy(dev_sec_info->serial, p_startup_info->serial, MMC_PROD_NAME_WITH_PSN_LEN);

	/* copy RPMB keys from startup_info to dev_sec_info */
	memcpy(dev_sec_info->rpmb_key, p_startup_info->rpmb_key, RPMB_MAX_PARTITION_NUMBER*64);

	/* clear the seed */
	memset(p_startup_info->seed_list, 0, sizeof(p_startup_info->seed_list));
	barrier();

	/* clear the RPMB key */
	memset(p_startup_info->rpmb_key, 0, RPMB_MAX_PARTITION_NUMBER*64);
	barrier();

	return TRUE;
}

evmm_desc_t *boot_params_parse(uint64_t tos_startup_info, uint64_t loader_addr)
{
	tos_startup_info_t *p_startup_info = (tos_startup_info_t *)tos_startup_info;
	memory_layout_t *loader_mem;
	evmm_desc_t *evmm_desc;
	device_sec_info_v0_t *dev_sec_info;
	uint64_t barrier_size;

	if(!p_startup_info) {
		print_panic("p_startup_info is NULL\n");
		return NULL;
	}

	if (p_startup_info->version != TOS_STARTUP_VERSION ||
		p_startup_info->size != sizeof(tos_startup_info_t)) {
		print_panic("TOS version/size not match\n");
		return NULL;
	}

	loader_mem = (memory_layout_t *) loader_addr;
	evmm_desc = &(loader_mem->payload.xd);
	memset(evmm_desc, 0, sizeof(evmm_desc_t));

	/* get evmm/stage1 runtime_addr/total_size */
	barrier_size = calulate_barrier_size(p_startup_info->vmm_mem_size, MINIMAL_EVMM_RT_SIZE);
	if (barrier_size == (uint64_t)-1) {
		print_panic("vmm mem size is smaller than %u\n", MINIMAL_EVMM_RT_SIZE);
		return NULL;
	}

	evmm_desc->evmm_file.barrier_size = barrier_size;
	evmm_desc->evmm_file.runtime_total_size = (uint64_t)p_startup_info->vmm_mem_size - 2 * barrier_size;
	evmm_desc->evmm_file.runtime_addr = (uint64_t)p_startup_info->vmm_mem_base + barrier_size;

	evmm_desc->stage1_file.runtime_addr = (uint64_t)loader_mem->stage1;
	evmm_desc->stage1_file.runtime_total_size = STAGE1_IMG_SIZE;

	evmm_desc->sipi_ap_wkup_addr = (uint64_t)p_startup_info->sipi_ap_wkup_addr;
#ifdef LIB_EFI_SERVICES
	evmm_desc->system_table_base = (uint64_t)p_startup_info->system_table_addr;
#endif

	evmm_desc->tsc_per_ms = TSC_PER_MS;
	evmm_desc->top_of_mem = TOP_OF_MEM;

	dev_sec_info = &(loader_mem->payload.dev_sec_info);
	memset(dev_sec_info, 0, sizeof(device_sec_info_v0_t));
	if (!fill_device_sec_info(dev_sec_info, p_startup_info)) {
		print_panic("failed to fill the device_sec_info\n");
		return NULL;
	}

#ifdef MODULE_TRUSTY_GUEST
	evmm_desc->trusty_desc.lk_file.runtime_addr = (uint64_t)p_startup_info->trusty_mem_base;
	evmm_desc->trusty_desc.lk_file.runtime_total_size = ((uint64_t)(p_startup_info->trusty_mem_size));
	evmm_desc->trusty_desc.dev_sec_info = dev_sec_info;
#endif

#ifdef MODULE_TRUSTY_TEE
	barrier_size = calulate_barrier_size(p_startup_info->trusty_mem_size, MINIMAL_TEE_RT_SIZE);
	if (barrier_size == (uint64_t)-1) {
		print_panic("TEE mem size is smaller than %u\n", MINIMAL_TEE_RT_SIZE);
		return NULL;
	}

	evmm_desc->trusty_tee_desc.tee_file.barrier_size = barrier_size;
	evmm_desc->trusty_tee_desc.tee_file.runtime_total_size = (uint64_t)p_startup_info->trusty_mem_size - 2 * barrier_size;
	evmm_desc->trusty_tee_desc.tee_file.runtime_addr = (uint64_t)p_startup_info->trusty_mem_base + barrier_size;

	evmm_desc->trusty_tee_desc.dev_sec_info = dev_sec_info;
#endif
	return evmm_desc;
}
