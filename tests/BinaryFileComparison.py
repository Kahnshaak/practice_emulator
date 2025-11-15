import sys

def compare_binary_files(file1, file2):
    with open(file1, 'rb') as f1, open(file2, 'rb') as f2:
        data1 = f1.read()
        data2 = f2.read()

    print(f"File1 ({file1}) has {len(data1)} bytes")
    print(f"File2 ({file2}) has {len(data2)} bytes")
    print()

    max_len = max(len(data1), len(data2))
    diff_bits = 0

    for i in range(max_len):
        byte1 = data1[i] if i < len(data1) else None
        byte2 = data2[i] if i < len(data2) else None

        if byte1 != byte2:
            diff_bits += 1
            print(f"Byte {i}: DIFFER")
            print(
                f"  Your output:    {byte1} (0x{byte1:02x} = {format(byte1, '08b')})" if byte1 is not None else "  Your output:    MISSING")
            print(
                f"  Expected:       {byte2} (0x{byte2:02x} = {format(byte2, '08b')})" if byte2 is not None else "  Expected:       MISSING")
            print()
        # else:
            # print(f"Byte {i}: MATCH - {byte1} (0x{byte1:02x} = {format(byte1, '08b')})")
    extra_bits = (max_len - min(len(data1), len(data2)))
    diff_bits -= extra_bits
    print(f"Total number of bits that differ: {diff_bits}")
    if extra_bits > 0: print(f"Extra bits that were ignored: {extra_bits}")
    print(f"Percentage of bits that differ: {diff_bits / max_len * 100}%")
    print()

# Run the comparison
if len(sys.argv) < 3:
    compare_binary_files("example.bin", "../assembler/example.bin")
else:
    compare_binary_files(sys.argv[1], sys.argv[2])
