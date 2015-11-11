L1: add  $t1, $t1, $t2  # (0) R[1] = R[1] + R[2]
    addi $t1, $t1, 1    # (1) R[1]++
    addi $t2, $t2, 1    # (2) R[2]++
    beq  $t1, $t2, L1   # (3) If R[1] == R[2] goto L1
