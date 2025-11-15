import struct
import sys

OPCODES = {
    # jump to, jump to register addr, branch non-zero, b greater than, b less than, b register zero
    'jmp': 1, 'jmr': 2, 'bnz': 3, 'bgt': 4, 'blt': 5, 'brz': 6,
    # move contents to another register, m immediate, load address, store, l address integer, s byte, l addr byte,
    'mov': 7, 'movi': 8, 'lda': 9, 'str': 10, 'ldr': 11, 'stb': 12, 'ldb': 13,
    # indirect store integer, i load integer, i st byte, i l byte
    'istr': 14, 'ildr': 15, 'istb': 16, 'ildb': 17,
    # add, add immediate, subtract, subtract immediate, multiply, multiply immediate
    'add': 18, 'addi': 19, 'sub': 20, 'subi': 21, 'mul': 22, 'muli': 23,
    # division, signed division, division with immediate
    'div': 24, 'sdiv': 25, 'divi': 26,
    # logical instructions (not bitwise)
    'and': 27, 'or': 28,
    # compare registers, compare to immediate
    'cmp': 29, 'cmpi': 30,
    # hardware traps
    'trp': 31,
    # allocate immediate, a from address value, a from indirect value
    'alci': 32, 'allc': 33, 'iallc': 34,
    # push word to stack, p byte to s, pop word from stack, pop byte from stack, push pc to stack, pop pc from stack
    'pshr': 35, 'pshb': 36, 'popr': 37, 'popb': 38, 'call': 39, 'ret': 40,
}

REGISTERS = {
    'r0': 0, 'r1': 1, 'r2': 2, 'r3': 3, 'r4': 4, 'r5': 5, 'r6': 6, 'r7': 7, 'r8': 8,
    'r9': 9, 'r10': 10, 'r11': 11, 'r12': 12, 'r13': 13, 'r14': 14, 'r15': 15,
    'pc': 16, 'sl': 17, 'sb': 18, 'sp': 19, 'fp': 20, 'hp': 21
}

labels = {}
data_section = []
code_section = []
current_address = 0
jmp_main = 0
line_num = 0

def error(msg):
    global line_num
    print(f"Assembler error occurred on line {line_num}!")
    print(msg)
    sys.exit(2)

def parse_immediate(val):
    if not val.startswith('#') and (not val.startswith("'") and not val.endswith("'")):
        error("Parse immediate: immediate value must start with # or '")
    try:
        if val.startswith('#'):
            return parse_number(val)
        else:
            return parse_character(val)
    except Exception:
        error("Parse immediate: immediate value must be valid integer or character")
        sys.exit(2)

def parse_number(num):
    try:
        return int(num[1:])
    except Exception:
        error("Parse number: number value must be valid integer")
        sys.exit(2)

def parse_character(char):
    if not char.startswith("'") or not char.endswith("'"):
        error("Parse error: character must be enclosed in single quotes")

    char_parts = list(char[1:-1]) # remove the ''
    if not char_parts:
        error("Parse error: character must be enclosed in single quotes")

    if char_parts[0] == '\\':
        if len(char_parts) != 2:
            error("Invalid escape sequence")
            sys.exit(2)

        escape_char = char_parts[1]
        if escape_char == 'n':
            return ord('\n')
        elif escape_char == 't':
            return ord('\t')
        elif escape_char == 'r':
            return ord('\r')
        elif escape_char == 'b':
            return ord('\b')
        elif escape_char == '\\':
            return ord('\\')
        elif escape_char == "'":
            return ord("'")
        elif escape_char == '"':
            return ord('"')
        else:
            error("Invalid escape sequence")
            sys.exit(2)

    elif len(char_parts) == 1:
        return ord(char_parts[0])
    else:
        error("Invalid escape sequence")
        sys.exit(2)

def parse_string(string) -> list:
    if not string.startswith('"') or not string.endswith('"'):
        error("Parse error: string must be enclosed in double quotes")
        sys.exit(2)
    string_content = string[1:-1]
    if len(string_content) > 255:
        error("String too long")

    strList = [len(string_content)]
    escape = False

    for char in string_content:
        if escape:
            escape = False
            if char == 'n':
                strList.append(ord('\n'))
            elif char == 't':
                strList.append(ord('\t'))
            elif char == 'r':
                strList.append(ord('\r'))
            elif char == 'b':
                strList.append(ord('\b'))
            elif char == '\\':
                strList.append(ord('\\'))
            elif char == "'":
                strList.append(ord("'"))
            elif char == '"':
                strList.append(ord('"'))
            else:
                error("Invalid escape sequence")
                sys.exit(2)
        elif char == '\\':
            escape = True
        else:
            strList.append(ord(char))

    strList.append(ord('\0'))
    return strList

def parse_register(reg):
    global REGISTERS

    reg = reg.lower()
    if reg not in REGISTERS:
        error("Parse register: invalid register name")

    return REGISTERS[reg]

def is_label(label):
    if not label:
        return False
    if not (label[0].isalnum()):
        return False
    return all(c.isalnum() or c == '_' or c == '$' for c in label)

def process_data_directive(parts, address):
    global labels

    if parts[0].startswith('.'):
        directive = parts[0].lower()
        value = parts[1] if len(parts) > 1 else None
        label = None
    else:
        if len(parts) < 2:
            error("Invalid data directive")
        label = parts[0]
        directive = parts[1].lower()
        value = parts[2] if len(parts) > 2 else None

    if label:
        if label in labels:
            error("Label already defined")
        labels[label] = address

    if directive == '.int':
        if value:
            if value.startswith('#'):
                value = parse_immediate(value)
            else:
                error("Invalid value in .int directive")
        else:
            value = 0

        data_bytes = struct.pack('<I', value)
        data_section.extend(data_bytes)
        return 4

    elif directive == '.byt':
        if value:
            if value.startswith('#'):
                value = parse_immediate(value)
                if value < 0 or value > 255:
                    error("Invalid byte value")
            elif value.startswith("'"):
                value = parse_character(value)
            else:
                error("Invalid value in .byt directive")
        else:
            value = 0

        data_section.append(value)
        return 1

    elif directive == '.bts':
        if not value or not value.startswith('#'):
            error("Invalid value in .bts directive")

        value = parse_immediate(value)
        if value < 0 or value > 255:
            error("Invalid byte value")

        for _ in range(value):
            data_section.append(0)
        return value

    elif directive == '.str':
        if not value:
            error("Invalid value in .str directive")
        value = " ".join(parts[2:])#[1:-1] # join list back together and strip '


        if value.startswith('#'):
            value = parse_immediate(value)
            if value < 0 or value > 255:
                error("Invalid byte value")

            data_section.append(value)
            for _ in range(value):
                data_section.append(0)
            data_section.append(ord("\0"))
            return value + 2
        elif value.startswith('"'):
            strList = parse_string(value)
            for char in strList:
                data_section.append(char)
            return len(strList)
        else:
            error("Invalid value in .str directive")
            sys.exit(2)

    else:
        error("Invalid data directive")
        sys.exit(2)

def encode_instruction(opcode, op1, op2, op3, immediate):
    opcode = opcode & 0xFF
    op1 = op1 & 0xFF
    op2 = op2 & 0xFF
    op3 = op3 & 0xFF
    immediate = immediate & 0xFFFFFFFF

    return struct.pack('<BBBBI', opcode, op1, op2, op3, immediate)

def process_instruction(parts, address):
    global labels, jmp_main

    label = None
    if not parts[0].lower() in OPCODES:
        if len(parts) < 2:
            error("Invalid instruction")

        label = parts[0]
        if not is_label(label):
            error("Invalid label")
        operator = parts[1].lower()
        if len(parts) < 3:
            error("Must include operands")
        operands = parts[2:]
    else:
        operator = parts[0].lower()
        if len(parts) < 2 and not operator == 'ret':
            error("Must include operands")
        operands = parts[1:]

    if label:
        if label in labels:
            error("Label already defined")
        labels[label] = address
    if operator not in OPCODES:
        error("Invalid opcode")
    operator = OPCODES[operator]
    operands = [op.strip() for op in ' '.join(operands).split(',')]

    op1 = op2 = op3 = immediate = 0

    if operator == 1: # jmp
        if len(operands) != 1:
            error("Invalid operands")
        target = operands[0]
        if not is_label(target):
            error("Invalid target")
        code_section.append(('jmp', target, address))
        return 8

    elif operator == 2: # jmr
        if len(operands) != 1:
            error("Invalid operands")
        op1 = parse_register(operands[0])
        code_section.append(('jmr', op1, address))
        return 8

    elif operator in [3, 4, 5, 6]: # [bnz, bgt, blt, brz]
        if len(operands) != 2:
            error("Invalid operands")
        check = parse_register(operands[0])
        target = operands[1]
        if not is_label(target):
            error("Invalid target")
        code_section.append((operator, target, address, check))
        return 8

    elif operator == 7: # mov
        if len(operands) != 2:
            error("Invalid operands")
        op1 = parse_register(operands[0])
        op2 = parse_register(operands[1])

    elif operator == 8: # movi
        if len(operands) != 2:
            error("Invalid operands")
        op1 = parse_register(operands[0])
        immediate = parse_immediate(operands[1])

    elif operator == 9: # lda
        if len(operands) != 2:
            error("Invalid operands")
        op1 = parse_register(operands[0])
        target = operands[1]
        if not is_label(target):
            error("Invalid target")
        code_section.append((operator, target, address, op1))
        return 8

    elif operator in [10, 11, 12, 13]: # ['str', 'ldr', 'stb', 'ldb']
        if len(operands) != 2:
            error("Invalid operands")
        op1 = parse_register(operands[0])
        target = operands[1]
        if not is_label(target):
            error("Invalid target")
        code_section.append((operator, target, address, op1))
        return 8

    elif operator in [14, 15, 16, 17]: # [istr, ildr, istb, ildb]
        if len(operands) != 2:
            error("Invalid operands")
        op1 = parse_register(operands[0])
        op2 = parse_register(operands[1])

    elif operator in [18, 20, 22, 24, 25]: # ['add', 'sub', 'mul', 'div', 'sdiv',]
        if len(operands) != 3:
            error("Invalid operands")
        op1 = parse_register(operands[0])
        op2 = parse_register(operands[1])
        op3 = parse_register(operands[2])

    elif operator in [19, 21, 23, 26]: # ['addi', 'subi', 'muli', 'divi']
        if len(operands) != 3:
            error("Invalid operands")
        op1 = parse_register(operands[0])
        op2 = parse_register(operands[1])
        immediate = parse_immediate(operands[2])

    elif operator in [27, 28]: # [and, or]
        if len(operands) != 3:
            error("Invalid operands")
        op1 = parse_register(operands[0])
        op2 = parse_register(operands[1])
        op3 = parse_register(operands[2])

    elif operator == 29:
        if len(operands) != 3:
            error("Invalid operands")
        op1 = parse_register(operands[0])
        op2 = parse_register(operands[1])
        op3 = parse_register(operands[2])

    elif operator == 30:
        if len(operands) != 3:
            error("Invalid operands")
        op1 = parse_register(operands[0])
        op2 = parse_register(operands[1])
        immediate = parse_immediate(operands[2])

    elif operator == 31: # trp
        if len(operands) != 1:
            error("Invalid operands")
        immediate = parse_immediate(operands[0])

    elif operator == 32: # alci
        if len(operands) != 2:
            error("Invalid operands")
        op1 = parse_register(operands[0])
        immediate = parse_immediate(operands[1])

    elif operator == 33: # allc
        if len(operands) != 2:
            error("Invalid operands")
        op1 = parse_register(operands[0])
        target = operands[1]
        if not is_label(target):
            error("Invalid target")
        code_section.append((operator, target, address, op1))
        return 8

    elif operator == 34: # iallc
        if len(operands) != 2:
            error("Invalid operands")
        op1 = parse_register(operands[0])
        op2 = parse_register(operands[1])

    elif operator in [35, 36, 37, 38]: # [pshr, pshb, popr, popb]
        if len(operands) != 1:
            error("Invalid operands")
        op1 = parse_register(operands[0])

    elif operator == 39: # call
        if len(operands) != 1:
            error("Invalid operands")
        target = operands[0]
        if not is_label(target):
            error("Invalid target")
        code_section.append(('call', target, address))
        return 8

    elif operator == 40: #ret
        if len(operands) != 1:
            error("Invalid operands")

    else:
        error("Unsupported opcode")

    instruction_bytes = encode_instruction(operator, op1, op2, op3, immediate)
    code_section.append(instruction_bytes)
    return 8

def first_pass(filename):
    global line_num, jmp_main, current_address

    try:
        with open(filename, 'r') as f:
            start = f.tell()
            peek = f.readline()
            f.seek(start)

            lines = f.readlines()
    except FileNotFoundError:
        print(f"USAGE: python3 asm4380.py inputFile.asm")
        sys.exit(1)
    except IOError:
        error("Error reading input file")

    in_data_section = True
    found_main = False
    data_address = 4

    peek = peek.strip().split()
    if peek and (peek[0].lower() in OPCODES or (len(peek) > 1 and peek[1].lower() in OPCODES)):
        jmp_main = current_address = 4
        in_data_section = False
        found_main = True

    for i, line in enumerate(lines):
        line_num = i + 1
        line = line.split(';')[0].strip()

        if not line:
            continue

        parts = line.split()
        if in_data_section and parts[0].lower() in OPCODES:
            # short circuit data section
            if line_num < 2:
                jmp_main = current_address = data_address
                in_data_section = False
                found_main = True
                continue

            elif not parts[0].lower() == 'jmp' or len(parts) != 2:
                error("first pass: jmp_main improperly implemented")
                sys.exit(2)

            in_data_section = False
            found_main = True
            jmp_main = current_address = data_address

        if in_data_section:
            size = process_data_directive(parts, data_address)
            data_address += size if size else 0

        else:
            size = process_instruction(parts, current_address)
            current_address += size

    if not found_main:
        error("Missing main function")

def second_pass():
    global code_section

    bytecode = []

    for item in code_section:
        if isinstance(item, bytes):
            bytecode.extend(item)
        else:
            if item[0] == 'jmp':
                _, target, address = item
                if target not in labels:
                    error("Second pass: Undefined label")
                target_address = labels[target]
                instruction_bytes = encode_instruction(OPCODES['jmp'], 0, 0, 0, target_address)
                bytecode.extend(instruction_bytes)
            elif item[0] == 'jmr':
                _, reg, address = item
                instruction_bytes = encode_instruction(OPCODES['jmr'], reg, 0, 0, 0)
                bytecode.extend(instruction_bytes)
            elif item[0] == 'call':
                _, target, address = item
                if target not in labels:
                    error("Second pass: Undefined label")
                target_address = labels[target]
                instruction_bytes = encode_instruction(OPCODES['call'], 0, 0, 0, target_address)
                bytecode.extend(instruction_bytes)
            else:
                operator, target, address, reg = item
                if target not in labels:
                    error("Second pass: Undefined label")

                target_address = labels[target]
                instruction_bytes = encode_instruction(operator, reg, 0, 0, target_address)
                bytecode.extend(instruction_bytes)
    return bytecode

def write_output(filename, bytecode):
    output_filename = filename.replace('.asm', '.bin')

    try:
        with open(output_filename, 'wb') as f:
            f.write(struct.pack('<I', jmp_main))
            f.write(bytes(data_section))
            f.write(bytes(bytecode))
    except IOError:
        error("Error writing to output file")

def main():
    if len(sys.argv) < 2:
        print("USAGE: python3 asm4380.py inputFile.asm")
        sys.exit(1)

    filename = sys.argv[1]

    if not filename.endswith('.asm'):
        print("USAGE: python3 asm4380.py inputFile.asm")
        sys.exit(1)

    first_pass(filename)
    bytecode = second_pass()
    write_output(filename, bytecode)

    sys.exit(0)

if __name__ == "__main__":
    main()
