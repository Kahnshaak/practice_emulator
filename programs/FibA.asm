;This is program A, which calculates the Fibonacci sequence.

; Data Section
prompt1 .str "Please enter the Fibonacci term you would like computed: "
prompt2 .str "Term "
prompt3 .str " in the Fibonacci sequence is: "
newline .str "\n"

; Code Section
        jmp MAIN
MAIN    lda r3, prompt1
        trp #5              ;print prompt1
        trp #2
        mov r1, r3          ;store input term
        cmpi r0, r3, #1     ;term == 1
        brz r0, TERM1       ;if term is 1
        cmpi r0, r3, #2     ;term == 2
        brz r0, TERM2       ;if term is 2
        jmp COMPUTE

TERM1   movi r4, #0         ;fib(1) = 0
        jmp PRINT
TERM2   movi r4, #1         ;fib(2) = 1
        jmp PRINT

;r0 = cmp flag, r1 = input term, r2 = n2, r3 = n1, r4 = result, r5 = current term
COMPUTE movi r3, #1         ;n1 = 0 (fib(1))
        movi r2, #1         ;n2 = 1 (fib(2))
        movi r5, #3         ;start from term 3

LOOP    cmp r0, r5, r1      ;compare current term with target
        brz r0, DONE        ;if equal, we're done
        mov r4, r2          ;temp = n2
        add r2, r3, r2      ;n2 = n1 + n2
        mov r3, r4          ;n1 = temp
        addi r5, r5, #1     ;increment term counter
        jmp LOOP

DONE    mov r4, r2          ;result = n2

PRINT   lda r3, prompt2
        trp #5              ;print "Term "
        mov r3, r1          ;print INPUT term (not current counter)
        trp #1
        lda r3, prompt3
        trp #5              ;print " in the Fibonacci sequence is: "
        mov r3, r4
        trp #1              ;print result
        lda r3, newline
        trp #5              ;print newline
        trp #0              ;end program