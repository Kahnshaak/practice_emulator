#!/usr/bin/env python3

import pytest
import sys
import os
import struct
import tempfile
from unittest.mock import patch
from io import StringIO

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'assembler'))

import asm4380

class TestASM4380:
    
    def setup_method(self):
        asm4380.labels = {}
        asm4380.data_section = []
        asm4380.code_section = []
        asm4380.current_address = 0
        asm4380.jmp_main = 0
        asm4380.line_num = 0
    
    def test_parse_immediate_valid(self):
        assert asm4380.parse_immediate('#42') == 42
        assert asm4380.parse_immediate('#-10') == -10
        assert asm4380.parse_immediate('#0') == 0
        assert asm4380.parse_immediate("'A'") == 65
    
    def test_parse_immediate_invalid(self):
        with pytest.raises(SystemExit) as excinfo:
            asm4380.parse_immediate('42')
        assert excinfo.value.code == 2
    
    def test_parse_number_valid(self):
        assert asm4380.parse_number('#42') == 42
        assert asm4380.parse_number('#-10') == -10
        assert asm4380.parse_number('#0') == 0
    
    def test_parse_number_invalid(self):
        with pytest.raises(SystemExit) as excinfo:
            asm4380.parse_number('#abc')
        assert excinfo.value.code == 2
    
    def test_parse_character_valid(self):
        assert asm4380.parse_character("'A'") == 65
        assert asm4380.parse_character("'0'") == 48
        assert asm4380.parse_character("'#'") == 35
    
    def test_parse_character_invalid(self):
        with pytest.raises(SystemExit) as excinfo:
            asm4380.parse_character("'AB'")  # Too long
        assert excinfo.value.code == 2
        
        with pytest.raises(SystemExit) as excinfo:
            asm4380.parse_character("A")  # Missing quotes
        assert excinfo.value.code == 2
    
    def test_parse_register_valid(self):
        assert asm4380.parse_register('r0') == 0
        assert asm4380.parse_register('r15') == 15
        assert asm4380.parse_register('pc') == 16
        assert asm4380.parse_register('sp') == 19
        assert asm4380.parse_register('hp') == 21
    
    def test_parse_register_invalid(self):
        # with pytest.raises(SystemExit) as excinfo:
        #     asm4380.parse_register('R0')
        # assert excinfo.value.code == 2
        
        with pytest.raises(SystemExit) as excinfo:
            asm4380.parse_register('XX')
        assert excinfo.value.code == 2
    
    def test_is_label_valid(self):
        assert asm4380.is_label('MAIN') == True
        assert asm4380.is_label('loop_1') == True
        assert asm4380.is_label('func$1') == True
        assert asm4380.is_label('A') == True
        assert asm4380.is_label('test123') == True
    
    def test_is_label_invalid(self):
        assert asm4380.is_label('_bad') == False
        assert asm4380.is_label('$bad') == False
        assert asm4380.is_label('lab-el') == False
    
    def test_strip_comments(self):
        assert asm4380.strip_comments('mov r1, r2 ; this is a comment') == 'mov r1, r2'
        assert asm4380.strip_comments('; full line comment') == ''
        assert asm4380.strip_comments('mov r1, r2') == 'mov r1, r2'
        assert asm4380.strip_comments('') == ''
    
    def test_encode_instruction(self):
        encoded = asm4380.encode_instruction(7, 1, 2, 0, 0)
        expected = struct.pack('<BBBBi', 7, 1, 2, 0, 0)
        assert encoded == expected
        
        encoded = asm4380.encode_instruction(8, 5, 0, 0, 42)
        expected = struct.pack('<BBBBi', 8, 5, 0, 0, 42)
        assert encoded == expected
    
    def test_data_directive_int(self):
        size = asm4380.process_data_directive(['MYINT', '.INT', '#42'], 100)
        assert size == 4
        assert asm4380.labels['MYINT'] == 100
        assert len(asm4380.data_section) == 4
        
        expected = struct.pack('<i', 42)
        assert bytes(asm4380.data_section[-4:]) == expected
    
    def test_data_directive_byt(self):
        size = asm4380.process_data_directive(['CHAR', '.BYT', "'A'"], 100)
        assert size == 1
        assert asm4380.labels['CHAR'] == 100
        assert asm4380.data_section[-1] == 65
        
        size = asm4380.process_data_directive(['.BYT', '#255'], 101)
        assert size == 1
        assert asm4380.data_section[-1] == 255
    
    def test_duplicate_label_error(self):
        asm4380.process_data_directive(['LABEL1', '.INT', '#1'], 100)
        
        with pytest.raises(SystemExit) as excinfo:
            asm4380.process_data_directive(['LABEL1', '.INT', '#2'], 104)
        assert excinfo.value.code == 2

    def create_temp_asm_file(self, content):
        temp_file = tempfile.NamedTemporaryFile(mode='w', suffix='.asm', delete=False)
        temp_file.write(content)
        temp_file.close()
        return temp_file.name

    def test_simple_program_assembly(self):
        program = """
        VALUE .INT #42
        
        jmp MAIN
        MAIN movi r1, #10
              ldr r2, VALUE
              add r3, r1, r2
              trp #0
        """
        
        temp_file = self.create_temp_asm_file(program)
        
        try:
            with patch.object(sys, 'argv', ['asm4380.py', temp_file]):
                try:
                    asm4380.main()
                    assert False, "Expected SystemExit"
                except SystemExit as e:
                    assert e.code == 0
            
            output_file = temp_file.replace('.asm', '.bin')
            assert os.path.exists(output_file)
            
            with open(output_file, 'rb') as f:
                main_addr = struct.unpack('<i', f.read(4))[0]
                assert main_addr == 8
                
                data_bytes = f.read(4)
                value = struct.unpack('<i', data_bytes)[0]
                assert value == 42
                
                instruction = f.read(8)
                opcode = instruction[0]
                assert opcode == 1
        
        finally:
            if os.path.exists(temp_file):
                os.unlink(temp_file)
            output_file = temp_file.replace('.asm', '.bin')
            if os.path.exists(output_file):
                os.unlink(output_file)
    
    def test_missing_jmp_main_error(self):
        program = """
        VALUE .INT #42
        
        MAIN movi r1, #10
              trp #0
        """
        
        temp_file = self.create_temp_asm_file(program)
        
        try:
            with patch.object(sys, 'argv', ['asm4380.py', temp_file]):
                try:
                    asm4380.main()
                    assert False, "Expected SystemExit with code 2"
                except SystemExit as e:
                    assert e.code == 2
        
        finally:
            if os.path.exists(temp_file):
                os.unlink(temp_file)
    
    def test_undefined_label_error(self):
        program = """
        jmp MAIN
        MAIN jmp UNDEFINED_LABEL
              trp #0
        """
        
        temp_file = self.create_temp_asm_file(program)
        
        try:
            with patch.object(sys, 'argv', ['asm4380.py', temp_file]):
                try:
                    asm4380.main()
                    assert False, "Expected SystemExit with code 2"
                except SystemExit as e:
                    assert e.code == 2
        
        finally:
            if os.path.exists(temp_file):
                os.unlink(temp_file)
    
    def test_invalid_instruction_error(self):
        program = """
        jmp MAIN
        MAIN INVALID_OP r1, r2
              trp #0
        """
        
        temp_file = self.create_temp_asm_file(program)
        
        try:
            with patch.object(sys, 'argv', ['asm4380.py', temp_file]):
                try:
                    asm4380.main()
                    assert False, "Expected SystemExit with code 2"
                except SystemExit as e:
                    assert e.code == 2
        
        finally:
            if os.path.exists(temp_file):
                os.unlink(temp_file)
    
    def test_wrong_operand_count_error(self):
        program = """
        jmp MAIN
        MAIN mov r1
              trp #0
        """
        
        temp_file = self.create_temp_asm_file(program)
        
        try:
            with patch.object(sys, 'argv', ['asm4380.py', temp_file]):
                try:
                    asm4380.main()
                    assert False, "Expected SystemExit with code 2"
                except SystemExit as e:
                    assert e.code == 2
        
        finally:
            if os.path.exists(temp_file):
                os.unlink(temp_file)
    
    def test_usage_error_no_args(self):
        with patch.object(sys, 'argv', ['asm4380.py']):
            with patch('builtins.print') as mock_print:
                try:
                    asm4380.main()
                    assert False, "Expected SystemExit with code 1"
                except SystemExit as e:
                    assert e.code == 1
                    mock_print.assert_called_with("USAGE: python3 asm4380.py inputFile.asm")
    
    def test_usage_error_wrong_extension(self):
        with patch.object(sys, 'argv', ['asm4380.py', 'test.txt']):
            with patch('builtins.print') as mock_print:
                try:
                    asm4380.main()
                    assert False, "Expected SystemExit with code 1"
                except SystemExit as e:
                    assert e.code == 1
                    mock_print.assert_called_with("USAGE: python3 asm4380.py inputFile.asm")
    
    def test_usage_error_file_not_found(self):
        with patch.object(sys, 'argv', ['asm4380.py', 'nonexistent.asm']):
            with patch('builtins.print') as mock_print:
                try:
                    asm4380.main()
                    assert False, "Expected SystemExit with code 1"
                except SystemExit as e:
                    assert e.code == 1
                    mock_print.assert_called_with("USAGE: python3 asm4380.py inputFile.asm")
    
    def test_all_instruction_types(self):
        program = """
        DATA_VAL .INT #100
        BYTE_VAL .BYT #50
        
        jmp MAIN
        MAIN     mov r1, r2
                  movi r3, #42
                  lda r4, DATA_VAL
                  str r5, DATA_VAL
                  ldr r6, DATA_VAL
                  stb r7, BYTE_VAL
                  ldb r8, BYTE_VAL
                  add r9, r10, r11
                  addi r12, r13, #10
                  sub r14, r15, r0
                  subi r1, r2, #5
                  mul r3, r4, r5
                  muli r6, r7, #3
                  div r8, r9, r10
                  sdiv r11, r12, r13
                  divi r14, r15, #2
                  trp #0
        """
        
        temp_file = self.create_temp_asm_file(program)
        
        try:
            with patch.object(sys, 'argv', ['asm4380.py', temp_file]):
                try:
                    asm4380.main()
                except SystemExit as e:
                    assert e.code == 0
            
            output_file = temp_file.replace('.asm', '.bin')
            assert os.path.exists(output_file)
        
        finally:
            if os.path.exists(temp_file):
                os.unlink(temp_file)
            output_file = temp_file.replace('.asm', '.bin')
            if os.path.exists(output_file):
                os.unlink(output_file)
    
    def test_case_insensitive_instructions(self):
        program = """
        jmp main
        main mov r1, r2
              MOVI r3, #42
              Add r4, r5, r6
              trp #0
        """
        
        temp_file = self.create_temp_asm_file(program)
        
        try:
            with patch.object(sys, 'argv', ['asm4380.py', temp_file]):
                try:
                    asm4380.main()
                except SystemExit as e:
                    assert e.code == 0
        
        finally:
            if os.path.exists(temp_file):
                os.unlink(temp_file)
    
    def test_case_sensitive_labels(self):
        program = """
        jmp MAIN
        Main trp #0
        MAIN jmp Main
        """
        
        temp_file = self.create_temp_asm_file(program)
        
        try:
            with patch.object(sys, 'argv', ['asm4380.py', temp_file]):
                try:
                    asm4380.main()
                except SystemExit as e:
                    assert e.code == 0
        
        finally:
            if os.path.exists(temp_file):
                os.unlink(temp_file)

if __name__ == '__main__':
    pytest.main([__file__, '-v'])
