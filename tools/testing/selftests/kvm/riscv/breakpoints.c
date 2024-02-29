// SPDX-License-Identifier: GPL-2.0
/*
 * RISC-V KVM breakpoint tests.
 *
 * Copyright 2024 Beijing ESWIN Computing Technology Co., Ltd.
 *
 */
#include "kvm_util.h"

#define PC(v) ((uint64_t)&(v))

extern unsigned char sw_bp;

static void guest_code(void)
{
	asm volatile("sw_bp: ebreak");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");

	GUEST_DONE();
}

int main(void)
{
	struct kvm_vm *vm;
	struct kvm_vcpu *vcpu;
	struct kvm_guest_debug debug;
	uint64_t pc;

	TEST_REQUIRE(kvm_has_cap(KVM_CAP_SET_GUEST_DEBUG));

	vm = vm_create_with_one_vcpu(&vcpu, guest_code);

	memset(&debug, 0, sizeof(debug));
	debug.control = KVM_GUESTDBG_ENABLE;
	vcpu_guest_debug_set(vcpu, &debug);
	vcpu_run(vcpu);

	TEST_ASSERT_KVM_EXIT_REASON(vcpu, KVM_EXIT_DEBUG);

	vcpu_get_reg(vcpu, RISCV_CORE_REG(regs.pc), &pc);

	TEST_ASSERT_EQ(pc, PC(sw_bp));

	kvm_vm_free(vm);

	return 0;
}
