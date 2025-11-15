# Practice Emulator

## Motivation

This project represents a desire to exercise low level skills.
I hadn't done a large project using C++, but knew that would be something that would be beneficial.
I also utilized python in the 'assembler' for this made up architecture since the string processing would already be difficult enough, and wasn't the point of the project.

## High-Level Architecture

This project utilizes a RISC-V-style instruction set since the point was to practice and learn how architecture works at a high-level, rather than make an actual emulator.
Though, I do see the irony of running a RISC-V emulation over my x86 processor in my computer.
The instruction size is 64 bits, partly to replicate what is standard, and partly because I rely on 32-bit Immediate values in the assembly I will end up writing.
More on the example programs later.

I am also going to be using 22 registers for this processor, as well as 32 bit, byte-addressable, little-endian memory.
The registers will be in 16 general-use registers, and 6 specialized registers.
For more details, check include/emu.h.

Looking through both the assembler/asm.py and include/emu.h files, you will be able to see the instructions I included in this project.
You can see some distinction between a "byte" and "integer".
This language treats integer values as 4 bytes.
They are as follows, starting from opcode 1 through 40:

| Instruction | Description |
|-------------|-------------|
|     JMP     | Jump to a supplied memory address |
|     JMR     | Jump to address located in register |
|     BNZ     | Branch with a non-zero condition |
|     BGT     | Branch with a greater-than condition |
|     BLT     | Branch with a less-than condition |
|     BRZ     | Branch, checking if the given register is zero |
|     MOV     | Move contents of register to another register |
|     MOVI    | Move immediate value to a register |
|     LDA     | Load immediate value (address) into given register |
|     STR     | Store value of register in given address |
|     LDR     | Load contents of given address as an integer |
|     STB     | Store value of register as a byte |
|     LDB     | Load contents of a given address (and the 3 following) as in integer |
|     ISTR    | Store an integer in a register at an address pointed at by another register |
|     ILDR    | Load an integer to a register from an address pointed at by another register |
|     ISTB    | Store a byte in a register to an address pointed at by another register |
|     ILDB    | Load a byte to a register from an address pointed at by another register |
|     ADD     | Add the numbers in 2 registers |
|     ADDI    | Add the number in a register to the immediate value |
|     SUB     | Subtract the numbers in 2 registers |
|     SUBI    | Subtract the number in a register with the immediate value |
|     MUL     | Multiply the numbers in 2 registers |
|     MULI    | Multiply the number in a register with the immediate value |
|     DIV     | Divide the numbers in 2 registers |
|     SDIV    | Do signed division on the numbers in 2 registers |
|     DIVI    | Divide the number in a register with the immediate value |
|     AND     | Logical AND between 2 register values |
|     OR      | Logical OR between 2 register values |
|     CMP     | Do a comparison between the values in 2 registers to allow for branching checks (lt, gt, z, nz) |
|     CMP     | Do a comparison between the values in a register and the immediate value to allow for branching checks (lt, gt, z, nz) |
|     TRP     | Trap codes for various operations. Configured operations are: Halt, Int In/Out, Char In/Out, String In/Out, and Register Dump |
|     ALCI    | Allocate heap memory in the amount specified in the immediate value |
|     ALLC    | Allocate heap memory in the amount specified in the address given by the immediate value |
|     IALLC   | Allocate heap memory in the amount required to satisfy a value in a given register |
|     PSHR    | Push to the stack a 4-byte value in a register |
|     PSHB    | Push to the stack a byte value in a register |
|     POPR    | Pop from the stack a 4-byte value to a register |
|     POPB    | Pop from the stack a byte value to a register |
|     CALL    | Push PC onto stack and jump to PC in immediate |
|     RET     | Pop stack and put value into PC |


## Features

This emulator has the following features:
 - An L1 cache, with options to run as associated/2-way associated/direct mapped
 - Memory management in stack and heap operations
 - Variable memory assignment at runtime
 - Function operations
 - Branching and looping
 - Strings utilizing a combined Pascal/C syntax

## Assembly syntax

The following is an example of the syntax accepted by the assembler:

```asm
; This is the data section, specifying labels and other values such as arrays
label .byt 4      ;this is a comment
char  .byt 'A'   ; Here is a char
array .int 6     ; Start of an array
      .int 9
      .int 4
      .int 2
      .int 0   ;nice
welcome .str "Welcome to my program! Thank goodness I don't have to make a char array manually"

;Code section
jmp MAIN ; this is how the program knows where the code section begins. This address is placed as the first instruction so the emulator can jump the data section
MAIN lda r3, welcome ; instructions are not case sensitive, but labels are
     trp #5 ; traps need the '#'
    ; also that prints out the string labeled 'welcome'
movi r6, array
 movi, r5, #4
call prnt_loop
trp, #0 ; halt

prnt_loop ildr, r3, r6 ; get value from the address in r6 and put in r3
trp, #1 ; print what value is in r3 as an int
  subi r5, r5, #1 ; decrement loop counter
  addi r6, r6, #1 ; increment the address
  bnz r5, prnt_loop ; check if r5 is zero, otherwise loop

    ret
```

## Running

Clone and enter the repo:

```bash
git clone git@github.com:Kahnshaak/practice_emulator
cd practice_emulator
```

I have this set up to use nix so you don't need to have the right util versions. You can start the shell with:

```bash
nix develop
```

If do not using nix, then this will require having around g++ 11.4, Python 3.10, CMake 3.22.1, and Make 4.3. This also assumes use on linux. This probably won't work on other systems.

Build the project

```bash
mkdir build
cd build/
cmake ..
make
```

You can run one of the example programs in the programs folder. From the build folder:

```bash
python3 ../assembler/asm.py ../programs/Primes.asm
```

Then put the machine code into the emulator:

```bash
./emu ../programs/Primes.bin
```

You can also specify some options:

```bash
./emu ../programs/Primes.bin -m 200000 -c 1
```
This requires a 200,000 byte memory size for the emulator (option -m) and forces a direct mapped cache (option c).
The cache can be any of the following:
 - 0: No cache
 - 1: Direct Mapped Cache
 - 2: Associative Cache
 - 3: 2-way Associative Cache

 The cache also does reporting on many operations are done which can be logged away for experimenting.
