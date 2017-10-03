# ---------------- Beginning of Generated Code --------------------
        .file   "foo"
        .text
.globl graph1
        .type   graph1, @function
graph1:
.LFB0:
	.cfi_startproc
	pushq	%rbp              # save base pointer on stack
	.cfi_def_cfa_offset 16
	movq	%rsp, %rbp        # move stack pointer to base pointer
	.cfi_offset 6, -16
	.cfi_def_cfa_register 6
        subq	$1344, %rsp 	  # make space for this stack frame
	movq	%rbx, %r9        # save %rbx (callee-saved) in %r9
# ------------------------- begin Your code -----------------------------
	movl	$48,%eax         	#  48 -> %eax
	movl	%eax,%edi         	#  %eax -> %edi
	call	new              	#  new()
	movq	%rax,-1016(%rbp)     	#  %rax -> john
	movl	$48,%eax         	#  48 -> %eax
	movl	%eax,%edi         	#  %eax -> %edi
	call	new              	#  new()
	movq	%rax,-1008(%rbp)     	#  %rax -> mary
	movl	$48,%eax         	#  48 -> %eax
	movl	%eax,%edi         	#  %eax -> %edi
	call	new              	#  new()
	movq	%rax,-1000(%rbp)     	#  %rax -> fred
	movl	$2,%eax         	#  2 -> %eax
	movq	-1016(%rbp),%rcx     	#  john -> %rcx
	movl	%eax,32(%rcx)         	#  %eax -> [32+%rcx]
	movl	$19,%eax         	#  19 -> %eax
	movq	-1016(%rbp),%rcx     	#  john -> %rcx
	movl	%eax,0(%rcx)         	#  %eax -> [0+%rcx]
.L0:
	movq	-1008(%rbp),%rax     	#  mary -> %rax
	movq	-1016(%rbp),%rcx     	#  john -> %rcx
	movq	%rax,8(%rcx)         	#  %rax -> [8+%rcx]
	movsd	.LC3(%rip),%xmm0   	#  40000.000000 -> %xmm0
	movq	-1016(%rbp),%rax     	#  john -> %rax
	movsd	%xmm0,40(%rax)         	#  %xmm0 -> [40+%rax]
	movl	$3,%eax         	#  3 -> %eax
	movq	-1016(%rbp),%rcx     	#  john -> %rcx
	movl	%eax,16(%rcx)         	#  %eax -> [16+%rcx]
	movl	$21,%eax         	#  21 -> %eax
	movq	-1008(%rbp),%rcx     	#  mary -> %rcx
	movl	%eax,0(%rcx)         	#  %eax -> [0+%rcx]
	movq	-1000(%rbp),%rax     	#  fred -> %rax
	movq	-1008(%rbp),%rcx     	#  mary -> %rcx
	movq	%rax,8(%rcx)         	#  %rax -> [8+%rcx]
.L1:
	movl	$20,%eax         	#  20 -> %eax
	movq	-1000(%rbp),%rcx     	#  fred -> %rcx
	movl	%eax,0(%rcx)         	#  %eax -> [0+%rcx]
	movl	$0,%eax         	#  0 -> %eax
	movq	-1000(%rbp),%rcx     	#  fred -> %rcx
	movl	%eax,8(%rcx)         	#  %eax -> [8+%rcx]
	movsd	.LC4(%rip),%xmm0   	#  4.500000 -> %xmm0
	movq	-1016(%rbp),%rax     	#  john -> %rax
	movq	8(%rax),%rcx         	#  [8+%rax] -> %rcx
	movq	8(%rcx),%rax         	#  [8+%rcx] -> %rax
	movsd	%xmm0,24(%rax)         	#  %xmm0 -> [24+%rax]
	movq	-1016(%rbp),%rax     	#  john -> %rax
	movsd	40(%rax),%xmm0         	#  [40+%rax] -> %xmm0
	movl	$96,%eax         	#  96 -> %eax
	cltq	                  	#  sign-extend
	movsd	%xmm0,-1296(%rbp,%rax)	#  %xmm0 -> ac[%rax]
	movq	-1016(%rbp),%rax     	#  john -> %rax
	movq	%rax,-992(%rbp)     	#  %rax -> ptr
	movl	$0,%eax         	#  0 -> %eax
	movl	%eax,-1308(%rbp)     	#  %eax -> sum
	movl	$1,%eax         	#  1 -> %eax
	movl	%eax,-1312(%rbp)     	#  %eax -> i
.L2:
	movq	-992(%rbp),%rax     	#  ptr -> %rax
	movl	$0,%ecx         	#  0 -> %ecx
	cmpl	%ecx,%eax           	#  compare %eax - %ecx
	jne	.L5 			#  jump if     !=
	jmp	.L6 			#  jump 
.L5:
	movl	-1308(%rbp),%eax     	#  sum -> %eax
	movq	-992(%rbp),%rcx     	#  ptr -> %rcx
	movq	0(%rcx),%rdx         	#  [0+%rcx] -> %rdx
	addl	%edx,%eax         	#  %eax + %edx -> %eax
	movl	%eax,-1308(%rbp)     	#  %eax -> sum
	movq	-992(%rbp),%rax     	#  ptr -> %rax
	movsd	0(%rax),%xmm0         	#  [0+%rax] -> %xmm0
	movl	$-48,%eax         	#  -48 -> %eax
	movl	$48,%ecx         	#  48 -> %ecx
	movl	-1312(%rbp),%edx     	#  i -> %edx
	imull	%edx,%ecx         	#  %ecx * %edx -> %ecx
	addl	%ecx,%eax         	#  %eax + %ecx -> %eax
	cltq	                  	#  sign-extend
	movsd	%xmm0,-976(%rbp,%rax)	#  %xmm0 -> people[%rax]
	movq	-1016(%rbp),%rax     	#  john -> %rax
	movsd	32(%rax),%xmm0         	#  [32+%rax] -> %xmm0
	movl	$-8,%eax         	#  -8 -> %eax
	movl	$12,%ecx         	#  12 -> %ecx
	movl	-1312(%rbp),%edx     	#  i -> %edx
	imull	%edx,%ecx         	#  %ecx * %edx -> %ecx
	addl	%ecx,%eax         	#  %eax + %ecx -> %eax
	cltq	                  	#  sign-extend
	movsd	%xmm0,-1136(%rbp,%rax)	#  %xmm0 -> aco[%rax]
	movq	-992(%rbp),%rax     	#  ptr -> %rax
	movq	8(%rax),%rcx         	#  [8+%rax] -> %rcx
	movq	%rcx,-992(%rbp)     	#  %rcx -> ptr
	movl	-1312(%rbp),%eax     	#  i -> %eax
	movl	$1,%ecx         	#  1 -> %ecx
	addl	%ecx,%eax         	#  %eax + %ecx -> %eax
	movl	%eax,-1312(%rbp)     	#  %eax -> i
	jmp	.L2 			#  jump 
.L6:
	movl	$.LC7,%edi       	#  addr of literal .LC7
	call	write              	#  write()
	movl	-1312(%rbp),%eax     	#  i -> %eax
	movl	%eax,%edi         	#  %eax -> %edi
	call	writelni              	#  writelni()
	movl	$.LC8,%edi       	#  addr of literal .LC8
	call	write              	#  write()
	movl	-1308(%rbp),%eax     	#  sum -> %eax
	movl	%eax,%edi         	#  %eax -> %edi
	call	writelni              	#  writelni()
	movl	$.LC9,%edi       	#  addr of literal .LC9
	call	write              	#  write()
	movq	-1000(%rbp),%rax     	#  fred -> %rax
	movsd	24(%rax),%xmm0         	#  [24+%rax] -> %xmm0
	call	writelnf              	#  writelnf()
	movl	-1308(%rbp),%eax     	#  sum -> %eax
	movl	$3,%ecx         	#  3 -> %ecx
	cmpl	%ecx,%eax           	#  compare %eax - %ecx
	jl	.L10 			#  jump if     <
	jmp	.L11 			#  jump 
.L10:
	jmp	.L1 			#  jump 
.L11:
# ----------------------- begin Epilogue code ---------------------------
	movq	%r9, %rbx        # restore %rbx (callee-saved) from %r9
        leave
        ret
        .cfi_endproc
.LFE0:
        .size   graph1, .-graph1
# ----------------- end Epilogue; Literal data follows ------------------
        .section        .rodata
	.align  4
.LC7:
	.string	"i = "
	.align  4
.LC8:
	.string	"Sum of ages = "
	.align  4
.LC9:
	.string	"Fred loc im = "
	.align  8
.LC3:
	.long	0   	#  40000.000000
	.long	1088653312
	.align  8
.LC4:
	.long	0   	#  4.500000
	.long	1074921472

        .ident  "CS 375 Compiler - Spring 2016"
