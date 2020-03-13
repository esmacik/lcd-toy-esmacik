    .arch msp430g2553
    .p2align 1,0
    .text 
    
    .global song_state_machine
song_state_machine:
    add #1, r12
    cmp #30, r12
    JNZ else
    mov #0, r12
    else:
    ret
