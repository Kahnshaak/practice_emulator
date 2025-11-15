; Recursive Fibonacci

; Data Section
prompt1 .str "Please enter the Fibonacci term you would like computed: "
prompt2 .str "Term "
prompt3 .str " in the Fibonacci sequence is: "
newline .str "\n"

; Code Section
        jmp MAIN

; FIB function: input in r2, output in r2
FIB     cmpi r0, r2, #1     ;N == 1?
        brz r0, BASE_1
        cmpi r0, r2, #2     ;N == 2?
        brz r0, BASE_2

        ; Save current state
        pshr r2             ;save N

        ; Calculate fib(N-1)
        subi r2, r2, #1     ;N-1
        call FIB            ;result in r2
        mov r4, r2          ;save fib(N-1)

        ; Restore N and calculate fib(N-2)
        popr r2             ;restore r4
        pshr r4             ;save fib(N-1) result
        subi r2, r2, #2     ;N-2
        call FIB            ;result in r2 = fib(N-2)

        ; Add results: fib(N-1) + fib(N-2)
        popr r4             ;restore fib(N-1)
        add r2, r4, r2      ;r2 = fib(N-1) + fib(N-2)
        ret

BASE_1  movi r2, #1         ;fib(1) = 1
        ret
BASE_2  movi r2, #1         ;fib(2) = 1
        ret

MAIN    lda r3, prompt1
        trp #5            ;print prompt1
        trp #2            ;read input
        mov r1, r3        ;store input term
        mov r2, r3        ;copy to r2 for function call

        call FIB          ;result returned in r2

PRINT   lda r3, prompt2
        trp #5            ;print "Term "
        mov r3, r1        ;print input term
        trp #1
        lda r3, prompt3
        trp #5            ;print " in the Fibonacci sequence is: "
        mov r3, r2        ;print result
        trp #1
        lda r3, newline
        trp #5            ;print newline
        trp #0
