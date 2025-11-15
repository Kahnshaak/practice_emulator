#include <gtest/gtest.h>
#include "../include/emu4380.h"

//////// test init_mem ////////
TEST(initMem, TestCorrectInputs)
{
    // test init_mem does not have its own error handling, that is handled by main
    EXPECT_NO_THROW(init_mem(131072)); // check default
    ASSERT_EQ(prog_mem_size, 131072);
    EXPECT_NO_THROW(init_mem(4294967295)); // check max
    ASSERT_EQ(prog_mem_size, 4294967295);
    EXPECT_NO_THROW(init_mem(0)); // check min
    ASSERT_EQ(prog_mem_size, 0);
}

//////// test naiveLegal ////////
TEST(naiveLegal, TestCorrectOutputs)
{
    static int opcodes[18] = {1, 7, 8, 9, 10, 11, 12, 13, 18, 19, 20, 21, 22, 23, 24, 25, 26, 31};
    // lower bound operands
    cntrl_regs[OPERAND_1] = 0;
    cntrl_regs[OPERAND_2] = 0;
    cntrl_regs[OPERAND_3] = 0;
    cntrl_regs[IMMEDIATE] = 0;
    for (int i = 0; i < 18; i++)
    {
        cntrl_regs[OPERATION] = opcodes[i];
        ASSERT_EQ(naiveLegal(), true);
    }
    // upper bound operands
    cntrl_regs[OPERATION] = 1;
    cntrl_regs[OPERAND_1] = 255;
    cntrl_regs[OPERAND_2] = 255;
    cntrl_regs[OPERAND_3] = 255;
    cntrl_regs[IMMEDIATE] = 4294967295;
    EXPECT_EQ(naiveLegal(), true);
}

TEST(naiveLegal, TestIncorrectOutputs)
{
    // out of bound operands
    cntrl_regs[OPERATION] = 7;
    cntrl_regs[OPERAND_1] = 256;
    cntrl_regs[OPERAND_2] = 256;
    cntrl_regs[OPERAND_3] = 256;
    cntrl_regs[IMMEDIATE] = -6;
    EXPECT_EQ(naiveLegal(), false);

    // incorrect opcodes
    static int badOpcodes[13] = {2, 3, 4, 5, 6, 14, 15, 16, 17, 27, 28, 29, 30};
    cntrl_regs[OPERAND_1] = 0;
    cntrl_regs[OPERAND_2] = 0;
    cntrl_regs[OPERAND_3] = 0;
    cntrl_regs[IMMEDIATE] = 0;
    for (int i = 0; i < 13; i++)
    {
        cntrl_regs[OPERATION] = badOpcodes[i];
        EXPECT_EQ(naiveLegal(), false);
    }
}


//////// test setCntrlRegs ////////
TEST(setControlRegisters, TestCorrectInputs)
{
    // set PC
    reg_file[PC] = 0;


    // init prog_mem
    init_mem(8);

    //////////////////// test 1 of 2 ////////////////////
    // set some memory addresses
    prog_mem[0] = 18;
    prog_mem[1] = 1;
    prog_mem[2] = 7;
    prog_mem[3] = 7;
    prog_mem[4] = 0;
    prog_mem[5] = 0;
    prog_mem[6] = 0;
    prog_mem[7] = 0;

    // run test1 0 for immediate
    setCntrlRegs();

    std::cout << "OPERATION: " << cntrl_regs[OPERATION] << " == 18" << std::endl;
    std::cout << "OPERAND_1: " << cntrl_regs[OPERAND_1] << " == 1" << std::endl;
    std::cout << "OPERAND_2: " << cntrl_regs[OPERAND_2] << " == 7" << std::endl;
    std::cout << "OPERAND_3: " << cntrl_regs[OPERAND_3] << " == 7" << std::endl;
    std::cout << "IMMEDIATE: " << cntrl_regs[IMMEDIATE] << " == 0" << std::endl;

    ASSERT_EQ(cntrl_regs[OPERATION], 18);
    ASSERT_EQ(cntrl_regs[OPERAND_1], 1);
    ASSERT_EQ(cntrl_regs[OPERAND_2], 7);
    ASSERT_EQ(cntrl_regs[OPERAND_3], 7);
    ASSERT_EQ(cntrl_regs[IMMEDIATE], 0);

    //////////////////// test 2 of 2 ////////////////////
    // set some memory addresses
    prog_mem[4] = 2;
    prog_mem[5] = 2;
    prog_mem[6] = 0;
    prog_mem[7] = 0;

    // run tests
    setCntrlRegs();
    std::cout << "OPERATION: " << cntrl_regs[OPERATION] << " == 18" << std::endl;
    std::cout << "OPERAND_1: " << cntrl_regs[OPERAND_1] << " == 1" << std::endl;
    std::cout << "OPERAND_2: " << cntrl_regs[OPERAND_2] << " == 7" << std::endl;
    std::cout << "OPERAND_3: " << cntrl_regs[OPERAND_3] << " == 7" << std::endl;
    std::cout << "IMMEDIATE: " << cntrl_regs[IMMEDIATE] << " == 514" << std::endl;
    ASSERT_EQ(cntrl_regs[OPERATION], 18);
    ASSERT_EQ(cntrl_regs[OPERAND_1], 1);
    ASSERT_EQ(cntrl_regs[OPERAND_2], 7);
    ASSERT_EQ(cntrl_regs[OPERAND_3], 7);
    ASSERT_EQ(cntrl_regs[IMMEDIATE], 514);
}

// test checkMemoryLocationExists
TEST(checkMemLocExists, TestCorrectInputs)
{
    // init prog_mem
    init_mem(64);
    ASSERT_EQ(checkMemoryLocationExists(0), true); // check min
    ASSERT_EQ(checkMemoryLocationExists(32), true); // check mid
    ASSERT_EQ(checkMemoryLocationExists(63), true); // check max

    ASSERT_EQ(checkMemoryLocationExists(128), false); // check too big
    ASSERT_EQ(checkMemoryLocationExists(64), false); // check slightly too big
}

// test checkRegAddressExists
TEST(checkRegLocExists, TestCorrectInputs)
{
    cntrl_regs[OPERATION] = 0;
    cntrl_regs[OPERAND_1] = 1;
    cntrl_regs[OPERAND_2] = 2;
    cntrl_regs[OPERAND_3] = 3;
    cntrl_regs[IMMEDIATE] = 5;
    ASSERT_EQ(checkRegAddressExists(OPERATION), true);
    ASSERT_EQ(checkRegAddressExists(OPERAND_1), true);
    ASSERT_EQ(checkRegAddressExists(OPERAND_2), true);
    ASSERT_EQ(checkRegAddressExists(OPERAND_3), true);
    ASSERT_EQ(checkRegAddressExists(IMMEDIATE), true);
}

TEST(checkRegLocExists, TestIncorectInputs)
{
    cntrl_regs[OPERATION] = 22;
    cntrl_regs[OPERAND_1] = 23;
    cntrl_regs[OPERAND_2] = 24;
    cntrl_regs[OPERAND_3] = 25;
    cntrl_regs[IMMEDIATE] = 26;
    ASSERT_EQ(checkRegAddressExists(OPERATION), false);
    ASSERT_EQ(checkRegAddressExists(OPERAND_1), false);
    ASSERT_EQ(checkRegAddressExists(OPERAND_2), false);
    ASSERT_EQ(checkRegAddressExists(OPERAND_3), false);
    ASSERT_EQ(checkRegAddressExists(IMMEDIATE), false);
}

// test checkOperandOneExists
TEST(checkOPERANDOneExists, TestCorrectInputs)
{
    for (int i = 0; i < 22; i++)
    {
        cntrl_regs[OPERAND_1] = i;
        ASSERT_EQ(checkOPERANDOneExists(), true);
    }
}

TEST(checkOperandOneExists, TestIncorrectInputs)
{
    cntrl_regs[OPERAND_1] = 22;
    ASSERT_EQ(checkOperandOneExists(), false);

    cntrl_regs[OPERAND_1] = 23;
    ASSERT_EQ(checkOperandOneExists(), false);
}

// test checkJmp

TEST(checkJmp, TestCorrectInputs)
{
    init_mem(64);
    cntrl_regs[IMMEDIATE] = 63; // max
    ASSERT_EQ(checkJmp(), true);

    cntrl_regs[IMMEDIATE] = 0; // min
    ASSERT_EQ(checkJmp(), true);

    cntrl_regs[IMMEDIATE] = 32; // mid
    ASSERT_EQ(checkJmp(), true);
}

TEST(checkJmp, TestIncorrectInputs)
{
    init_mem(64);
    cntrl_regs[IMMEDIATE] = 64;
    ASSERT_EQ(checkJmp(), false);

    cntrl_regs[IMMEDIATE] = 128;
    ASSERT_EQ(checkJmp(), false);
}

// test checkMov
TEST(checkMov, TestCorrectInputs)
{
    // check min
    cntrl_regs[OPERAND_1] = 0;
    cntrl_regs[OPERAND_2] = 0;
    ASSERT_EQ(checkMov(), true);

    // check mid
    cntrl_regs[OPERAND_1] = 11;
    cntrl_regs[OPERAND_2] = 11;
    ASSERT_EQ(checkMov(), true);

    // check max
    cntrl_regs[OPERAND_1] = 21;
    cntrl_regs[OPERAND_2] = 21;
    ASSERT_EQ(checkMov(), true);
}

TEST(checkMov, TestIncorrectInputs)
{
    // set operand1 too high
    cntrl_regs[OPERAND_1] = 22;
    cntrl_regs[OPERAND_2] = 0;
    ASSERT_EQ(checkMov(), false);

    // set operand2 too high
    cntrl_regs[OPERAND_1] = 0;
    cntrl_regs[OPERAND_2] = 22;
    ASSERT_EQ(checkMov(), false);

    // set both too high
    cntrl_regs[OPERAND_1] = 55;
    cntrl_regs[OPERAND_2] = 55;
    ASSERT_EQ(checkMov(), false);

    // set both absurdly too high
    cntrl_regs[OPERAND_1] = 5000000;
    cntrl_regs[OPERAND_2] = 5000000;
    ASSERT_EQ(checkMov(), false);
}

// test checkStr and also test checkLdr
TEST(checkStr, testCorrectInputs)
{
    init_mem(64);
    // check min
    cntrl_regs[OPERAND_1] = 0;
    cntrl_regs[IMMEDIATE] = 0;
    ASSERT_EQ(checkStr(), true);

    // check mid
    cntrl_regs[OPERAND_1] = 11;
    cntrl_regs[IMMEDIATE] = 32;
    ASSERT_EQ(checkStr(), true);

    // check max
    cntrl_regs[OPERAND_1] = 21;
    cntrl_regs[IMMEDIATE] = 64;
    ASSERT_EQ(checkStr(), true);
}

TEST(checkStr, TestIncorrectInputs)
{
    init_mem(64);
    // check op1 too big
    cntrl_regs[OPERAND_1] = 22;
    cntrl_regs[IMMEDIATE] = 0;
    ASSERT_EQ(checkStr(), false);

    // check imm too big
    cntrl_regs[OPERAND_1] = 0;
    cntrl_regs[IMMEDIATE] = 65;
    ASSERT_EQ(checkStr(), false);
}

// test checkTrp

TEST(checkTrp, TestCorrectInputs)
{
    const unsigned int legal_trp_vals[6] = {0, 1, 2, 3, 4, 98};
    for (unsigned int legal_trp_val : legal_trp_vals)
    {
        cntrl_regs[IMMEDIATE] = legal_trp_val;
        ASSERT_EQ(checkTrp(), true);
    }

}

TEST(checkTrp, TestIncorrectInputs)
{
    const unsigned int illegal_trp_vals[6] = {5, 6, 7, 8, 9, 99};
    for (unsigned int illegal_trp_val : illegal_trp_vals)
    {
        cntrl_regs[IMMEDIATE] = illegal_trp_val;
        ASSERT_EQ(checkTrp(), false);
    }
}

/*
 * The following tests are on the functions for each operation
 * When writing the program initially, it was decided to use functionalize
 * as many aspects of the codebase in order to facilitate better modularity
 * since the theory was this could be added to in the future with new commands.
 *
 * As a part of that, each test only tests correct inputs, as in theory, error
 * handling is done before the function is ever called in decode, the only error handling
 * is in div function and its derivatives.
 */

// test jmp
TEST(jmp, testCorrectInputs)
{
    init_mem(64);
    reg_file[PC] = 0;
    ASSERT_EQ(reg_file[PC], 0);
    cntrl_regs[IMMEDIATE] = 62;
    jmp();
    ASSERT_EQ(reg_file[PC], 62);
}

// test mov
TEST(mov, testCorrectInputs)
{
    reg_file[R10] = 664;
    cntrl_regs[OPERAND_1] = 12; // register R12
    cntrl_regs[OPERAND_2] = 10; // register R10
    mov();
    ASSERT_EQ(reg_file[R12], 664);
    ASSERT_EQ(reg_file[R10], 0);
}

// test movi
TEST(movi, testCorrectInputs)
{
    reg_file[R10] = 664;
    cntrl_regs[OPERAND_1] = 12; // register R12
    cntrl_regs[OPERAND_2] = 10; // register R10
    cntrl_regs[IMMEDIATE] = 120002;
    movi();
    ASSERT_EQ(reg_file[R12], 120002);
    ASSERT_EQ(reg_file[R10], 664);
}

// test lda
TEST(lda, testCorrectInputs)
{
    init_mem(64);
    constexpr unsigned char num = 15;
    prog_mem[12] = num;
    cntrl_regs[OPERAND_1] = 12;
    cntrl_regs[IMMEDIATE] = 12;
    lda();
    ASSERT_EQ(reg_file[R12], 12);
    ASSERT_EQ(prog_mem[12], 15);
}

// test str
TEST(str, testCorrectInputs)
{
    init_mem(64);
    prog_mem[12] = 15;
    reg_file[R12] = 664;
    cntrl_regs[OPERAND_1] = 12;
    cntrl_regs[IMMEDIATE] = 12;
    str();
    ASSERT_EQ(prog_mem[12], 15);

}

// test ldr
TEST(ldr, testCorrectINputs)
{
    init_mem(64);
    prog_mem[10] = 78;
    cntrl_regs[OPERAND_1] = 10;
    cntrl_regs[IMMEDIATE] = 10;
    ldr();
    ASSERT_EQ(reg_file[R10], 78);
}

// test stb
TEST(stb, testCorrectInputs)
{
    init_mem(64);
    reg_file[R10] = 664;
    cntrl_regs[OPERAND_1] = R10;
    cntrl_regs[IMMEDIATE] = 24;
    const int lsb = 152;
    stb();
    ASSERT_EQ(prog_mem[24], lsb);
}

// test ldb
TEST(ldb, testCorrectInputs)
{
    init_mem(64);
    cntrl_regs[OPERAND_1] = R10;
    cntrl_regs[IMMEDIATE] = 24;
    prog_mem[24] = 15;
    ldb();
    ASSERT_EQ(reg_file[R10], 15);
}

// test add
TEST(add, testCorrectInputs)
{
    init_mem(64);
    reg_file[R10] = 664;
    reg_file[R12] = 664;
    cntrl_regs[OPERAND_1] = R0;
    cntrl_regs[OPERAND_2] = R10;
    cntrl_regs[OPERAND_3] = R12;
    add();

    ASSERT_EQ(reg_file[R10], 664);
    ASSERT_EQ(reg_file[R12], 664);
    ASSERT_EQ(reg_file[R0], 1328);

}

// test addi
TEST(addi, testCorrectInputs)
{
    init_mem(64);
    reg_file[R10] = 15;
    cntrl_regs[OPERAND_1] = R0;
    cntrl_regs[OPERAND_2] = R10;
    cntrl_regs[IMMEDIATE] = 15;
    addi();

    ASSERT_EQ(reg_file[R10], 15);
    ASSERT_EQ(reg_file[R0], 30);
}

// test sub
TEST(sub, testCorrectInputs)
{
    reg_file[R10] = 5;
    reg_file[R12] = 5;
    cntrl_regs[OPERAND_1] = R0;
    cntrl_regs[OPERAND_2] = R10;
    cntrl_regs[OPERAND_3] = R12;
    sub();

    ASSERT_EQ(reg_file[R10], 5);
    ASSERT_EQ(reg_file[R12], 5);
    ASSERT_EQ(reg_file[R0], 0);

    reg_file[R10] = 10;
    reg_file[R12] = 5;
    sub();
    ASSERT_EQ(reg_file[R0], 5);
}

// test subi
TEST(subi, testCorrectInputs)
{
    reg_file[R10] = 30;
    cntrl_regs[OPERAND_1] = R0;
    cntrl_regs[OPERAND_2] = R10;
    cntrl_regs[IMMEDIATE] = 15;
    subi();

    ASSERT_EQ(reg_file[R10], 30);
    ASSERT_EQ(reg_file[R0], 15);
}

// test mul
TEST(mul, testCorrectInputs)
{
    reg_file[R10] = 5;
    reg_file[R12] = 5;
    cntrl_regs[OPERAND_1] = R0;
    cntrl_regs[OPERAND_2] = R10;
    cntrl_regs[OPERAND_3] = R12;
    mul();

    ASSERT_EQ(reg_file[R10], 5);
    ASSERT_EQ(reg_file[R12], 5);
    ASSERT_EQ(reg_file[R0], 25);
}

// test muli
TEST(muli, testCorrectInputs)
{
    reg_file[R10] = 5;
    cntrl_regs[OPERAND_1] = R0;
    cntrl_regs[OPERAND_2] = R10;
    cntrl_regs[IMMEDIATE] = 20;
    muli();

    ASSERT_EQ(reg_file[R10], 5);
    ASSERT_EQ(reg_file[R0], 100);
}

// test div
TEST(div, testCorrectInputs)
{
    reg_file[R10] = 7;
    reg_file[R12] = 4;
    cntrl_regs[OPERAND_1] = R0;
    cntrl_regs[OPERAND_2] = R10;
    cntrl_regs[OPERAND_3] = R12;
    ASSERT_EQ(div2(), true);
    ASSERT_EQ(reg_file[R10], 7);
    ASSERT_EQ(reg_file[R12], 4);
    ASSERT_EQ(reg_file[R0], 1);

    reg_file[R12] = 0;
    ASSERT_EQ(div2(), false);
    ASSERT_EQ(reg_file[R10], 7);
    ASSERT_EQ(reg_file[R12], 0);
}

// test sdiv
TEST(sdiv, testCorrectInputs)
{
    reg_file[R10] = 249;
    reg_file[R12] = 2;
    cntrl_regs[OPERAND_1] = R0;
    cntrl_regs[OPERAND_2] = R10;
    cntrl_regs[OPERAND_3] = R12;
    ASSERT_EQ(sdiv(), true);
    ASSERT_EQ(reg_file[R10], 249);
    ASSERT_EQ(reg_file[R12], 2);
    ASSERT_EQ(reg_file[R0], 4294967293);

    reg_file[R12] = 0;
    ASSERT_EQ(sdiv(), false);
}

// test divi
TEST(divi, testCorrectInputs)
{
    reg_file[R10] = 126;
    cntrl_regs[OPERAND_1] = R0;
    cntrl_regs[OPERAND_2] = R10;
    cntrl_regs[IMMEDIATE] = 3;
    ASSERT_EQ(divi(), true);
    ASSERT_EQ(reg_file[R10], 126);
    ASSERT_EQ(reg_file[R0], 42);

    cntrl_regs[IMMEDIATE] = 0;
    ASSERT_EQ(divi(), false);
    ASSERT_EQ(reg_file[R10], 126);
}

// test fetch
// here is the fetch function I use
// note that things are a little more functionalized
// bool fetch()
// {
//     if (reg_file[PC] > prog_mem_size - 1) // prog_mem_size is set by init_mem from 'size'
//     {
//         return false;
//     }
//     setCntrlRegs(); // set cntrl regs reads prog_mem[{pc}], prog_mem[{pc}+1], ... prog_mem[{pc+7}]
//                     // into op, operand1, operand2, ... imm
//     reg_file[PC] += 8;
//     return true;
// }
TEST(fetch, TestCorrectInputs)
{
    init_mem(64);
    // set some memory addresses
    reg_file[PC] = 4; // 4 is the first address for storing commands
    prog_mem[4] = 18; // add
    prog_mem[5] = 1;  // store in 1
    prog_mem[6] = 7;  // register 7 +
    prog_mem[7] = 7;  // register 7
    prog_mem[8] = 0;  // DC
    prog_mem[9] = 0;  // DC
    prog_mem[10] = 0; // DC
    prog_mem[11] = 0; // DC

    ASSERT_EQ(fetch(), true);
    ASSERT_EQ(cntrl_regs[OPERATION], 18);
    ASSERT_EQ(cntrl_regs[OPERAND_1], 1);
    ASSERT_EQ(cntrl_regs[OPERAND_2], 7);
    ASSERT_EQ(cntrl_regs[OPERAND_3], 7);
    ASSERT_EQ(cntrl_regs[IMMEDIATE], 0);
    ASSERT_EQ(reg_file[PC], 12);
}

TEST(fetch, TestIncorrectInputs) // pc out of bounds
{
    init_mem(64);
    reg_file[PC] = 64;
    ASSERT_EQ(fetch(), false);
}

// test decode
// the example decode is different then actual, decode has had changes and I cant be bothered to mark them here
// here is the decode function in emu380.cpp
// again I tried to functionalize is as much as I could
// bool decode()
// {
//     if (!naiveLegal()) // naiveLegal checks that the operation code is correct,
//                        // and ensures that all cntr_regs are not > their max (255 for operands, 42...95 for IMM)
//     {
//         return false;
//     }
//     // ensures that checkmap (a map of check functions mapped to the operation code)
//     // contains a check for a given operation
//     if (!checkMap.contains(cntrl_regs[OPERATION]))
//     {
//         return false;
//     }
//     // calls the function in checkmap to ensure that addresses exist that are needed by given operand address
//     if (!checkMap[cntrl_regs[OPERATION]]())
//     {
//         return false;
//     }
//     // sets global instruction function to instruction function stored in function map
//     instructionFunction = instructionMap[cntrl_regs[OPERATION]];
//
//     return true;
// }

TEST(decode, TestCorrectInputs)
{
    init_mem(64);
    // set some memory addresses
    reg_file[PC] = 4; // 4 is the first address for storing commands
    prog_mem[4] = 18; // add
    prog_mem[5] = 1; // store in 1
    prog_mem[6] = 7; // register 7 +
    prog_mem[7] = 7; // register 7
    prog_mem[8] = 0; // DC
    prog_mem[9] = 0; // DC
    prog_mem[10] = 0; // DC
    prog_mem[11] = 0; // DC
    fetch();
    ASSERT_EQ(decode(), true);
}

// test execute
// as before here is the annotated execute function
// bool execute()
// {
//     instructionFunction(); // instruction function set from decode
//     return true;
// }
TEST(execute, TestCorrectInputs)
{
    init_mem(64);
    // set some memory addresses
    reg_file[PC] = 4; // 4 is the first address for storing commands
    reg_file[R7] = 7;
    prog_mem[4] = 18; // add
    prog_mem[5] = 1; // store in 1
    prog_mem[6] = 7; // register 7 +
    prog_mem[7] = 7; // register 7
    prog_mem[8] = 0; // DC
    prog_mem[9] = 0; // DC
    prog_mem[10] = 0; // DC
    prog_mem[11] = 0; // DC
    fetch();
    decode();
    ASSERT_EQ(execute(), true);
    ASSERT_EQ(reg_file[R1], 14);
}
