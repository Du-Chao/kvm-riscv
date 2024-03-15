// SPDX-License-Identifier: GPL-2.0
/*
 * RISC-V KVM ebreak test.
 *
 * Copyright 2024 Beijing ESWIN Computing Technology Co., Ltd.
 *
 */
#include "kvm_util.h"

#define PC(v) ((uint64_t)&(v))

extern unsigned char sw_bp_1, sw_bp_2;

static void guest_code(void)
{
	/*
	 * nops are inserted to make it compatible
	 * with compressed instructions.
	 */
	asm volatile("sw_bp_1: ebreak\n"
		     "nop\n"
		     "sw_bp_2: ebreak\n"
		     "nop\n");

	GUEST_DONE();
}

int main(void)
{
	struct kvm_vm *vm;
	struct kvm_vcpu *vcpu;
	uint64_t pc;
	struct kvm_guest_debug debug = {
		.control = KVM_GUESTDBG_ENABLE,
	};
	struct ucall uc;

	TEST_REQUIRE(kvm_has_cap(KVM_CAP_SET_GUEST_DEBUG));

	vm = vm_create_with_one_vcpu(&vcpu, guest_code);
	vcpu_guest_debug_set(vcpu, &debug);
	vcpu_run(vcpu);

	TEST_ASSERT_KVM_EXIT_REASON(vcpu, KVM_EXIT_DEBUG);

	vcpu_get_reg(vcpu, RISCV_CORE_REG(regs.pc), &pc);
	TEST_ASSERT_EQ(pc, PC(sw_bp_1));

	/* skip sw_bp_1 */
	vcpu_set_reg(vcpu, RISCV_CORE_REG(regs.pc), pc + 4);

	/*
	 * Disable all debug controls.
	 * Guest should handle the ebreak without exiting to the VMM.
	 */
	memset(&debug, 0, sizeof(debug));
	vcpu_guest_debug_set(vcpu, &debug);

	vcpu_run(vcpu);
	TEST_ASSERT_EQ(get_ucall(vcpu, &uc), UCALL_SYNC);
	TEST_ASSERT_EQ(uc.args[1], RISCV_GUEST_EXC_BREAKPOINT);

	vcpu_get_reg(vcpu, RISCV_GENERAL_CSR_REG(sepc), &pc);
	TEST_ASSERT_EQ(pc, PC(sw_bp_2));

	kvm_vm_free(vm);

	return 0;
}
