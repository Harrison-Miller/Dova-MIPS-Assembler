addi $t0, $zero, 1
j next
next:
j skip1
add $t0, $t0, $t0
skip1:
j skip2
add $t0, $t0, $t0
add $t0, $t0, $t0
skip2:
j skip3
loop:
add $t0, $t0, $t0
add $t0, $t0, $t0
add $t0, $t0, $t0
skip3:
j loop
