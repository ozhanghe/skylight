global syscall_dispatch

%define nsyscalls 0

syscall_dispatch:
    push rcx
    mov rcx, 0xC0000102
    rdmsr
    pop rcx
    shr rdx, 0x20
    or rax, rdx
    mov r10, rsp
    mov rsp, rax
    mov rbp, rsp
    push rbp
    push r10

    cmp rax, nsyscalls
    jge .generate_ud

    lea r10, [rel syscall_table]
    lea r10, [r10 + (rax * 8)]
    mov rax, [r10]

    xor r10d, r10d

    cld
    call rax

    .sysret:
    pop rsp
    pop rbp
    o64 sysret

    .generate_ud:
    xor eax, eax
    mov [rcx], rax
    jmp .sysret

syscall_table:
    dq 0x0000000000000000