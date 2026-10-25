"""Static recon of a native game executable (see docs/native_trainer_discovery.md).

Answers the go/no-go questions before any live work: architecture, whether the code on disk is
the code that runs (packer / protector hints, .text entropy), whether a PDB is referenced, and -
for MSVC binaries, x86 or x64 - the RTTI class list with each class's vtables. The RTTI map is
what lets the live phase name an object from its first pointer, and find every instance of a class
by scanning for its vtable pointer.

    python -I scripts/native/recon.py "<game dir>/<Game>.exe" [--out <dir>] [--grep word,word]

Pass a DLL instead when the game's code lives in one. Writes recon.json to <game dir>/gcm_dump/
unless --out is given, and prints a summary.
"""

import argparse
import collections
import json
import math
import re
import struct
import sys
from pathlib import Path

PROTECTOR_SECTIONS = {
    ".bind": "Steam DRM wrapper",
    ".vmp0": "VMProtect", ".vmp1": "VMProtect",
    ".themida": "Themida", ".winlice": "WinLicense",
    ".enigma1": "Enigma Protector", ".enigma2": "Enigma Protector",
    ".arxan": "Arxan", ".ooa": "EA/Origin wrapper",
    "UPX0": "UPX", "UPX1": "UPX", ".aspack": "ASPack", ".petite": "Petite",
}
PROTECTOR_STRINGS = [b"Denuvo", b"VMProtect", b"Themida", b"steam_api64.dll", b"steam_api.dll", b"EOSSDK"]


def entropy(data):
    if not data:
        return 0.0
    stride = max(1, len(data) // 4_000_000)
    counts = collections.Counter(data[::stride])
    total = sum(counts.values())
    return -sum(c / total * math.log2(c / total) for c in counts.values())


def demangle_class(name):
    # ".?AVInner@Outer@@" -> "Outer::Inner"; templates (?$) are left as written.
    body = name[4:-2] if name.endswith("@@") else name[4:]
    if "?$" in body:
        return body
    return "::".join(reversed([p for p in body.split("@") if p]))


class PE:
    def __init__(self, data):
        self.data = data
        pe = struct.unpack_from("<I", data, 0x3C)[0]
        if data[pe:pe + 4] != b"PE\0\0":
            raise ValueError("not a PE file")
        self.machine = struct.unpack_from("<H", data, pe + 4)[0]
        count = struct.unpack_from("<H", data, pe + 6)[0]
        opt_size = struct.unpack_from("<H", data, pe + 20)[0]
        opt = pe + 24
        self.is64 = struct.unpack_from("<H", data, opt)[0] == 0x20B
        self.image_base = struct.unpack_from("<Q", data, opt + 24)[0] if self.is64 else struct.unpack_from("<I", data, opt + 28)[0]
        self.image_size = struct.unpack_from("<I", data, opt + 56)[0]
        dirs = opt + (112 if self.is64 else 96)
        self.debug_dir = struct.unpack_from("<II", data, dirs + 6 * 8)
        self.sections = []
        for i in range(count):
            s = opt + opt_size + 40 * i
            name = data[s:s + 8].rstrip(b"\0").decode("latin1")
            vsize, va, rsize, rptr = struct.unpack_from("<IIII", data, s + 8)
            chars = struct.unpack_from("<I", data, s + 36)[0]
            self.sections.append({"name": name, "va": va, "vsize": vsize, "rptr": rptr, "rsize": rsize, "chars": chars})

    def section_bytes(self, sec):
        return self.data[sec["rptr"]:sec["rptr"] + min(sec["rsize"], sec["vsize"] or sec["rsize"])]

    def rva_to_off(self, rva):
        for s in self.sections:
            if s["va"] <= rva < s["va"] + max(s["vsize"], s["rsize"]):
                return s["rptr"] + rva - s["va"] if rva - s["va"] < s["rsize"] else None
        return None

    def pdb_path(self):
        rva, size = self.debug_dir
        off = self.rva_to_off(rva) if rva else None
        if off is None:
            return None
        for i in range(size // 28):
            entry = off + 28 * i
            kind = struct.unpack_from("<I", self.data, entry + 12)[0]
            ptr = struct.unpack_from("<I", self.data, entry + 24)[0]
            if kind == 2 and self.data[ptr:ptr + 4] == b"RSDS":
                return self.data[ptr + 24:self.data.index(b"\0", ptr + 24)].decode("utf-8", "replace")
        return None


def recover_rtti(pe):
    """MSVC RTTI. A vtable is preceded by a pointer to its CompleteObjectLocator {signature, offset,
    cdOffset, pTypeDescriptor, pClassDescriptor[, pSelf]} in a read-only section, and the
    TypeDescriptor holds the mangled class name after its vtable pointer and spare field. x64
    locators (signature 1) hold RVAs and pSelf, their own RVA; x86 ones (signature 0) hold absolute
    addresses, so they are recognised by pointing at a TypeDescriptor."""
    readonly = [s for s in pe.sections if s["chars"] & 0x40000000 and not s["chars"] & 0x80000000 and not s["chars"] & 0x20000000]
    pointer = 8 if pe.is64 else 4
    names = {}

    def type_name(td_rva):
        if td_rva not in names:
            off = pe.rva_to_off(td_rva + 2 * pointer)
            end = pe.data.find(b"\0", off, off + 4096) if off is not None else -1
            name = pe.data[off:end].decode("latin1") if end > 0 else ""
            names[td_rva] = name if name.startswith(".?A") else None
        return names[td_rva]

    image_end = pe.image_base + pe.image_size
    cols = {}
    for sec in readonly:
        raw = pe.section_bytes(sec)
        words = memoryview(raw[:len(raw) // 4 * 4]).cast("I")
        base = sec["va"]
        for i in range(len(words) - 5):
            if pe.is64:
                if words[i] != 1 or words[i + 5] != base + 4 * i:
                    continue
                td_rva = words[i + 3]
            else:
                if words[i] != 0 or not pe.image_base <= words[i + 3] < image_end or not pe.image_base <= words[i + 4] < image_end:
                    continue
                td_rva = words[i + 3] - pe.image_base
            name = type_name(td_rva)
            if name:
                cols[pe.image_base + base + 4 * i] = (name, words[i + 1])

    classes = collections.defaultdict(list)
    for sec in readonly:
        raw = pe.section_bytes(sec)
        slots = memoryview(raw[:len(raw) // pointer * pointer]).cast("Q" if pe.is64 else "I")
        for i, value in enumerate(slots):
            hit = cols.get(value)
            if hit:
                name, this_offset = hit
                classes[name].append({"vtableRva": sec["va"] + pointer * i + pointer, "thisOffset": this_offset})
    return [{"name": demangle_class(n), "mangled": n, "vtables": sorted(v, key=lambda x: x["thisOffset"])}
            for n, v in sorted(classes.items())]


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("exe", type=Path)
    parser.add_argument("--out", type=Path)
    parser.add_argument("--grep", default="", help="comma-separated words; print RTTI classes containing any")
    args = parser.parse_args()

    data = args.exe.read_bytes()
    pe = PE(data)
    arch = {0x8664: "x64", 0x14C: "x86", 0xAA64: "arm64"}.get(pe.machine, hex(pe.machine))

    sections = []
    for s in pe.sections:
        sections.append({**{k: s[k] for k in ("name", "va", "vsize", "rsize")},
                         "characteristics": hex(s["chars"]),
                         "entropy": round(entropy(pe.section_bytes(s)), 2)})
    code = [s for s in sections if int(s["characteristics"], 16) & 0x20000000]
    hints = sorted({PROTECTOR_SECTIONS[s["name"]] for s in sections if s["name"] in PROTECTOR_SECTIONS})
    strings = {k.decode(): data.count(k) for k in PROTECTOR_STRINGS if data.count(k)}
    packed = any(s["entropy"] > 7.2 for s in code) or any(s["rsize"] == 0 and s["vsize"] > 0 for s in code)

    rtti = recover_rtti(pe)
    report = {
        "exe": str(args.exe),
        "arch": arch,
        "imageBase": hex(pe.image_base),
        "pdb": pe.pdb_path(),
        "codeLooksPacked": packed,
        "protectorSections": hints,
        "markerStrings": strings,
        "sections": sections,
        "rttiClassCount": len(rtti),
        "rtti": rtti,
    }

    out_dir = args.out or args.exe.parent / "gcm_dump"
    out_dir.mkdir(parents=True, exist_ok=True)
    (out_dir / "recon.json").write_text(json.dumps(report, indent=1), encoding="utf-8")

    print(f"{args.exe.name}: {arch}, image base {hex(pe.image_base)}, PDB {report['pdb'] or 'none'}")
    for s in sections:
        print(f"  {s['name']:10} va={s['va']:#010x} vsize={s['vsize']:#010x} entropy={s['entropy']:.2f} {s['characteristics']}")
    print(f"code looks packed: {packed}; protector sections: {hints or 'none'}; marker strings: {strings or 'none'}")
    print(f"RTTI classes with vtables: {len(rtti)} -> {out_dir / 'recon.json'}")
    for word in [w.strip().lower() for w in args.grep.split(",") if w.strip()]:
        hits = [c for c in rtti if word in c["name"].lower()]
        print(f"[{word}] {len(hits)}")
        for c in hits[:60]:
            print(f"  {c['name']:60} " + " ".join(f"{v['vtableRva']:#x}(+{v['thisOffset']:#x})" for v in c["vtables"][:3]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
