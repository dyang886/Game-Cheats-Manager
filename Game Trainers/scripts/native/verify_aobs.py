"""Check every AOB signature in a native trainer against the game's modules on disk.

    python -I scripts/native/verify_aobs.py "trainers/<Game> Trainer" "<game dir>/<Game>.exe" [<module> ...]

TrainerBase::findCode takes a signature's first match in the module's executable memory, so a
signature that matches nothing breaks its cheat and one that matches more than once may hook the
wrong place. This rebuilds each module's mapped image from its file and counts each signature in
all of it - stricter than findCode, which sees only the executable part - so it catches a game
update without launching anything. Pass every module the trainer's signatures live in (the exe and
any DLLs); a signature has to match exactly once across them. It only works for code that is not
packed on disk; scripts/native/recon.py says whether it is.

Signatures are the string literals in trainer.h that read as an AOB ("8B 40 ?? ..."), as
CodeSite::aob takes them; adjacent literals are joined, as C++ joins them.
Exits 1 when any signature does not match exactly once.
"""

import re
import struct
import sys
from pathlib import Path

HEX_TOKEN = r"(?:[0-9A-Fa-f]{2}|\?\?|\?)"
SPACED = re.compile(rf"^{HEX_TOKEN}(?: +{HEX_TOKEN})+$")
STRING_RUN = re.compile(r'"(?:[^"\\\n]|\\.)*"(?:\s*"(?:[^"\\\n]|\\.)*")*')
MIN_TOKENS = 4


def strip_comments(source):
    # Keeps offsets stable (comments become spaces) so line numbers stay right.
    def blank(match):
        return re.sub(r"[^\n]", " ", match.group(0))
    return re.sub(r'//[^\n]*|/\*.*?\*/|"(?:[^"\\\n]|\\.)*"', lambda m: m.group(0) if m.group(0).startswith('"') else blank(m), source, flags=re.S)


def signatures(source):
    found = []
    for match in STRING_RUN.finditer(source):
        text = "".join(re.findall(r'"((?:[^"\\\n]|\\.)*)"', match.group(0))).strip()
        if SPACED.match(text) and len(text.split()) >= MIN_TOKENS:
            found.append((source.count("\n", 0, match.start()) + 1, text.split()))
    return found


def mapped_image(exe):
    data = exe.read_bytes()
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    count = struct.unpack_from("<H", data, pe + 6)[0]
    opt_size = struct.unpack_from("<H", data, pe + 20)[0]
    opt = pe + 24
    size_of_image = struct.unpack_from("<I", data, opt + 56)[0]
    size_of_headers = struct.unpack_from("<I", data, opt + 60)[0]
    image = bytearray(size_of_image)
    image[:size_of_headers] = data[:size_of_headers]
    for i in range(count):
        s = opt + opt_size + 40 * i
        vsize, va, rsize, rptr = struct.unpack_from("<IIII", data, s + 8)
        chunk = data[rptr:rptr + min(rsize, vsize or rsize)]
        image[va:va + len(chunk)] = chunk
    return bytes(image)


def to_regex(tokens):
    parts = [b"." if t.startswith("?") else re.escape(bytes([int(t, 16)])) for t in tokens]
    return re.compile(b"".join(parts), re.S)


def main(argv):
    if len(argv) < 3:
        sys.exit(__doc__)
    trainer_h = Path(argv[1]) / "trainer.h"
    modules = [Path(p) for p in argv[2:]]
    source = strip_comments(trainer_h.read_text(encoding="utf-8"))
    sigs = signatures(source)
    if not sigs:
        sys.exit(f"No signatures found in {trainer_h}")

    images = [(module.name, mapped_image(module)) for module in modules]
    bad = 0
    for line, tokens in sigs:
        pattern = to_regex(tokens)
        # finditer skips overlapping matches; a second pass from each hit + 1 would find them, but a
        # signature that overlaps itself is not one a trainer should ship anyway.
        hits = [(name, m.start()) for name, image in images for m in pattern.finditer(image)]
        status = "ok" if len(hits) == 1 else ("MISSING" if not hits else f"AMBIGUOUS x{len(hits)}")
        bad += status != "ok"
        where = f"{hits[0][0]}+{hits[0][1]:X}" if hits else ""
        print(f"line {line:4}  {status:12} {where:28} {' '.join(tokens[:12])}{' ...' if len(tokens) > 12 else ''}")
    print(f"{len(sigs) - bad}/{len(sigs)} signatures match exactly once in {', '.join(m.name for m in modules)}")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
