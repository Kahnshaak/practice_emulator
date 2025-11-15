; Prime Number Generator

; Data Section
welcome .str "Welcome to the Prime Number Generator.\nThis program searches for and displays the first 20 prime numbers greater than\nor equal to a user provided lower bound.\n"
prompt1 .str "Please enter a lower bound: "
prompt2 .str "The first 20 prime numbers greater than or equal to "
prompt3 .str " are:"
newline .str "\n"

; Code Section
        jmp MAIN

; MOD function: r4=dividend, r5=divisor, returns remainder in r6
MOD     div r8, r4, r5      ;quotient in r8
        mul r6, r8, r5      ;r6 = quotient * divisor
        sub r6, r4, r6      ;r6 = dividend - (quotient * divisor)
        ret

; IS_PRIME function: input r4=number to test, output r6=1 if prime, 0 if not
IS_PRIME cmpi r9, r4, #2    ;if n < 2, not prime
        blt r9, NOT_PRIME
        cmpi r9, r4, #2     ;if n == 2, prime
        brz r9, IS_PRIME_TRUE
        cmpi r9, r4, #3     ;if n == 3, prime
        brz r9, IS_PRIME_TRUE

        ; Check if even (except 2)
        movi r5, #2
        call MOD
        brz r6, NOT_PRIME   ;if even, not prime

        ; Check odd divisors from 3 to sqrt(n)
        movi r5, #3         ;start checking from 3

CHECK_LOOP  mul r8, r5, r5      ;r8 = r5^2
        cmp r9, r8, r4      ;compare r5^2 with n
        bgt r9, IS_PRIME_TRUE ;if r5^2 > n, n is prime

        call MOD            ;check if r4 % r5 == 0
        brz r6, NOT_PRIME   ;if remainder is 0, not prime

        addi r5, r5, #2     ;check next odd number
        jmp CHECK_LOOP

IS_PRIME_TRUE   movi r6, #1
        ret

NOT_PRIME   movi r6, #0
        ret

MAIN    lda r3, welcome
        trp #5              ;print welcome
        lda r3, prompt1
        trp #5              ;print prompt1
        trp #2              ;read lower bound
        mov r1, r3          ;store lower bound

        alci r11, #80       ;allocate space for 20 integers (20*4 bytes)
        mov r12, r11        ;r12 = current position in array
        movi r2, #0         ;count of primes found
        mov r4, r1          ;current number to test

FIND_PRIMES cmpi r9, r2, #20    ;found 20 primes?
        brz r9, PRINT_RESULTS

        call IS_PRIME       ;test current number
        brz r6, NEXT_NUM    ;if not prime, try next number

        ; Found a prime - store it
        istr r4, r12        ;store prime in array
        addi r12, r12, #4   ;move to next array position
        addi r2, r2, #1     ;increment prime count

NEXT_NUM    addi r4, r4, #1     ;try next number
        jmp FIND_PRIMES

PRINT_RESULTS   lda r3, prompt2
        trp #5              ;print "The first 20 prime numbers greater than or equal to "
        mov r3, r1
        trp #1              ;print lower bound
        lda r3, prompt3
        trp #5              ;print " are:"

        mov r12, r11        ;reset to start of array
        movi r2, #20        ;counter for printing

PRINT_LOOP  ildr r3, r12        ;load prime from array
        trp #1              ;print prime
        lda r3, newline
        trp #5              ;print newline
        addi r12, r12, #4   ;move to next prime
        subi r2, r2, #1     ;decrement counter
        bnz r2, PRINT_LOOP  ;continue if more to print

        trp #0
