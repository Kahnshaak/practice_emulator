#include <gtest/gtest.h>
#include <climits>
#include "../include/emu4380.h"
#include <cstring>
#include <string>
#include <climits>
#include <unistd.h>
#include <cstdio> // For the sample test he provided

TEST(fetch, out_of_bounds1) {
  init_mem(1000);
  reg_file[PC] = 1000;
  ASSERT_FALSE(fetch());
}

TEST(fetch, out_of_bounds2) {
  init_mem(1000);
  reg_file[PC] = 1234;
  EXPECT_FALSE(fetch());
}

TEST(fetch, out_of_bounds3) {
  init_mem(1);
  reg_file[PC] = 0;
  EXPECT_FALSE(fetch());
}

TEST(fetch, out_of_bounds4) {
  init_mem(UINT_MAX);
  reg_file[PC] = UINT_MAX - 7;
  EXPECT_FALSE(fetch());
}

TEST(fetch, in_bounds1) {
  init_mem(12345);
  reg_file[PC] = 14;
  EXPECT_TRUE(fetch());
}

TEST(fetch, in_bounds2) {
  init_mem(2049);
  reg_file[PC] = 2049 - 8;
  EXPECT_TRUE(fetch());
}

TEST(fetch, pc1) {
  init_mem(1000);
  reg_file[PC] = 8;
  EXPECT_TRUE(fetch());
  EXPECT_EQ(reg_file[PC], 16);
}

TEST(decode, valid_opcode1) {
  cntrl_regs[OPERATION] = 1;
  cntrl_regs[IMMEDIATE] = 32;
  cntrl_regs[OPERAND_1] = 56356; // DC
  EXPECT_TRUE(decode());
}

TEST(decode, invalid_opcode1) {
  cntrl_regs[OPERATION] = 0;
  EXPECT_FALSE(decode());
}

TEST(decode, invalid_opcode2) {
  cntrl_regs[OPERATION] = 98;
  EXPECT_FALSE(decode());
}

TEST(decode, valid_operand1) {
  cntrl_regs[OPERATION] = 11; // LDR
  cntrl_regs[OPERAND_1] = PC;
  EXPECT_TRUE(decode());
}

TEST(decode, invalid_operand1) {
  cntrl_regs[OPERATION] = 8; // MOVI
  cntrl_regs[OPERAND_1] = 22;
  EXPECT_FALSE(decode());
}

// execute/instruction tests
TEST(instruction, jmp_valid1) {
  init_mem(100000);
  cntrl_regs[OPERATION] = 1; // JMP
  cntrl_regs[IMMEDIATE] = 9999;
  EXPECT_TRUE(execute());
  EXPECT_EQ(reg_file[PC], 9999);
}

TEST(instruction, jmp_invalid1) {
  init_mem(100000);
  reg_file[PC] = 456;
  cntrl_regs[OPERATION] = 1; // JMP
  cntrl_regs[IMMEDIATE] = 999999;
  EXPECT_FALSE(execute());
  EXPECT_EQ(reg_file[PC], 456);
}

TEST(instruction, mov_valid1) {
  init_mem(100000);
  reg_file[R6] = 420;
  cntrl_regs[OPERATION] = 7; // MOV
  cntrl_regs[OPERAND_1] = R5;
  cntrl_regs[OPERAND_2] = R6;
  EXPECT_TRUE(execute());
  EXPECT_EQ(reg_file[R5], 420);
}

TEST(instruction, lda_valid1) {
  init_mem(123456);
  reg_file[R2] = 4;
  cntrl_regs[OPERATION] = 9;
  cntrl_regs[OPERAND_1] = R2;
  cntrl_regs[IMMEDIATE] = 123456;
  // We're not checking bounds anyway... whatever does something next will
  //check if its valid
  EXPECT_TRUE(execute());
  EXPECT_EQ(reg_file[R2], 123456);
}

TEST(instruction, str_valid1) {
  init_mem(131'072);
  cntrl_regs[OPERATION] = 10;
  cntrl_regs[OPERAND_1] = R1;
  cntrl_regs[IMMEDIATE] = 5999;
  reg_file[R1] = 0x12ABCDEF;

  EXPECT_TRUE(execute());
  EXPECT_EQ(prog_mem[5999], 0xEF);
  EXPECT_EQ(prog_mem[6000], 0xCD);
  EXPECT_EQ(prog_mem[6001], 0xAB);
  EXPECT_EQ(prog_mem[6002], 0x12);
}

TEST(instruction, ldr_valid1) {
  init_mem(131'000);
  cntrl_regs[OPERATION] = 11;
  cntrl_regs[IMMEDIATE] = 120'000;
  cntrl_regs[OPERAND_1] = SL;
  prog_mem[120'003] = 0x12;
  prog_mem[120'002] = 0x34;
  prog_mem[120'001] = 0x56;
  prog_mem[120'000] = 0x78;
  EXPECT_TRUE(execute());
  EXPECT_EQ(reg_file[SL], 0x12345678);
}

TEST(instruction, invalid_input){
  init_mem(1000);

  unsigned int start = reg_file[PC];

  cntrl_regs[0] = 1;
  cntrl_regs[4] = 1000;
  cntrl_regs[1] = 0xff; // DC
  cntrl_regs[2] = 5;    // DC
  cntrl_regs[3] = 5;    // DC

  EXPECT_FALSE(execute()) << "JMP Executed despite invalid input";
}

TEST(instruction, valid_input){
  init_mem(131'072);

  //Get PC counter
  unsigned int start = reg_file[PC];

  cntrl_regs[0] = 1;
  cntrl_regs[4] = 1000;
  cntrl_regs[1] = 0xff; // DC
  cntrl_regs[2] = 5;    // DC
  cntrl_regs[3] = 5;    // DC

  EXPECT_TRUE(execute()) << "JMP did not execute despite valid input";
}

TEST(trap_codes, trp1){
  init_mem(131'072);
  testing::internal::CaptureStdout();

  cntrl_regs[0] = 31;
  cntrl_regs[4] = 1;
  cntrl_regs[1] = 0xff; // DC
  cntrl_regs[2] = 5;    // DC
  cntrl_regs[3] = 5;    // DC

  execute();

  std::string output = testing::internal::GetCapturedStdout();
  EXPECT_EQ(output, "0") << "Improper output detected during TRP 1 test";
}

TEST(trap_codes, trp1_R3_correct){
  init_mem(131'072);
  testing::internal::CaptureStdout();

  reg_file[RegNames::R3] = 42;

  cntrl_regs[0] = 31;
  cntrl_regs[4] = 1;
  cntrl_regs[1] = 0xff; // DC
  cntrl_regs[2] = 5;    // DC
  cntrl_regs[3] = 5;    // DC

  execute();

  std::string output = testing::internal::GetCapturedStdout();
  EXPECT_EQ(output, "42") << "Improper output detected during TRP 1 test";
}

TEST(trap_codes, trp2){
  init_mem(131'072); // Regs are all zeroed
  // From Dr J's example
  int fildes[2];
  int status = pipe(fildes);
  ASSERT_NE(status, -1) << "Problem setting up stdin for input TRP tests";

  status = dup2(fildes[0], STDIN_FILENO);
  ASSERT_NE(status, -1) << "Problem setting up stdin for input TRP tests";

  char buf[] = "255\n";
  int bsize = strlen(buf);

  ssize_t nbytes = write(fildes[1], buf, bsize);

  reg_file[RegNames::R3] = 0;
  cntrl_regs[0] = 31;
  cntrl_regs[4] = 2;
  cntrl_regs[1] = 0xff; // DC
  cntrl_regs[2] = 5;    // DC
  cntrl_regs[3] = 5;    // DC

  execute();

  EXPECT_EQ(reg_file[RegNames::R3], 255) << "Improper register value stored during TRP 2 test";
}

TEST(trap_codes, trp2_double_set){
  init_mem(131'072); // Regs are all zeroed
  // From Dr J's example
  int fildes[2];
  int status = pipe(fildes);
  ASSERT_NE(status, -1) << "Problem setting up stdin for input TRP tests";

  status = dup2(fildes[0], STDIN_FILENO);
  ASSERT_NE(status, -1) << "Problem setting up stdin for input TRP tests";

  char buf[] = "255\n";
  int bsize = strlen(buf);

  ssize_t nbytes = write(fildes[1], buf, bsize);

  reg_file[RegNames::R3] = 0;
  cntrl_regs[0] = 31;
  cntrl_regs[4] = 2;
  cntrl_regs[1] = 0xff; // DC
  cntrl_regs[2] = 5;    // DC
  cntrl_regs[3] = 5;    // DC

  execute();

  EXPECT_EQ(reg_file[RegNames::R3], 255) << "Improper register value stored during TRP 2 test";

  char buf2[] = "420\n";
  int bsize2 = strlen(buf2);

  ssize_t nbytes2 = write(fildes[1], buf2, bsize2);

  execute();

  EXPECT_EQ(reg_file[RegNames::R3], 420) << "Improper register value stored during TRP 2 test";
}

TEST(trap_codes, trp3){
  init_mem(131'072);
  testing::internal::CaptureStdout();

  reg_file[R3] = 48; // 0


  cntrl_regs[0] = 31;
  cntrl_regs[4] = 3;
  cntrl_regs[1] = 0xff; // DC
  cntrl_regs[2] = 5;    // DC
  cntrl_regs[3] = 5;    // DC

  execute();

  std::string output = testing::internal::GetCapturedStdout();
  EXPECT_EQ(output, "0") << "Improper output detected during TRP 3 test";
}

TEST(trap_codes, trp3_R3_correct){
  init_mem(131'072);
  testing::internal::CaptureStdout();

  reg_file[R3] = 97;// "a"

  cntrl_regs[0] = 31;
  cntrl_regs[4] = 3;
  cntrl_regs[1] = 0xff; // DC
  cntrl_regs[2] = 5;    // DC
  cntrl_regs[3] = 5;    // DC

  execute();

  std::string output = testing::internal::GetCapturedStdout();
  EXPECT_EQ(output, "a") << "Improper output detected during TRP 1 test";
}

TEST(trap_codes, trp4){
  init_mem(131'072); // Regs are all zeroed
  // From Dr J's example
  int fildes[2];
  int status = pipe(fildes);
  ASSERT_NE(status, -1) << "Problem setting up stdin for input TRP tests";

  status = dup2(fildes[0], STDIN_FILENO);
  ASSERT_NE(status, -1) << "Problem setting up stdin for input TRP tests";

  char buf[] = "a\n";
  int bsize = strlen(buf);

  ssize_t nbytes = write(fildes[1], buf, bsize);

  reg_file[R3] = 0;
  cntrl_regs[0] = 31;
  cntrl_regs[4] = 4;
  cntrl_regs[1] = 0xff; // DC
  cntrl_regs[2] = 5;    // DC
  cntrl_regs[3] = 5;    // DC

  execute();

  EXPECT_EQ(reg_file[R3], 97) << "Improper register value stored during TRP 2 test";
}

TEST(trap_codes, trp4_double_set){
  init_mem(131'072); // Regs are all zeroed
  // From Dr J's example
  int fildes[2];
  int status = pipe(fildes);
  ASSERT_NE(status, -1) << "Problem setting up stdin for input TRP tests";

  status = dup2(fildes[0], STDIN_FILENO);
  ASSERT_NE(status, -1) << "Problem setting up stdin for input TRP tests";

  char buf[] = "a\n";
  int bsize = strlen(buf);

  ssize_t nbytes = write(fildes[1], buf, bsize);

  reg_file[RegNames::R3] = 0;
  cntrl_regs[0] = 31;
  cntrl_regs[4] = 4;
  cntrl_regs[1] = 0xff; // DC
  cntrl_regs[2] = 5;    // DC
  cntrl_regs[3] = 5;    // DC

  execute();

  EXPECT_EQ(reg_file[RegNames::R3],97) << "Improper register value stored during TRP 2 test";

  char buf2[] = "b\n";
  int bsize2 = strlen(buf2);

  ssize_t nbytes2 = write(fildes[1], buf2, bsize2);

  execute();

  EXPECT_EQ(reg_file[RegNames::R3], 'b') << "Improper register value stored during TRP 2 test";
}

//arithmetic
TEST(arithmetic, add){
  init_mem(131'072);

  reg_file[0] = 3;
  reg_file[1] = 2;
  reg_file[2] = 1;


  cntrl_regs[0] = 18;
  cntrl_regs[1] = 2;
  cntrl_regs[2] = 1;
  cntrl_regs[3] = 0;
  cntrl_regs[4] = 0;

  decode();
  execute();

  EXPECT_EQ(reg_file[2], 5) << "Register specified does not match";
}

TEST(arithmetic, addi){
  init_mem(131'072);

  reg_file[0] = 3;
  reg_file[1] = 2;
  reg_file[2] = 0;

  cntrl_regs[0] = 19;
  cntrl_regs[1] = 2;
  cntrl_regs[2] = 0;
  cntrl_regs[3] = 1;
  cntrl_regs[4] = 7;

  decode();
  execute();

  EXPECT_EQ(reg_file[2], 10) << "Register specified does not match";
}

TEST(arithmetic, sub){
  init_mem(131'072);

  reg_file[0] = 3;
  reg_file[1] = 2;
  reg_file[2] = 0;

  cntrl_regs[0] = 20;
  cntrl_regs[1] = 2;
  cntrl_regs[2] = 0;
  cntrl_regs[3] = 1;
  cntrl_regs[4] = 0;

  decode();
  execute();

  EXPECT_EQ(reg_file[2], 1) << "Register specified does not match";
}

TEST(arithmetic, subi){
  init_mem(131'072);

  reg_file[0] = 3;
  reg_file[1] = 2;
  reg_file[2] = 0;

  cntrl_regs[0] = 21;
  cntrl_regs[1] = 2;
  cntrl_regs[2] = 0;
  cntrl_regs[3] = 1;
  cntrl_regs[4] = 1;

  decode();
  execute();

  EXPECT_EQ(reg_file[2], 2) << "Register specified does not match";
}

TEST(arithmetic, mul){
  init_mem(131'072);

  reg_file[0] = 3;
  reg_file[1] = 2;
  reg_file[2] = 0;

  cntrl_regs[0] = 22;
  cntrl_regs[1] = 2;
  cntrl_regs[2] = 0;
  cntrl_regs[3] = 1;
  cntrl_regs[4] = 0;

  decode();
  execute();

  EXPECT_EQ(reg_file[2], 6) << "Register specified does not match";
}

TEST(arithmetic, muli){
  init_mem(131'072);

  reg_file[0] = 3;
  reg_file[1] = 2;
  reg_file[2] = 0;

  cntrl_regs[0] = 23;
  cntrl_regs[1] = 2;
  cntrl_regs[2] = 0;
  cntrl_regs[3] = 1;
  cntrl_regs[4] = 5;

  decode();
  execute();

  EXPECT_EQ(reg_file[2], 15) << "Register specified does not match";
}

TEST(arithmetic, div){
  init_mem(131'072);

  reg_file[0] = 3;
  reg_file[1] = 2;
  reg_file[2] = 0;

  cntrl_regs[0] = 24;
  cntrl_regs[1] = 2;
  cntrl_regs[2] = 0;
  cntrl_regs[3] = 1;
  cntrl_regs[4] = 0;

  decode();
  execute();

  EXPECT_EQ(reg_file[2], 1) << "Register specified does not match";
}

TEST(arithmetic, sdiv){
  init_mem(131'072);

  reg_file[0] = 3;
  reg_file[1] = 2;
  reg_file[2] = 0;

  cntrl_regs[0] = 25;
  cntrl_regs[1] = 2;
  cntrl_regs[2] = 0;
  cntrl_regs[3] = 1;
  cntrl_regs[4] = 0;

  decode();
  execute();

  EXPECT_EQ(reg_file[2], 1) << "Register specified does not match";
}

TEST(arithmetic, divi){
  init_mem(131'072);

  reg_file[0] = 3;
  reg_file[1] = 2;
  reg_file[2] = 0;

  cntrl_regs[0] = 26;
  cntrl_regs[1] = 2;
  cntrl_regs[2] = 0;
  cntrl_regs[3] = 1;
  cntrl_regs[4] = 1;

  decode();
  execute();

  EXPECT_EQ(reg_file[2], 3) << "Register specified does not match";
}

TEST(instruction, movi_valid) {
    init_mem(1000);
    reg_file[R5] = 999;

    cntrl_regs[OPERATION] = 8;
    cntrl_regs[OPERAND_1] = R5;
    cntrl_regs[IMMEDIATE] = 12345;

    EXPECT_TRUE(execute());
    EXPECT_EQ(reg_file[R5], 12345);
}

TEST(instruction, lda_valid) {
    init_mem(100000);
    reg_file[R3] = 0;

    cntrl_regs[OPERATION] = 9;
    cntrl_regs[OPERAND_1] = R3;
    cntrl_regs[IMMEDIATE] = 50000;

    EXPECT_TRUE(execute());
    EXPECT_EQ(reg_file[R3], 50000);
}

TEST(instruction, stb_valid) {
    init_mem(1000);
    reg_file[R7] = 0x12345678;

    cntrl_regs[OPERATION] = 12;
    cntrl_regs[OPERAND_1] = R7;
    cntrl_regs[IMMEDIATE] = 500;

    EXPECT_TRUE(execute());
    EXPECT_EQ(prog_mem[500], 0x78);
}

TEST(instruction, ldb_valid) {
    init_mem(1000);
    prog_mem[600] = 0xAB;
    reg_file[R9] = 0;

    cntrl_regs[OPERATION] = 13;
    cntrl_regs[OPERAND_1] = R9;
    cntrl_regs[IMMEDIATE] = 600;

    EXPECT_TRUE(execute());
    EXPECT_EQ(reg_file[R9], 0xAB);
}

TEST(instruction, trp0_exit) {
    init_mem(1000);
    test_mode = true;

    cntrl_regs[OPERATION] = 31;
    cntrl_regs[IMMEDIATE] = 0;

    EXPECT_TRUE(execute());

    test_mode = false;
}

TEST(instruction, trp98_register_dump) {
    init_mem(1000);
    testing::internal::CaptureStdout();

    reg_file[R0] = 100;
    reg_file[R1] = 200;
    reg_file[PC] = 300;

    cntrl_regs[OPERATION] = 31;
    cntrl_regs[IMMEDIATE] = 98;

    EXPECT_TRUE(execute());

    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_TRUE(output.find("R0\t100") != std::string::npos);
    EXPECT_TRUE(output.find("R1\t200") != std::string::npos);
    EXPECT_TRUE(output.find("PC\t300") != std::string::npos);
}

TEST(instruction, div_by_zero) {
    init_mem(1000);
    reg_file[R1] = 10;
    reg_file[R2] = 0;

    cntrl_regs[OPERATION] = 24;
    cntrl_regs[OPERAND_1] = R3;
    cntrl_regs[OPERAND_2] = R1;
    cntrl_regs[OPERAND_3] = R2;

    EXPECT_FALSE(execute());
}

TEST(instruction, sdiv_by_zero) {
    init_mem(1000);
    reg_file[R1] = -10;
    reg_file[R2] = 0;

    cntrl_regs[OPERATION] = 25;
    cntrl_regs[OPERAND_1] = R3;
    cntrl_regs[OPERAND_2] = R1;
    cntrl_regs[OPERAND_3] = R2;

    EXPECT_FALSE(execute());
}

TEST(instruction, divi_by_zero) {
    init_mem(1000);
    reg_file[R1] = 42;

    cntrl_regs[OPERATION] = 26;
    cntrl_regs[OPERAND_1] = R3;
    cntrl_regs[OPERAND_2] = R1;
    cntrl_regs[IMMEDIATE] = 0;

    EXPECT_FALSE(execute());
}

TEST(registers, all_registers_accessible) {
    init_mem(1000);

    for (int i = 0; i <= 21; i++) {
        cntrl_regs[OPERATION] = 8;
        cntrl_regs[OPERAND_1] = i;
        cntrl_regs[IMMEDIATE] = i * 100;

        if (decode()) {
            EXPECT_TRUE(execute());
            EXPECT_EQ(reg_file[i], i * 100);
        }
    }
}

TEST(registers, invalid_register_decode) {
    init_mem(1000);

    cntrl_regs[OPERATION] = 8;
    cntrl_regs[OPERAND_1] = 22;
    cntrl_regs[IMMEDIATE] = 123;

    EXPECT_FALSE(decode());
}

TEST(memory, little_endian_storage) {
    init_mem(1000);
    reg_file[R1] = 0x12345678;

    cntrl_regs[OPERATION] = 10;
    cntrl_regs[OPERAND_1] = R1;
    cntrl_regs[IMMEDIATE] = 100;

    EXPECT_TRUE(execute());

    EXPECT_EQ(prog_mem[100], 0x78);
    EXPECT_EQ(prog_mem[101], 0x56);
    EXPECT_EQ(prog_mem[102], 0x34);
    EXPECT_EQ(prog_mem[103], 0x12);
}

TEST(memory, little_endian_loading) {
    init_mem(1000);

    prog_mem[200] = 0xEF;
    prog_mem[201] = 0xCD;
    prog_mem[202] = 0xAB;
    prog_mem[203] = 0x89;

    cntrl_regs[OPERATION] = 11;
    cntrl_regs[OPERAND_1] = R5;
    cntrl_regs[IMMEDIATE] = 200;

    EXPECT_TRUE(execute());
    EXPECT_EQ(reg_file[R5], 0x89ABCDEF);
}

TEST(memory, boundary_access_invalid) {
    init_mem(1000);

    cntrl_regs[OPERATION] = 10; // STR
    cntrl_regs[OPERAND_1] = R1;
    cntrl_regs[IMMEDIATE] = 997;

    EXPECT_FALSE(execute());
}

TEST(integration, pc_increment_sequence) {
    init_mem(1000);
    unsigned int initial_pc = 100;
    reg_file[PC] = initial_pc;

    EXPECT_TRUE(fetch());
    EXPECT_EQ(reg_file[PC], initial_pc + 8);

    EXPECT_TRUE(fetch());
    EXPECT_EQ(reg_file[PC], initial_pc + 16);

    EXPECT_TRUE(fetch());
    EXPECT_EQ(reg_file[PC], initial_pc + 24);
}

TEST(integration, mov_chain) {
    init_mem(1000);

    reg_file[R1] = 999;

    cntrl_regs[OPERATION] = 7;
    cntrl_regs[OPERAND_1] = R2;
    cntrl_regs[OPERAND_2] = R1;
    EXPECT_TRUE(execute());
    EXPECT_EQ(reg_file[R2], 999);

    cntrl_regs[OPERAND_1] = R3;
    cntrl_regs[OPERAND_2] = R2;
    EXPECT_TRUE(execute());
    EXPECT_EQ(reg_file[R3], 999);
}

TEST(edge_cases, maximum_memory_size) {
    EXPECT_TRUE(init_mem(1000000));

    cntrl_regs[OPERATION] = 10;
    cntrl_regs[OPERAND_1] = R1;
    cntrl_regs[IMMEDIATE] = 999996;
    reg_file[R1] = 0x12345678;

    EXPECT_TRUE(execute());
}

TEST(edge_cases, signed_arithmetic) {
    init_mem(1000);

    reg_file[R1] = static_cast<unsigned int>(-10);
    reg_file[R2] = 3;

    cntrl_regs[OPERATION] = 25;
    cntrl_regs[OPERAND_1] = R3;
    cntrl_regs[OPERAND_2] = R1;
    cntrl_regs[OPERAND_3] = R2;

    EXPECT_TRUE(decode());
    EXPECT_TRUE(execute());

    EXPECT_EQ(static_cast<int>(reg_file[R3]), -3);
}

TEST(fixes, str_decode_no_bounds_check) {
  init_mem(1000);

  cntrl_regs[OPERATION] = 10;
  cntrl_regs[OPERAND_1] = R1;
  cntrl_regs[IMMEDIATE] = 999999;

  EXPECT_TRUE(decode()) << "STR decode is doing bounds check when it shouldn't";
}

TEST(fixes, ldr_decode_no_bounds_check) {
  init_mem(1000);

  cntrl_regs[OPERATION] = 11;
  cntrl_regs[OPERAND_1] = R2;
  cntrl_regs[IMMEDIATE] = 999999;

  EXPECT_TRUE(decode()) << "LDR decode is doing bounds check when it shouldn't";
}

TEST(fixes, stb_decode_no_bounds_check) {
  init_mem(1000);

  cntrl_regs[OPERATION] = 12;
  cntrl_regs[OPERAND_1] = R1;
  cntrl_regs[IMMEDIATE] = 999999;

  EXPECT_TRUE(decode()) << "STB decode is doing bounds check when it shouldn't";
}

TEST(fixes, lda_execute_large_immediate_value) {
  init_mem(1000);

  reg_file[R1] = 0;
  cntrl_regs[OPERATION] = 9;
  cntrl_regs[OPERAND_1] = R1;
  cntrl_regs[IMMEDIATE] = 0xffffff;

  EXPECT_TRUE(execute()) << "LDA execute is not handling large immediate values";
  EXPECT_EQ(reg_file[R1], 0xffffff) << "LDA execute is not setting the correct value";
}

TEST(fixes, lda_exec_negative_one) {
  init_mem(1000);

  reg_file[R1] = 0;
  cntrl_regs[OPERATION] = 9;
  cntrl_regs[OPERAND_1] = R1;
  cntrl_regs[IMMEDIATE] = static_cast<unsigned int>(-1);

  EXPECT_TRUE(execute()) << "LDA execute is not handling negative one";
  EXPECT_EQ(reg_file[R1], static_cast<unsigned int>(-1)) << "LDA execute is not setting the correct value";
}

TEST(fixes, lda_exec_sb_register) {
  init_mem(1000);

  reg_file[SB] = 0;
  cntrl_regs[OPERATION] = 9;
  cntrl_regs[OPERAND_1] = SB;
  cntrl_regs[IMMEDIATE] = static_cast<unsigned int>(-1);

  EXPECT_TRUE(execute()) << "LDA doesn't executes for SB";
  EXPECT_EQ(reg_file[SB], static_cast<unsigned int>(-1)) << "LDA doesn't sets the correct value for SB";
}

TEST(fixes, divi_signed_division) {
  init_mem(1000);

  reg_file[R1] = 0;
  reg_file[R2] = static_cast<unsigned int>(-20);

  cntrl_regs[OPERATION] = 26;
  cntrl_regs[OPERAND_1] = R1;
  cntrl_regs[OPERAND_2] = R2;
  cntrl_regs[IMMEDIATE] = 3;

  EXPECT_TRUE(decode()) << "DIVI decode should work";
  EXPECT_TRUE(execute()) << "DIVI should execute successfully";
  EXPECT_EQ(static_cast<int>(reg_file[R1]), -6) << "DIVI should perform signed division";
}

TEST(fixes, divi_positive_division) {
  init_mem(1000);

  reg_file[R1] = 0;
  reg_file[R2] = 20;

  cntrl_regs[OPERATION] = 26;
  cntrl_regs[OPERAND_1] = R1;
  cntrl_regs[OPERAND_2] = R2;
  cntrl_regs[IMMEDIATE] = 3;

  EXPECT_TRUE(decode()) << "DIVI decode should work";
  EXPECT_TRUE(execute()) << "DIVI should execute successfully";
  EXPECT_EQ(reg_file[R1], 6) << "DIVI should perform correct division";
}

TEST(fixes, divi_mixed_sign_division) {
  init_mem(1000);

  reg_file[R1] = 0;
  reg_file[R2] = 20;

  cntrl_regs[OPERATION] = 26; // DIVI
  cntrl_regs[OPERAND_1] = R1;
  cntrl_regs[OPERAND_2] = R2;
  cntrl_regs[IMMEDIATE] = static_cast<unsigned int>(-3);

  EXPECT_TRUE(decode()) << "DIVI decode should work";
  EXPECT_TRUE(execute()) << "DIVI should execute successfully";
  EXPECT_EQ(static_cast<int>(reg_file[R1]), -6) << "DIVI should handle mixed signs correctly";
}

TEST(fixes, divi_zero_division) {
  init_mem(1000);

  reg_file[R1] = 0;
  reg_file[R2] = 20;

  cntrl_regs[OPERATION] = 26;
  cntrl_regs[OPERAND_1] = R1;
  cntrl_regs[OPERAND_2] = R2;
  cntrl_regs[IMMEDIATE] = 0;

  EXPECT_FALSE(decode()) << "DIVI decode should fail for zero";
  EXPECT_EQ(reg_file[R1], 0) << "DIVI should fail for zero";
}

TEST(execute_fixes, trp1) {
    init_mem(1000);
    testing::internal::CaptureStdout();

    reg_file[R3] = 0;
    cntrl_regs[OPERATION] = 31;
    cntrl_regs[IMMEDIATE] = 1;

    EXPECT_TRUE(execute()) << "TRP 1 should execute successfully";

    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(output, "0") << "TRP 1 should output zero";
}

TEST(execute_fixes, trp1_forty_two) {
    init_mem(1000);
    testing::internal::CaptureStdout();

    reg_file[R3] = 42;
    cntrl_regs[OPERATION] = 31;
    cntrl_regs[IMMEDIATE] = 1;

    EXPECT_TRUE(execute()) << "TRP 1 should execute successfully";

    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(output, "42") << "TRP 1 should output 42";
}

TEST(execute_fixes, trp1_large_number) {
    init_mem(1000);
    testing::internal::CaptureStdout();

    reg_file[R3] = 1024;
    cntrl_regs[OPERATION] = 31;
    cntrl_regs[IMMEDIATE] = 1;

    EXPECT_TRUE(execute()) << "TRP 1 should execute successfully";

    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(output, "1024") << "TRP 1 should output 1024";
}

TEST(execute_fixes, trp3) {
    init_mem(1000);
    testing::internal::CaptureStdout();

    reg_file[R3] = '*';
    cntrl_regs[OPERATION] = 31;
    cntrl_regs[IMMEDIATE] = 3;

    EXPECT_TRUE(execute()) << "TRP 3 should execute successfully";

    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(output, "*") << "TRP 3 should output '*'";
}

TEST(execute_fixes, trp98_correct_format) {
    init_mem(1000);
    testing::internal::CaptureStdout();

    reg_file[R0] = 21;
    reg_file[R1] = 20;
    reg_file[R2] = 19;
    reg_file[R3] = 18;
    reg_file[R4] = 17;
    reg_file[R5] = 16;
    reg_file[R6] = 15;
    reg_file[R7] = 14;
    reg_file[R8] = 13;
    reg_file[R9] = 12;
    reg_file[R10] = 11;
    reg_file[R11] = 10;
    reg_file[R12] = 9;
    reg_file[R13] = 8;
    reg_file[R14] = 7;
    reg_file[R15] = 6;
    reg_file[PC] = 5;
    reg_file[SL] = 4;
    reg_file[SB] = 3;
    reg_file[SP] = 2;
    reg_file[FP] = 1;
    reg_file[HP] = 0;

    cntrl_regs[OPERATION] = 31;
    cntrl_regs[IMMEDIATE] = 98;

    EXPECT_TRUE(execute()) << "TRP 98 should execute successfully";

    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_TRUE(output.find("R0\t21\n") != std::string::npos) << "TRP 98 should use tab-separated format";
    EXPECT_TRUE(output.find("R1\t20\n") != std::string::npos) << "TRP 98 should format R1 correctly";
    EXPECT_TRUE(output.find("PC\t5\n") != std::string::npos) << "TRP 98 should format PC correctly";
    EXPECT_TRUE(output.find("SB\t3\n") != std::string::npos) << "TRP 98 should format SB correctly";
    EXPECT_TRUE(output.find("HP\t0\n") != std::string::npos) << "TRP 98 should format HP correctly";

}

TEST(cache, init_cache_direct_mapped) {
    init_mem(1000);
    init_cache(1);

    prog_mem[100] = 0x42;
    unsigned char result = readByte(100);
    EXPECT_EQ(result, 0x42);
}

TEST(cache, init_cache_fully_associative) {
    init_mem(1000);
    init_cache(2);

    prog_mem[200] = 0xAB;
    unsigned char result = readByte(200);
    EXPECT_EQ(result, 0xAB);
}

TEST(cache, init_cache_two_way_set_associative) {
    init_mem(1000);
    init_cache(3);

    prog_mem[300] = 0xCD;
    unsigned char result = readByte(300);
    EXPECT_EQ(result, 0xCD);
}

TEST(cache, no_cache_operation) {
    init_mem(1000);
    init_cache(0);

    prog_mem[400] = 0xEF;
    unsigned char result = readByte(400);
    EXPECT_EQ(result, 0xEF);
}

TEST(cache, memory_cycle_counting_no_cache) {
    init_mem(1000);
    init_cache(0);
    mem_cycle_cntr = 0;
    memStream = false;

    readByte(100);
    EXPECT_EQ(mem_cycle_cntr, 8);

    readByte(101);
    EXPECT_EQ(mem_cycle_cntr, 10); // 8 + 2
}

TEST(branches, jmr_valid) {
    init_mem(1000);
    reg_file[R5] = 500;
    reg_file[PC] = 100;

    cntrl_regs[OPERATION] = 2; // JMR
    cntrl_regs[OPERAND_1] = R5;

    EXPECT_TRUE(execute());
    EXPECT_EQ(reg_file[PC], 500);
}

TEST(branches, bnz_taken) {
    init_mem(1000);
    reg_file[R3] = 5;
    reg_file[PC] = 100;

    cntrl_regs[OPERATION] = 3; // BNZ
    cntrl_regs[OPERAND_1] = R3;
    cntrl_regs[IMMEDIATE] = 200;

    EXPECT_TRUE(execute());
    EXPECT_EQ(reg_file[PC], 200);
}

TEST(branches, bnz_not_taken) {
    init_mem(1000);
    reg_file[R3] = 0;
    reg_file[PC] = 100;

    cntrl_regs[OPERATION] = 3; // BNZ
    cntrl_regs[OPERAND_1] = R3;
    cntrl_regs[IMMEDIATE] = 200;

    EXPECT_TRUE(execute());
    EXPECT_EQ(reg_file[PC], 100);
}

TEST(branches, bgt_taken_positive) {
    init_mem(1000);
    reg_file[R2] = 10;
    reg_file[PC] = 150;

    cntrl_regs[OPERATION] = 4; // BGT
    cntrl_regs[OPERAND_1] = R2;
    cntrl_regs[IMMEDIATE] = 300;

    EXPECT_TRUE(execute());
    EXPECT_EQ(reg_file[PC], 300);
}

TEST(branches, bgt_not_taken_zero) {
    init_mem(1000);
    reg_file[R2] = 0;
    reg_file[PC] = 150;

    cntrl_regs[OPERATION] = 4; // BGT
    cntrl_regs[OPERAND_1] = R2;
    cntrl_regs[IMMEDIATE] = 300;

    EXPECT_TRUE(execute());
    EXPECT_EQ(reg_file[PC], 150);
}

TEST(branches, blt_taken_negative) {
    init_mem(1000);
    reg_file[R1] = static_cast<unsigned int>(-5);
    reg_file[PC] = 75;

    cntrl_regs[OPERATION] = 5; // BLT
    cntrl_regs[OPERAND_1] = R1;
    cntrl_regs[IMMEDIATE] = 400;

    EXPECT_TRUE(execute());
    EXPECT_EQ(reg_file[PC], 400);
}

TEST(branches, brz_taken) {
    init_mem(1000);
    reg_file[R4] = 0;
    reg_file[PC] = 250;

    cntrl_regs[OPERATION] = 6; // BRZ
    cntrl_regs[OPERAND_1] = R4;
    cntrl_regs[IMMEDIATE] = 500;

    EXPECT_TRUE(execute());
    EXPECT_EQ(reg_file[PC], 500);
}

TEST(indirect, istr_valid) {
    init_mem(1000);
    reg_file[R1] = 0x12345678;
    reg_file[R2] = 600;

    cntrl_regs[OPERATION] = 14; // ISTR
    cntrl_regs[OPERAND_1] = R1;
    cntrl_regs[OPERAND_2] = R2;

    EXPECT_TRUE(execute());

    EXPECT_EQ(prog_mem[600], 0x78);
    EXPECT_EQ(prog_mem[601], 0x56);
    EXPECT_EQ(prog_mem[602], 0x34);
    EXPECT_EQ(prog_mem[603], 0x12);
}

TEST(indirect, ildr_valid) {
    init_mem(1000);

    prog_mem[700] = 0xAB;
    prog_mem[701] = 0xCD;
    prog_mem[702] = 0xEF;
    prog_mem[703] = 0x12;

    reg_file[R3] = 700;
    reg_file[R1] = 0;

    cntrl_regs[OPERATION] = 15; // ILDR
    cntrl_regs[OPERAND_1] = R1;
    cntrl_regs[OPERAND_2] = R3;

    EXPECT_TRUE(execute());
    EXPECT_EQ(reg_file[R1], 0x12EFCDAB);
}

TEST(indirect, istb_valid) {
    init_mem(1000);
    reg_file[R5] = 0x12345678;
    reg_file[R6] = 800;

    cntrl_regs[OPERATION] = 16; // ISTB
    cntrl_regs[OPERAND_1] = R5;
    cntrl_regs[OPERAND_2] = R6;

    EXPECT_TRUE(execute());
    EXPECT_EQ(prog_mem[800], 0x78);
}

TEST(indirect, ildb_valid) {
    init_mem(1000);
    prog_mem[900] = 0x42;
    reg_file[R7] = 900;
    reg_file[R8] = 0;

    cntrl_regs[OPERATION] = 17; // ILDB
    cntrl_regs[OPERAND_1] = R8;
    cntrl_regs[OPERAND_2] = R7;

    EXPECT_TRUE(execute());
    EXPECT_EQ(reg_file[R8], 0x42);
}

TEST(comparison, cmp_equal) {
    init_mem(1000);
    reg_file[R1] = 42;
    reg_file[R2] = 42;
    reg_file[R3] = 999;

    cntrl_regs[OPERATION] = 29; // CMP
    cntrl_regs[OPERAND_1] = R3;
    cntrl_regs[OPERAND_2] = R1;
    cntrl_regs[OPERAND_3] = R2;

    EXPECT_TRUE(execute());
    EXPECT_EQ(reg_file[R3], 0);
}

TEST(comparison, cmp_greater) {
    init_mem(1000);
    reg_file[R1] = 50;
    reg_file[R2] = 30;
    reg_file[R3] = 999;

    cntrl_regs[OPERATION] = 29; // CMP
    cntrl_regs[OPERAND_1] = R3;
    cntrl_regs[OPERAND_2] = R1;
    cntrl_regs[OPERAND_3] = R2;

    EXPECT_TRUE(execute());
    EXPECT_EQ(reg_file[R3], 1);
}

TEST(comparison, cmp_less) {
    init_mem(1000);
    reg_file[R1] = 10;
    reg_file[R2] = 20;
    reg_file[R3] = 999;

    cntrl_regs[OPERATION] = 29; // CMP
    cntrl_regs[OPERAND_1] = R3;
    cntrl_regs[OPERAND_2] = R1;
    cntrl_regs[OPERAND_3] = R2;

    EXPECT_TRUE(execute());
    EXPECT_EQ(reg_file[R3], static_cast<unsigned int>(-1));
}

TEST(comparison, cmpi_equal) {
    init_mem(1000);
    reg_file[R1] = 100;
    reg_file[R2] = 999;

    cntrl_regs[OPERATION] = 30; // CMPI
    cntrl_regs[OPERAND_1] = R2;
    cntrl_regs[OPERAND_2] = R1;
    cntrl_regs[IMMEDIATE] = 100;

    EXPECT_TRUE(execute());
    EXPECT_EQ(reg_file[R2], 0);
}

TEST(comparison, cmpi_with_negative_immediate) {
    init_mem(1000);
    reg_file[R1] = static_cast<unsigned int>(-10);
    reg_file[R2] = 999;

    cntrl_regs[OPERATION] = 30; // CMPI
    cntrl_regs[OPERAND_1] = R2;
    cntrl_regs[OPERAND_2] = R1;
    cntrl_regs[IMMEDIATE] = static_cast<unsigned int>(-5);

    EXPECT_TRUE(execute());
    EXPECT_EQ(reg_file[R2], static_cast<unsigned int>(-1)); // -10 < -5
}

TEST(boundary, str_at_memory_boundary) {
    init_mem(1000);
    reg_file[R1] = 0x12345678;

    cntrl_regs[OPERATION] = 10; // STR
    cntrl_regs[OPERAND_1] = R1;
    cntrl_regs[IMMEDIATE] = 997;

    EXPECT_FALSE(execute());
}

TEST(boundary, ldr_at_memory_boundary) {
    init_mem(1000);
    reg_file[R1] = 0;

    cntrl_regs[OPERATION] = 11; // LDR
    cntrl_regs[OPERAND_1] = R1;
    cntrl_regs[IMMEDIATE] = 997;

    EXPECT_FALSE(execute());
}

TEST(boundary, stb_at_exact_boundary) {
    init_mem(1000);
    reg_file[R1] = 0x42;

    cntrl_regs[OPERATION] = 12; // STB
    cntrl_regs[OPERAND_1] = R1;
    cntrl_regs[IMMEDIATE] = 1000;

    EXPECT_FALSE(execute());
}

TEST(boundary, ldb_at_exact_boundary) {
    init_mem(1000);
    reg_file[R1] = 0;

    cntrl_regs[OPERATION] = 13; // LDB
    cntrl_regs[OPERAND_1] = R1;
    cntrl_regs[IMMEDIATE] = 1000;

    EXPECT_FALSE(execute());
}

TEST(decode, jmp_invalid_address) {
    init_mem(1000);

    cntrl_regs[OPERATION] = 1; // JMP
    cntrl_regs[IMMEDIATE] = 1000;

    EXPECT_FALSE(decode());
}

TEST(decode, branch_invalid_register) {
    init_mem(1000);

    cntrl_regs[OPERATION] = 3; // BNZ
    cntrl_regs[OPERAND_1] = 25;
    cntrl_regs[IMMEDIATE] = 100;

    EXPECT_FALSE(decode());
}

TEST(decode, branch_invalid_address) {
    init_mem(1000);

    cntrl_regs[OPERATION] = 4; // BGT
    cntrl_regs[OPERAND_1] = R1;
    cntrl_regs[IMMEDIATE] = 2000;

    EXPECT_FALSE(decode());
}

TEST(decode, three_operand_invalid_register) {
    init_mem(1000);

    cntrl_regs[OPERATION] = 18; // ADD
    cntrl_regs[OPERAND_1] = R1;
    cntrl_regs[OPERAND_2] = R2;
    cntrl_regs[OPERAND_3] = 30;

    EXPECT_FALSE(decode());
}

TEST(decode, trp_invalid_code) {
    init_mem(1000);

    cntrl_regs[OPERATION] = 31; // TRP
    cntrl_regs[IMMEDIATE] = 50;

    EXPECT_FALSE(decode());
}

TEST(memory_streaming, consecutive_reads_no_cache) {
    init_mem(1000);
    init_cache(0);
    mem_cycle_cntr = 0;

    prog_mem[100] = 0x11;
    prog_mem[101] = 0x22;
    prog_mem[102] = 0x33;

    unsigned char val1 = readByte(100); // 8 cycles
    unsigned char val2 = readByte(101); // 2
    unsigned char val3 = readByte(102); // 2

    EXPECT_EQ(val1, 0x11);
    EXPECT_EQ(val2, 0x22);
    EXPECT_EQ(val3, 0x33);
    EXPECT_EQ(mem_cycle_cntr, 12); // 8 + 2 + 2
}

TEST(memory_streaming, word_read_no_cache) {
    init_mem(1000);
    init_cache(0);
    mem_cycle_cntr = 0;
    memStream = false;

    prog_mem[200] = 0x78;
    prog_mem[201] = 0x56;
    prog_mem[202] = 0x34;
    prog_mem[203] = 0x12;

    unsigned int word = readWord(200);

    EXPECT_EQ(word, 0x12345678);
    EXPECT_EQ(mem_cycle_cntr, 8);
}

TEST(special_registers, pc_operations) {
    init_mem(1000);

    cntrl_regs[OPERATION] = 8; // MOVI
    cntrl_regs[OPERAND_1] = PC;
    cntrl_regs[IMMEDIATE] = 500;

    EXPECT_TRUE(decode());
    EXPECT_TRUE(execute());
    EXPECT_EQ(reg_file[PC], 500);
}

TEST(special_registers, stack_pointer_operations) {
    init_mem(1000);

    reg_file[SP] = 800;
    reg_file[R1] = 100;

    cntrl_regs[OPERATION] = 18; // ADD
    cntrl_regs[OPERAND_1] = SP;
    cntrl_regs[OPERAND_2] = SP;
    cntrl_regs[OPERAND_3] = R1;

    EXPECT_TRUE(decode());
    EXPECT_TRUE(execute());
    EXPECT_EQ(reg_file[SP], 900);
}

TEST(fetch, instruction_parsing) {
    init_mem(1000);

    prog_mem[0] = 7;
    prog_mem[1] = 1;
    prog_mem[2] = 2;
    prog_mem[3] = 3;

    prog_mem[4] = 0x78;
    prog_mem[5] = 0x56;
    prog_mem[6] = 0x34;
    prog_mem[7] = 0x12;

    reg_file[PC] = 0;

    EXPECT_TRUE(fetch());

    EXPECT_EQ(cntrl_regs[OPERATION], 7);
    EXPECT_EQ(cntrl_regs[OPERAND_1], 1);
    EXPECT_EQ(cntrl_regs[OPERAND_2], 2);
    EXPECT_EQ(cntrl_regs[OPERAND_3], 3);
    EXPECT_EQ(cntrl_regs[IMMEDIATE], 0x12345678);
    EXPECT_EQ(reg_file[PC], 8);
}

TEST(integration, simple_program) {
    init_mem(1000);

    reg_file[PC] = 100;
    cntrl_regs[OPERATION] = 8;
    cntrl_regs[OPERAND_1] = R1;
    cntrl_regs[IMMEDIATE] = 42;
    EXPECT_TRUE(execute());
    EXPECT_EQ(reg_file[R1], 42);

    cntrl_regs[OPERATION] = 8;
    cntrl_regs[OPERAND_1] = R2;
    cntrl_regs[IMMEDIATE] = 8;
    EXPECT_TRUE(execute());
    EXPECT_EQ(reg_file[R2], 8);

    cntrl_regs[OPERATION] = 18;
    cntrl_regs[OPERAND_1] = R3;
    cntrl_regs[OPERAND_2] = R1;
    cntrl_regs[OPERAND_3] = R2;
    EXPECT_TRUE(execute());
    EXPECT_EQ(reg_file[R3], 50);
}

TEST(integration, memory_store_load_cycle) {
    init_mem(1000);

    reg_file[R1] = 0xDEADBEEF;

    cntrl_regs[OPERATION] = 10;
    cntrl_regs[OPERAND_1] = R1;
    cntrl_regs[IMMEDIATE] = 500;
    EXPECT_TRUE(execute());

    reg_file[R1] = 0;

    cntrl_regs[OPERATION] = 11;
    cntrl_regs[OPERAND_1] = R1;
    cntrl_regs[IMMEDIATE] = 500;
    EXPECT_TRUE(execute());

    EXPECT_EQ(reg_file[R1], 0xDEADBEEF);
}

TEST(edge_cases, overflow_arithmetic) {
    init_mem(1000);

    reg_file[R1] = UINT_MAX;
    reg_file[R2] = 1;

    cntrl_regs[OPERATION] = 18; // ADD
    cntrl_regs[OPERAND_1] = R3;
    cntrl_regs[OPERAND_2] = R1;
    cntrl_regs[OPERAND_3] = R2;

    EXPECT_TRUE(execute());
    EXPECT_EQ(reg_file[R3], 0); // Overflow
}

TEST(edge_cases, underflow_arithmetic) {
    init_mem(1000);

    reg_file[R1] = 0;
    reg_file[R2] = 1;

    cntrl_regs[OPERATION] = 20; // SUB
    cntrl_regs[OPERAND_1] = R3;
    cntrl_regs[OPERAND_2] = R1;
    cntrl_regs[OPERAND_3] = R2;

    EXPECT_TRUE(execute());
    EXPECT_EQ(reg_file[R3], UINT_MAX); // Underflow
}

TEST(edge_cases, very_small_memory) {
    init_mem(4);
    reg_file[PC] = 0;
    EXPECT_FALSE(fetch());
}
