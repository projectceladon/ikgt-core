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
#include "vmm_base.h"
#include "vmm_objects.h"
#include "vmexit.h"
#include "scheduler.h"
#include "guest.h"
#include "heap.h"
#include "gcpu.h"
#include "vmcs.h"
#include "vmx_cap.h"
#include "event.h"
#include "host_cpu.h"
#include "modules/vmcall.h"
#include "lib/util.h"
#include "dbg.h"
#include "stack_profile.h"
#include "time_profile.h"

void profile_init(void)
{
#ifdef TIME_PROFILE
	time_profile_init();
#endif

#ifdef STACK_PROFILE
	stack_profile_init();
#endif

	return;
}
