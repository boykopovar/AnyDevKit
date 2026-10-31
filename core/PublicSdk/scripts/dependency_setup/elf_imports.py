import struct


def prepare_elf_import(source, destination):
    data = bytearray(source.read_bytes())
    if data[:6] != b"\x7fELF\x02\x01" or struct.unpack_from("<HH", data, 16) != (3, 62):
        raise ValueError(f"Expected x86-64 ELF shared library: {source}")
    section_offset = struct.unpack_from("<Q", data, 40)[0]
    section_size, count = struct.unpack_from("<HH", data, 58)
    if section_size != 64 or not count or section_offset + count * 64 > len(data):
        raise ValueError(f"Invalid ELF section table: {source}")
    sections = [list(struct.unpack_from("<IIQQQQIIQQ", data, section_offset + i * 64)) for i in range(count)]

    def contents(section):
        offset, size = section[4:6]
        if offset + size > len(data):
            raise ValueError(f"Truncated ELF section: {source}")
        return data[offset:offset + size]

    def replace(section, value):
        alignment = section[8] or 1
        data.extend(b"\0" * (-len(data) % alignment))
        section[4] = len(data)
        section[5] = len(value)
        data.extend(value)

    tables = [(i, s) for i, s in enumerate(sections) if s[1] == 11]
    if len(tables) != 1:
        raise ValueError(f"Expected one dynamic symbol table: {source}")
    index, symbols_section = tables[0]
    if symbols_section[9] != 24 or symbols_section[5] % 24 or symbols_section[6] >= count:
        raise ValueError(f"Invalid ELF dynamic symbols: {source}")
    strings_section = sections[symbols_section[6]]
    strings = contents(strings_section)
    symbols = contents(symbols_section)
    names = set()
    candidates = []
    for offset in range(0, len(symbols), 24):
        name, info, other, section, address, size = struct.unpack_from("<IBBHQQ", symbols, offset)
        end = strings.find(0, name)
        if name >= len(strings) or end < 0:
            raise ValueError(f"Invalid ELF symbol name: {source}")
        text = bytes(strings[name:end])
        if section:
            names.add(text)
        if section and info >> 4 in (1, 2) and other & 3 in (0, 3) and text.endswith(b"_nid_postfix"):
            candidates.append((text.removesuffix(b"_nid_postfix"), info, other, section, address, size))
    added = []
    for name, info, other, section, address, size in candidates:
        if name in names:
            continue
        if not name:
            raise ValueError(f"Empty ELF alias: {source}")
        symbols.extend(struct.pack("<IBBHQQ", len(strings), info, other, section, address, size))
        strings.extend(name + b"\0")
        names.add(name)
        added.append(name)
    if added:
        replace(symbols_section, symbols)
        replace(strings_section, strings)
        for section in sections:
            if section[6] == index and section[1] == 0x6FFFFFFF:
                versions = contents(section)
                versions.extend(struct.pack("<H", 1) * len(added))
                replace(section, versions)
        for i, section in enumerate(sections):
            struct.pack_into("<IIQQQQIIQQ", data, section_offset + i * 64, *section)
    destination.write_bytes(data)
