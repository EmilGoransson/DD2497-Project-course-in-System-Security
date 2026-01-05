.section .rodata

.section .text
.global canary_trap

#   If we use the interrupt attribute,
#   all registers will be saved to the stack, BUT,
#   in an unpredictable order. Instead, be save them
#   in natural order x1-x31 so that we can read them 
#   later.
#
#   The compiler reserves the first 256 bytes for some
#   reason, I will do the same. It might be some sort
#   of requirement, but I am not sure.

canary_trap:
    # Reserver the first 256 unused bytes and 32 registers
    # x0-x4 does not have to be saved, but to make thing simpler,
    # we allocate space for all 32 registers. Also SP has to be 
    # 16 byte alligned, (at least in X86).
    addi sp, sp, -512
    sd ra,  0(sp)
    sd x5,  48(sp)
    sd x6,  56(sp)
    sd x7,  64(sp)
    sd x8,  72(sp)
    sd x9,  80(sp)
    sd x10, 88(sp)
    sd x11, 96(sp)
    sd x12, 104(sp)
    sd x13, 112(sp)
    sd x14, 120(sp)
    sd x15, 128(sp)
    sd x16, 136(sp)
    sd x17, 144(sp)
    sd x18, 152(sp)
    sd x19, 160(sp)
    sd x20, 168(sp)
    sd x21, 176(sp)
    sd x22, 184(sp)
    sd x23, 192(sp)
    sd x24, 200(sp)
    sd x25, 208(sp)
    sd x26, 216(sp)
    sd x27, 224(sp)
    sd x28, 232(sp)
    sd x29, 240(sp)
    sd x30, 248(sp)
    sd x31, 256(sp)

    call canary_trap_handler

    ld ra,  0(sp)
    ld x5,  48(sp)
    ld x6,  56(sp)
    ld x7,  64(sp)
    ld x8,  72(sp)
    ld x9,  80(sp)
    ld x10, 88(sp)
    ld x11, 96(sp)
    ld x12, 104(sp)
    ld x13, 112(sp)
    ld x14, 120(sp)
    ld x15, 128(sp)
    ld x16, 136(sp)
    ld x17, 144(sp)
    ld x18, 152(sp)
    ld x19, 160(sp)
    ld x20, 168(sp)
    ld x21, 176(sp)
    ld x22, 184(sp)
    ld x23, 192(sp)
    ld x24, 200(sp)
    ld x25, 208(sp)
    ld x26, 216(sp)
    ld x27, 224(sp)
    ld x28, 232(sp)
    ld x29, 240(sp)
    ld x30, 248(sp)
    ld x31, 256(sp)
    addi sp, sp, 512
    # Mret is used by s3k kernel as explained in comments
    # Adding this is the same as setting the function as
    # attribute("interupt")
    mret 