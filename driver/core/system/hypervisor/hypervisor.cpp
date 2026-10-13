#include "hypervisor.hpp"
#include "sdk/sdk.hpp"
#include <intrin.h>

BOOLEAN zw_system_hypervisor_information_check(BOOLEAN* p_is_hypervisor_present) {
	ULONG return_length = 0;

	SYSTEM_HYPERVISOR_INFORMATION shi = { 0 };
	NTSTATUS status = ZwQuerySystemInformation(SystemHypervisorInformation, &shi, sizeof(shi), &return_length);

	if (!NT_SUCCESS(status)) return false;
	else {
		*p_is_hypervisor_present = shi.HypervisorPresent;
		return TRUE;
	}
}

HYPERVISOR_CHECK get_hypervisor_info() {
	HYPERVISOR_CHECK hc = {};

	BOOLEAN hypervisor_present;
	if (zw_system_hypervisor_information_check(&hypervisor_present)) {
		hc.nt_query.valid = TRUE;
		hc.nt_query.value = hypervisor_present;
	}

	int cpu_info[4];
	__cpuid(cpu_info, 1);
	hc.cpuid.valid = TRUE;
	hc.cpuid.value = (cpu_info[2] >> 31) & 1;

	return hc;
}