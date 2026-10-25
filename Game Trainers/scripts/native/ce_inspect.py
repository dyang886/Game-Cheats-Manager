"""RTTI-aware inspection of the attached game through Cheat Engine (see docs/native_trainer_discovery.md).

Reads the class map scripts/native/recon.py wrote (gcm_dump/recon.json beside the game exe) and
answers the questions the discovery loop keeps asking:

    python scripts/native/ce_inspect.py <recon.json> instances <Class> [<Class> ...]
        every live object of a class, found by scanning writable memory for its vtable pointer
    python scripts/native/ce_inspect.py <recon.json> dump <hex-address> <hex-size>
        pointer-sized slots as hex / int / float, naming vtables and code by module+offset and
        marking other readable addresses as pointers
    python scripts/native/ce_inspect.py <recon.json> vtables <Class> [<Class> ...]
        the classes' vtables side by side; '*' marks slots that differ (overrides)
    python scripts/native/ce_inspect.py <recon.json> disasm <hex-rva-start> <hex-rva-end> <out.asm>
        a module range disassembled to a file, one "module+RVA  bytes  text" line per instruction

The module is the file recon.json was made from - the exe, or a DLL that holds the game's code -
found by name in the attached process. x86 and x64 both work; the pointer size comes from recon.json.
"""

import json
import struct
import sys
from pathlib import Path, PureWindowsPath

sys.path.insert(0, str(Path(__file__).resolve().parent))
import ce_mcp  # noqa: E402


class Recon:
    def __init__(self, path):
        recon = json.loads(Path(path).read_text(encoding="utf-8"))
        self.module = PureWindowsPath(recon["exe"]).name
        self.pointer_size = 8 if recon["arch"] == "x64" else 4
        self.value_type = "uint64" if self.pointer_size == 8 else "uint32"
        self.by_name = {c["name"]: c for c in recon["rtti"]}
        self.by_vtable = {v["vtableRva"]: c["name"] for c in recon["rtti"] for v in c["vtables"]}

    def vtable_rva(self, name):
        cls = self.by_name.get(name)
        if cls is None:
            raise RuntimeError(f"{name} is not in the RTTI map")
        return cls["vtables"][0]["vtableRva"]


def module_range(gateway, recon):
    listing = gateway.call("module_list", {"nameContains": recon.module, "limit": 1000})
    for module in listing.get("modules", []):
        if module["name"].lower() == recon.module.lower():
            return int(module["base"], 16), int(module.get("size") or 0)
    raise RuntimeError(f"{recon.module} is not loaded in the attached process")


def instances(gateway, recon, names):
    base, _ = module_range(gateway, recon)
    for name in names:
        rva = recon.vtable_rva(name)
        result = gateway.call("aob_find_value", {"valueType": recon.value_type, "value": str(base + rva),
                                                 "writable": "required", "alignment": recon.pointer_size, "limit": 64})
        matches = result["result"]["matches"]
        print(f"{name} (vtable {recon.module}+{rva:X}): {len(matches)} {' '.join(matches)}")


def describe(gateway, values):
    """module+offset for each value inside a module, 'ptr' for another readable address, else ''."""
    listed = ", ".join(f"0x{v:X}" for v in values)
    source = f"""
local out = {{}}
for i, v in ipairs({{{listed}}}) do
  if inModule(v) then out[i] = getNameFromAddress(v, true, false)
  elseif v >= 0x10000 and readBytes(v, 1) ~= nil then out[i] = 'ptr'
  else out[i] = '' end
end
return table.concat(out, '\\n')
"""
    return gateway.lua(source).split("\n")


def dump(gateway, recon, address, size):
    base, msize = module_range(gateway, recon)
    raw = bytes.fromhex(gateway.call("memory_read", {"address": f"{address:X}", "valueType": "bytes", "size": size})["value"].replace(" ", ""))
    step = recon.pointer_size
    slots = [int.from_bytes(raw[off:off + step], "little") for off in range(0, len(raw) - step + 1, step)]
    notes = describe(gateway, slots)

    def as_float(f):
        return f"{f:.3f}" if 1e-4 < abs(f) < 1e7 else "-"

    for i, value in enumerate(slots):
        chunk = raw[i * step:(i + 1) * step]
        ints = struct.unpack(f"<{step // 4}i", chunk)
        floats = struct.unpack(f"<{step // 4}f", chunk)
        note = notes[i] if i < len(notes) else ""
        if base <= value < base + msize and value - base in recon.by_vtable:
            note += f" {recon.by_vtable[value - base]}"
        print(f"+{i * step:03X} {value:0{step * 2}X} | {' '.join(f'{n:>11}' for n in ints)} | "
              f"{' '.join(f'{as_float(f):>10}' for f in floats)} | {note}")


def vtables(gateway, recon, names):
    base, msize = module_range(gateway, recon)
    tables = {}
    for name in names:
        vt = base + recon.vtable_rva(name)
        values = [int(v) for v in gateway.call("memory_read", {"address": f"{vt:X}", "valueType": recon.value_type, "count": 160})["values"]]
        slots = []
        for v in values:
            if not base < v < base + msize:
                break
            slots.append(v - base)
        tables[name] = slots
    width = max(len(n) for n in names) + 2
    print(f"{recon.module} RVAs")
    print("slot " + "".join(f"{n[-width + 1:]:>{width}}" for n in names))
    for i in range(max(len(t) for t in tables.values())):
        row = [tables[n][i] if i < len(tables[n]) else None for n in names]
        cells = "".join(f"{('%X' % r) if r is not None else '':>{width}}" for r in row)
        print(f"{i * recon.pointer_size:4X} {cells}{'  *' if len({r for r in row if r is not None}) > 1 else ''}")


def disasm(gateway, recon, start_rva, end_rva, out):
    base, _ = module_range(gateway, recon)
    address, end = base + start_rva, base + end_rva
    lines = []
    while address < end:
        instructions = gateway.call("code_disassemble", {"address": f"{address:X}", "count": 1000})["instructions"]
        for ins in instructions:
            at = int(ins["address"], 16)
            if at >= end:
                break
            lines.append(f"{recon.module}+{at - base:X}  {ins.get('bytes', ''):<26} {ins['text']}")
        last = instructions[-1]
        address = int(last["address"], 16) + max(1, len(last.get("bytes", "").replace(" ", "")) // 2)
    Path(out).write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"{len(lines)} instructions -> {out}")


def main(argv):
    if len(argv) < 3:
        sys.exit(__doc__)
    recon = Recon(argv[1])
    command, rest = argv[2], argv[3:]
    gateway = ce_mcp.Gateway()
    try:
        if command == "instances":
            instances(gateway, recon, rest)
        elif command == "dump":
            dump(gateway, recon, int(rest[0], 16), int(rest[1], 16))
        elif command == "vtables":
            vtables(gateway, recon, rest)
        elif command == "disasm":
            disasm(gateway, recon, int(rest[0], 16), int(rest[1], 16), rest[2])
        else:
            sys.exit(__doc__)
    except RuntimeError as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 1
    finally:
        gateway.close()
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
