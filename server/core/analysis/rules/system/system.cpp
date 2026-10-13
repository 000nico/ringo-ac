#include "core/system/system.hpp"
#include "core/analysis/engine.hpp"

RULE_RESULT rule_secure_boot(const ENVIRONMENT_REPORT& r) {
    const auto& s = r.secure_boot;

    if(disagree(s.nt_query_enabled, s.uefi_var))
        return found("spoofing_detection", "secure boot: NtQuery vs UEFI variable disagree");
    if(disagree(s.nt_query_enabled, s.registry))
        return found("spoofing_detection", "secure boot: NtQuery vs Registry disagree");
    if(disagree(s.uefi_var, s.registry))
        return found("spoofing_detection", "secure boot: UEFI Variable vs Registry disagree");

    bool any_enabled = (s.nt_query_enabled.valid && s.nt_query_enabled.value)
                    || (s.uefi_var.valid && s.uefi_var.value)
                    || (s.registry.valid && s.registry.value);
    if (any_enabled && s.nt_query_capable.valid && !s.nt_query_capable.value)
        return found("spoofing_detection", "secure boot enabled but hardware not capable");

    return no_finding();
}

RULE_RESULT rule_hvci(const ENVIRONMENT_REPORT& r) {
    const auto& hvci = r.hvci.code_integrity;
    const auto& vbs_hvci = r.vbs.nt_query_hvci;
    const auto& secure_kernel = r.vbs.nt_query_secure_kernel;

    if (disagree(hvci, vbs_hvci))
        return found("spoofing_detection", "hvci: code integrity vs vbs query disagree");

    bool hvci_on = (hvci.valid && hvci.value) || (vbs_hvci.valid && vbs_hvci.value);
    if (hvci_on && secure_kernel.valid && !secure_kernel.value)
        return found("spoofing_detection", "hvci enabled but secure kernel not running");

    return no_finding();
}


RULE_RESULT rule_vbs(const ENVIRONMENT_REPORT& r) {
    const auto& secure_kernel = r.vbs.nt_query_secure_kernel;
    const auto& hv_nt         = r.hypervisor.nt_query;
    const auto& hv_cpuid      = r.hypervisor.cpuid;

    bool vbs_on = secure_kernel.valid && secure_kernel.value;

    if (vbs_on && hv_cpuid.valid && !hv_cpuid.value)
        return found("spoofing_detection", "vbs running but cpuid reports no hypervisor");
    if (vbs_on && hv_nt.valid && !hv_nt.value)
        return found("spoofing_detection", "vbs running but NtQuery reports no hypervisor");

    return no_finding();
}

RULE_RESULT rule_test_signing(const ENVIRONMENT_REPORT& r) {
    const auto& ts = r.test_signing.code_integrity;

    if (ts.valid && ts.value)
        return found("test_signing_enabled", "test signing mode is enabled");

    return no_finding();
}

RULE_RESULT rule_hypervisor(const ENVIRONMENT_REPORT& r) {
    const auto& nt    = r.hypervisor.nt_query;
    const auto& cpuid = r.hypervisor.cpuid;

    if (disagree(nt, cpuid))
        return found("spoofing_detection", "hypervisor: NtQuery vs CPUID disagree");

    bool hv_present = (nt.valid && nt.value) || (cpuid.valid && cpuid.value);
    bool vbs_running = r.vbs.nt_query_secure_kernel.valid && r.vbs.nt_query_secure_kernel.value;

    if (hv_present && !vbs_running)
        return found("hypervisor_detected", "hypervisor present without VBS");

    return no_finding();
}

RULE_RESULT rule_kernel_debugging(const ENVIRONMENT_REPORT& r) {
    const auto& enabled     = r.kernel_debugging.nt_query_enabled;
    const auto& not_present = r.kernel_debugging.nt_query_not_present;

    if (enabled.valid && not_present.valid && enabled.value == not_present.value)
        return found("spoofing_detection", "kernel debugging: enabled and not_present are inconsistent");

    if (enabled.valid && enabled.value)
        return found("debugger_present", "kernel debugger is enabled");

    return no_finding();
}

RULE_RESULT rule_tpm(const ENVIRONMENT_REPORT& r) {
    const auto& t20 = r.tpm.tpm_2_0;
    const auto& t12 = r.tpm.tpm_1_2;

    // los dos a la vez es imposible
    if (t20.valid && t20.value && t12.valid && t12.value)
        return found("spoofing_detection", "tpm: both 1.2 and 2.0 reported");

    return no_finding();
}