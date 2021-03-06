//
// Bareflank Hypervisor
// Copyright (C) 2015 Assured Information Security, Inc.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

#include <catch/catch.hpp>
#include <hippomocks.h>

#include <hve/arch/intel_x64/vmx/vmx.h>
#include <hve/arch/intel_x64/vmcs/vmcs.h>
#include <hve/arch/intel_x64/check/check.h>
#include <hve/arch/intel_x64/exit_handler/exit_handler.h>

#include <hve/arch/x64/gdt.h>
#include <hve/arch/x64/idt.h>

#include <intrinsics.h>
#include <bfnewdelete.h>

#include <memory_manager/arch/x64/map_ptr.h>
#include <memory_manager/arch/x64/root_page_table.h>
#include <memory_manager/memory_manager.h>

bfvmm::intel_x64::save_state_t g_save_state{};

std::map<uint32_t, uint64_t> g_msrs;
std::map<uint64_t, uint64_t> g_vmcs_fields;
std::map<uint32_t, uint32_t> g_eax_cpuid;
std::map<uint32_t, uint32_t> g_ebx_cpuid;
std::map<uint32_t, uint32_t> g_ecx_cpuid;

x64::rflags::value_type g_rflags = 0;
intel_x64::cr0::value_type g_cr0 = 0;
intel_x64::cr3::value_type g_cr3 = 0;
intel_x64::cr4::value_type g_cr4 = 0;
intel_x64::dr7::value_type g_dr7 = 0;

uint16_t g_es;
uint16_t g_cs;
uint16_t g_ss;
uint16_t g_ds;
uint16_t g_fs;
uint16_t g_gs;
uint16_t g_ldtr;
uint16_t g_tr;

::x64::gdt_reg::reg_t g_gdtr{};
::x64::idt_reg::reg_t g_idtr{};

std::vector<bfvmm::x64::gdt::segment_descriptor_type> g_gdt = {
    0x0,
    0xFF7FFFFFFFFFFFFF,
    0xFF7FFFFFFFFFFFFF,
    0xFF7FFFFFFFFFFFFF,
    0xFF7FFFFFFFFFFFFF,
    0xFFFFFFFFFFFFFFFF,
    0xFFFF8FFFFFFFFFFF,
    0x00000000FFFFFFFF,
};

std::vector<bfvmm::x64::idt::interrupt_descriptor_type> g_idt = {
    0xFFFFFFFFFFFFFFFF,
    0xFFFFFFFFFFFFFFFF,
    0xFFFFFFFFFFFFFFFF,
    0xFFFFFFFFFFFFFFFF
};

alignas(0x1000) static char g_map[100];

bool g_virt_to_phys_fails = false;
bool g_phys_to_virt_fails = false;
bool g_vmload_fails = false;
bool g_vmlaunch_fails = false;
bool g_vmxon_fails = false;
bool g_vmxoff_fails = false;
bool g_write_cr4_fails = false;

uint64_t g_test_addr = 0U;
uint64_t g_virt_apic_addr = 0U;
uint64_t g_virt_apic_mem[64] = {0U};
uint64_t g_vmcs_link_addr = 1U;
uint64_t g_vmcs_link_mem[1] = {0U};
uint64_t g_pdpt_addr = 2U;
uint64_t g_pdpt_mem[4] = {0U};

std::map<uint64_t, void *> g_mock_mem {
    {
        {g_virt_apic_addr, static_cast<void *>(&g_virt_apic_mem)},
        {g_vmcs_link_addr, static_cast<void *>(&g_vmcs_link_mem)},
        {g_pdpt_addr, static_cast<void *>(&g_pdpt_mem)}
    }};

extern "C" uint64_t
_read_msr(uint32_t addr) noexcept
{ return g_msrs[addr]; }

extern "C" void
_write_msr(uint32_t addr, uint64_t val) noexcept
{ g_msrs[addr] = val; }

extern "C" uint64_t
_read_cr0(void) noexcept
{ return g_cr0; }

extern "C" uint64_t
_read_cr3(void) noexcept
{ return g_cr3; }

extern "C" uint64_t
_read_cr4(void) noexcept
{ return g_cr4; }

extern "C" void
_write_cr0(uint64_t val) noexcept
{ g_cr0 = val; }

extern "C" void
_write_cr3(uint64_t val) noexcept
{ g_cr3 = val; }

extern "C" void
_write_cr4(uint64_t val) noexcept
{
    if (g_write_cr4_fails) {
        return;
    }

    g_cr4 = val;
}

extern "C" uint64_t
_read_dr7() noexcept
{ return g_dr7; }

extern "C" void
_write_dr7(uint64_t val) noexcept
{ g_dr7 = val; }

extern "C" uint64_t
_read_rflags(void) noexcept
{ return g_rflags; }

extern "C" void
_write_rflags(uint64_t val) noexcept
{ g_rflags = val; }

extern "C" uint16_t
_read_es() noexcept
{ return g_es; }

extern "C" uint16_t
_read_cs() noexcept
{ return g_cs; }

extern "C" uint16_t
_read_ss() noexcept
{ return g_ss; }

extern "C" uint16_t
_read_ds() noexcept
{ return g_ds; }

extern "C" uint16_t
_read_fs() noexcept
{ return g_fs; }

extern "C" uint16_t
_read_gs() noexcept
{ return g_gs; }

extern "C" uint16_t
_read_tr() noexcept
{ return g_tr; }

extern "C" uint16_t
_read_ldtr() noexcept
{ return g_ldtr; }

extern "C" void
_write_es(uint16_t val) noexcept
{ g_es = val; }

extern "C" void
_write_cs(uint16_t val) noexcept
{ g_cs = val; }

extern "C" void
_write_ss(uint16_t val) noexcept
{ g_ss = val; }

extern "C" void
_write_ds(uint16_t val) noexcept
{ g_ds = val; }

extern "C" void
_write_fs(uint16_t val) noexcept
{ g_fs = val; }

extern "C" void
_write_gs(uint16_t val) noexcept
{ g_gs = val; }

extern "C" void
_write_tr(uint16_t val) noexcept
{ g_tr = val; }

extern "C" void
_write_ldtr(uint16_t val) noexcept
{ g_ldtr = val; }

extern "C" void
_read_gdt(void *gdt_reg) noexcept
{ *static_cast<::x64::gdt_reg::reg_t *>(gdt_reg) = g_gdtr; }

extern "C" void
_read_idt(void *idt_reg) noexcept
{ *static_cast<::x64::idt_reg::reg_t *>(idt_reg) = g_idtr; }

extern "C" void
_stop() noexcept
{ }

extern "C" void
_wbinvd() noexcept
{ }

extern "C" void
_invlpg(const void *addr) noexcept
{ bfignored(addr); }

extern "C" void
_cpuid(void *eax, void *ebx, void *ecx, void *edx) noexcept
{
    bfignored(eax);
    bfignored(ebx);
    bfignored(ecx);
    bfignored(edx);
}

extern "C" uint32_t
_cpuid_eax(uint32_t val) noexcept
{ return g_eax_cpuid[val]; }

extern "C" uint32_t
_cpuid_subebx(uint32_t val, uint32_t sub) noexcept
{ bfignored(sub); return g_ebx_cpuid[val]; }

extern "C" uint32_t
_cpuid_ecx(uint32_t val) noexcept
{ return g_ecx_cpuid[val]; }

extern "C" bool
_vmread(uint64_t field, uint64_t *value) noexcept
{
    *value = g_vmcs_fields[field];
    return true;
}

extern "C" bool
_vmwrite(uint64_t field, uint64_t value) noexcept
{
    g_vmcs_fields[field] = value;
    return true;
}

extern "C" bool
_vmptrld(void *ptr) noexcept
{ (void)ptr; return !g_vmload_fails; }

extern "C" bool
_vmlaunch_demote() noexcept
{ return !g_vmlaunch_fails; }

extern "C" bool
_vmxon(void *ptr) noexcept
{ bfignored(ptr); return !g_vmxon_fails; }

extern "C" bool
_vmxoff() noexcept
{ return !g_vmxoff_fails; }

extern "C" uint64_t
thread_context_cpuid(void)
{ return 0; }

extern "C" uint64_t
thread_context_tlsptr(void)
{ return 0; }

uintptr_t
virtptr_to_physint(void *ptr)
{
    bfignored(ptr);

    if (g_virt_to_phys_fails) {
        throw gsl::fail_fast("");
    }

    return 0x0000000ABCDEF0000;
}

void *
physint_to_virtptr(uintptr_t ptr)
{
    bfignored(ptr);

    if (g_phys_to_virt_fails) {
        return nullptr;
    }

    return static_cast<void *>(g_mock_mem[g_test_addr]);
}

extern "C" void vmcs_launch(
    bfvmm::intel_x64::save_state_t *save_state) noexcept
{ bfignored(save_state); }

extern "C" void vmcs_promote(
    bfvmm::intel_x64::save_state_t *save_state, const void *gdt) noexcept
{ bfignored(save_state); bfignored(gdt); }

extern "C" void vmcs_resume(
    bfvmm::intel_x64::save_state_t *save_state) noexcept
{ bfignored(save_state); }

extern "C" void exit_handler_entry(void)
{ }

void
setup_msrs()
{
    g_msrs[intel_x64::msrs::ia32_vmx_basic::addr] = (1ULL << 55) | (6ULL << 50);

    g_msrs[intel_x64::msrs::ia32_vmx_true_pinbased_ctls::addr] = 0xFFFFFFFF00000000UL;
    g_msrs[intel_x64::msrs::ia32_vmx_true_procbased_ctls::addr] = 0xFFFFFFFF00000000UL;
    g_msrs[intel_x64::msrs::ia32_vmx_true_exit_ctls::addr] = 0xFFFFFFFF00000000UL;
    g_msrs[intel_x64::msrs::ia32_vmx_true_entry_ctls::addr] = 0xFFFFFFFF00000000UL;
    g_msrs[intel_x64::msrs::ia32_vmx_procbased_ctls2::addr] = 0xFFFFFFFF00000000UL;

    g_msrs[intel_x64::msrs::ia32_vmx_cr0_fixed0::addr] = 0U;
    g_msrs[intel_x64::msrs::ia32_vmx_cr0_fixed1::addr] = 0xFFFFFFFFFFFFFFFF;
    g_msrs[intel_x64::msrs::ia32_vmx_cr4_fixed0::addr] = 0U;
    g_msrs[intel_x64::msrs::ia32_vmx_cr4_fixed1::addr] = 0xFFFFFFFFFFFFFFFF;

    g_msrs[intel_x64::msrs::ia32_efer::addr] = intel_x64::msrs::ia32_efer::lma::mask;
    g_msrs[intel_x64::msrs::ia32_feature_control::addr] = (0x1ULL << 0);
}

void
setup_cpuid()
{
    g_ecx_cpuid[intel_x64::cpuid::feature_information::addr] = intel_x64::cpuid::feature_information::ecx::vmx::mask;
}

void
setup_registers()
{
    g_cr0 = 0x0;
    g_cr3 = 0x0;
    g_cr4 = 0x0;
    g_rflags = 0x0;
}

void
setup_gdt()
{
    auto limit = g_gdt.size() * sizeof(bfvmm::x64::gdt::segment_descriptor_type) - 1;

    g_gdtr.base = reinterpret_cast<uint64_t>(&g_gdt.at(0));
    g_gdtr.limit = gsl::narrow_cast<uint16_t>(limit);
}

void
setup_idt()
{
    auto limit = g_idt.size() * sizeof(bfvmm::x64::idt::interrupt_descriptor_type) - 1;

    g_idtr.base = reinterpret_cast<uint64_t>(&g_idt.at(0));
    g_idtr.limit = gsl::narrow_cast<uint16_t>(limit);
}

#ifdef _HIPPOMOCKS__ENABLE_CFUNC_MOCKING_SUPPORT

auto
setup_mm(MockRepository &mocks)
{
    auto mm = mocks.Mock<bfvmm::memory_manager>();
    mocks.OnCallFunc(bfvmm::memory_manager::instance).Return(mm);

    mocks.OnCall(mm, bfvmm::memory_manager::alloc_map).Return(static_cast<char *>(g_map));
    mocks.OnCall(mm, bfvmm::memory_manager::free_map);
    mocks.OnCall(mm, bfvmm::memory_manager::virtptr_to_physint).Do(virtptr_to_physint);
    mocks.OnCall(mm, bfvmm::memory_manager::physint_to_virtptr).Do(physint_to_virtptr);

    mocks.OnCallFunc(bfvmm::x64::map_with_cr3);
    mocks.OnCallFunc(bfvmm::x64::virt_to_phys_with_cr3).Return(0x42);

    return mm;
}

auto
setup_pt(MockRepository &mocks)
{
    auto pt = mocks.Mock<bfvmm::x64::root_page_table>();
    mocks.OnCallFunc(bfvmm::x64::root_pt).Return(pt);

    mocks.OnCall(pt, bfvmm::x64::root_page_table::map_4k);
    mocks.OnCall(pt, bfvmm::x64::root_page_table::unmap);
    mocks.OnCall(pt, bfvmm::x64::root_page_table::cr3).Return(0x000000ABCDEF0000);

    return pt;
}

#endif
