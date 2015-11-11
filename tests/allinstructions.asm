#this file doesn't do anything useful
#it's just a test for all the available instructions
#in the mips subset we are testing
main:
addi    $t0, $zero, 1
IType: add     $t2, $t1, $t0
sub     $t3, $t1, $t0
and     $t4, $t2, $t3
or      $t5, $t2, $t3
nor     $t6, $t4, $t5
slt     $t7, $t4, $zero
sll     $t8, $t7, 2
srl     $t9, $t7, 2
j IType
jr      $ra
beq     $t7, $t6, IType
lw      $s0, 4($s2)
lw      $s1, -4($s3)
sw      $s0, -4($s3)
sw      $s1, 4($s2)
ori     $s4, $s1, 1
andi    $s5, $s0, 1
bne     $s4, $s5, Exit
jal main
j Exit
j Derp
Exit:
Derp:
