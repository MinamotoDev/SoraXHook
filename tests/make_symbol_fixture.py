"""Create a stripped ELF whose private ART-like symbol exists only in .gnu_debugdata."""
import lzma
from pathlib import Path
import struct
import sys

def elf(sections):
    names = b'\0.shstrtab\0'
    name_offsets = []
    for name, *_ in sections:
        name_offsets.append(len(names))
        names += name.encode() + b'\0'
    data = bytearray(64)
    headers = [bytes(64)]
    names_offset = len(data)
    data += names
    headers.append(struct.pack('<IIQQQQIIQQ', 1, 3, 0, 0, names_offset, len(names), 0, 0, 1, 0))
    for name_offset, (_, kind, payload, link, entsize) in zip(name_offsets, sections):
        data += b'\0' * (-len(data) % 8)
        offset = len(data)
        data += payload
        headers.append(struct.pack('<IIQQQQIIQQ', name_offset, kind, 0, 0, offset, len(payload), link, 0, 8, entsize))
    data += b'\0' * (-len(data) % 8)
    shoff = len(data)
    data += b''.join(headers)
    ident = b'\x7fELF\x02\x01\x01' + bytes(9)
    data[:64] = struct.pack('<16sHHIQQQIHHHHHH', ident, 3, 183, 1, 0, 0, shoff, 0, 64, 0, 0, 64, len(headers), 1)
    return data

def main():
    destination = Path(sys.argv[1])
    names = b'\0_ZN3art15GetMethodShortyFixture\0'
    symbols = bytes(24) + struct.pack('<IBBHQQ', 1, 2, 0, 1, 0x123456, 4)
    mini = elf([('.strtab', 3, names, 0, 0), ('.symtab', 2, symbols, 2, 24)])
    compressed = lzma.compress(mini, check=lzma.CHECK_CRC64)
    destination.write_bytes(elf([('.gnu_debugdata', 1, compressed, 0, 0)]))

if __name__ == '__main__':
    main()
