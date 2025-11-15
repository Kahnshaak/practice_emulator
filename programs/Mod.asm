; Modulus Program

; Data Section
prompt1 .str "Please enter an integer dividend: "
prompt2 .str "Please enter an integer divisor: "
prompt3 .str " divided by "
prompt4 .str " results in a remainder of "
newline .str "\n"

; Code Section
        jmp MAIN

; MOD function: r1=dividend, r2=divisor, returns remainder in r4
MOD     div r5, r1, r2      ;r5 = r1 / r2 (quotient)
        mul r4, r5, r2      ;r4 = quotient * divisor
        sub r4, r1, r4      ;r4 = dividend - (quotient * divisor)
        ret

MAIN    lda r3, prompt1
        trp #5            ;print prompt1
        trp #2            ;read dividend
        mov r1, r3        ;store dividend
        lda r3, prompt2
        trp #5            ;print prompt2
        trp #2            ;read divisor
        mov r2, r3        ;store divisor

        call MOD

PRINT   mov r3, r1
        trp #1            ;print dividend
        lda r3, prompt3
        trp #5            ;print " divided by "
        mov r3, r2
        trp #1            ;print divisor
        lda r3, prompt4
        trp #5            ;print " results in a remainder of "
        mov r3, r4
        trp #1            ;print remainder
        lda r3, newline
        trp #5
        trp #0
