// CaveBuilder.h
// Assembles the code a TrainerBase::hookCode cave runs. The cave's address, the hooked instruction's
// address and the bytes the hook's jump replaced are all known when the builder is made, so jumps
// back into the game, relocated branches and references to data inside the cave are encoded here
// rather than by hand. build() resolves labels and throws on one that is never bound; the throw
// reaches the trainer's alert verbatim.
//
//   hookCode("SetBolts", CodeSite::aob(moduleName, "8B 40 38 48 83 C4 28 C3"), 7,
//            [value](CaveBuilder &cave)
//            {
//                cave.bytes({0xC7, 0x40, 0x38}).value(value) // mov dword ptr [rax+38],value
//                    .original()                             // the 7 bytes the jump replaced
//                    .jmpBack();
//            });
//
// Data the cave reads sits after its code, behind a label. On x64 an instruction reaches it
// RIP-relative; x86 has no RIP-relative addressing, so there the instruction carries its absolute
// address:
//
//   cave.ripRelative({0xF3, 0x0F, 0x59, 0x05}, "mult")  // mulss xmm0,[mult]      (x64)
//   cave.bytes({0xD9, 0x05}).address("mult")            // fld dword ptr [mult]   (x86)
//       ...
//       .jmpBack()
//       .label("mult").value(2.0f);
//
// Branches inside the cave are always near (rel32), so a label is never out of range.
#pragma once

#include <Windows.h>
#include <cstdint>
#include <cstring>
#include <map>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

class CaveBuilder
{
public:
    /// Branch conditions for jumpTo(). Each is the x86 condition of the same name.
    enum class Jcc
    {
        Always,
        O, NO, B, AE, E, NE, BE, A, S, NS, P, NP, L, GE, LE, G
    };

    /// Registers vtableCheck() can compare through. The values are ModRM r/m codes, so each 64-bit
    /// name and its 32-bit name are the same register. Not rax/eax, which the check loads the
    /// vtable into, and not rsp/rbp, which would need a SIB byte or a displacement.
    enum Register : BYTE
    {
        RCX = 1, RDX = 2, RBX = 3, RSI = 6, RDI = 7,
        ECX = 1, EDX = 2, EBX = 3, ESI = 6, EDI = 7
    };

    CaveBuilder(uintptr_t caveAddress, uintptr_t hookAddress, std::vector<BYTE> originalBytes)
        : cave(caveAddress), hook(hookAddress), overwritten(std::move(originalBytes)) {}

    uintptr_t caveAddress() const { return cave; }
    uintptr_t hookAddress() const { return hook; }
    /// The instruction after the overwritten bytes, where jmpBack() lands.
    uintptr_t returnAddress() const { return hook + overwritten.size(); }
    const std::vector<BYTE> &originalBytes() const { return overwritten; }

    /// Where a call or jmp rel32 among the overwritten bytes goes, for re-encoding it in the cave:
    /// its displacement starts rel32Offset bytes in, and the instruction ends right after it.
    uintptr_t originalBranchTarget(size_t rel32Offset) const
    {
        if (rel32Offset + 4 > overwritten.size())
            throw std::out_of_range("CaveBuilder: no rel32 at offset " + std::to_string(rel32Offset) + " of the overwritten bytes");
        int32_t rel = 0;
        std::memcpy(&rel, &overwritten[rel32Offset], 4);
        return hook + rel32Offset + 4 + static_cast<intptr_t>(rel);
    }

    CaveBuilder &bytes(std::initializer_list<BYTE> b)
    {
        code.insert(code.end(), b);
        return *this;
    }

    CaveBuilder &bytes(const std::vector<BYTE> &b)
    {
        code.insert(code.end(), b.begin(), b.end());
        return *this;
    }

    /// v's bytes, little-endian - an immediate operand, or data the cave reads.
    template <typename T>
    CaveBuilder &value(const T &v)
    {
        static_assert(std::is_trivially_copyable_v<T>, "CaveBuilder::value needs a plain value");
        const BYTE *raw = reinterpret_cast<const BYTE *>(&v);
        code.insert(code.end(), raw, raw + sizeof(T));
        return *this;
    }

    /// Every overwritten byte, unchanged. Only right for instructions that do not address relative
    /// to themselves - a rel32 branch among them needs originalBranchTarget() and jmp()/call().
    CaveBuilder &original()
    {
        return bytes(overwritten);
    }

    /// Part of the overwritten bytes, for when the cave replaces the rest.
    CaveBuilder &original(size_t offset, size_t length)
    {
        if (offset + length > overwritten.size())
            throw std::out_of_range("CaveBuilder: original(" + std::to_string(offset) + ", " + std::to_string(length) +
                                    ") runs past the " + std::to_string(overwritten.size()) + " overwritten bytes");
        code.insert(code.end(), overwritten.begin() + offset, overwritten.begin() + offset + length);
        return *this;
    }

    /// jmp to an absolute address.
    CaveBuilder &jmp(uintptr_t target)
    {
        code.push_back(0xE9);
        return externalRel32(target);
    }

    /// call an absolute address.
    CaveBuilder &call(uintptr_t target)
    {
        code.push_back(0xE8);
        return externalRel32(target);
    }

    /// jmp to the instruction after the overwritten bytes.
    CaveBuilder &jmpBack()
    {
        return jmp(returnAddress());
    }

    CaveBuilder &label(const std::string &name)
    {
        if (!labelOffsets.emplace(name, code.size()).second)
            throw std::logic_error("CaveBuilder: label '" + name + "' is bound twice");
        return *this;
    }

    /// Branch to a label bound before or after this point.
    CaveBuilder &jumpTo(Jcc condition, const std::string &labelName)
    {
        if (condition == Jcc::Always)
            code.push_back(0xE9);
        else
            bytes({0x0F, static_cast<BYTE>(0x80 + static_cast<int>(condition) - 1)});
        fixups.push_back({code.size(), labelName, Fixup::Relative});
        return value<int32_t>(0);
    }

    /// A label's absolute address, pointer-sized: x86's [disp32] operand, or x64's mov r64, imm64.
    CaveBuilder &address(const std::string &labelName)
    {
        fixups.push_back({code.size(), labelName, Fixup::Absolute});
        return value<uintptr_t>(0);
    }

#if defined(_WIN64)
    /// An instruction whose last four bytes are a RIP-relative displacement to a label. Nothing
    /// may follow the displacement - an instruction with an immediate after it is out.
    CaveBuilder &ripRelative(std::initializer_list<BYTE> opcode, const std::string &labelName)
    {
        return ripRelative(std::vector<BYTE>(opcode), labelName);
    }

    CaveBuilder &ripRelative(const std::vector<BYTE> &opcode, const std::string &labelName)
    {
        bytes(opcode);
        fixups.push_back({code.size(), labelName, Fixup::Relative});
        return value<int32_t>(0);
    }
#endif

    /// Sets ZF when the object [object] points at has this vtable, and leaves every register as it
    /// was: push rax / mov rax, vtable / cmp [object], rax / pop rax. Follow it with
    /// jumpTo(Jcc::NE, ...) - the usual guard where a function is shared with other classes.
    CaveBuilder &vtableCheck(Register object, uintptr_t vtable)
    {
#if defined(_WIN64)
        bytes({0x50, 0x48, 0xB8}).value<uint64_t>(vtable);
        return bytes({0x48, 0x39, static_cast<BYTE>(object), 0x58});
#else
        bytes({0x50, 0xB8}).value<uint32_t>(static_cast<uint32_t>(vtable));
        return bytes({0x39, static_cast<BYTE>(object), 0x58});
#endif
    }

    /// The finished bytes, every label reference resolved.
    std::vector<BYTE> build() const
    {
        std::vector<BYTE> out = code;
        for (const Fixup &f : fixups)
        {
            uintptr_t target = cave + labelOffset(f.label);
            if (f.kind == Fixup::Absolute)
            {
                std::memcpy(&out[f.at], &target, sizeof(target));
                continue;
            }
            int32_t rel = static_cast<int32_t>(target - (cave + f.at + 4)); // both ends are in the cave
            std::memcpy(&out[f.at], &rel, 4);
        }
        return out;
    }

    /// Where a label sits from the start of the cave.
    size_t labelOffset(const std::string &name) const
    {
        auto it = labelOffsets.find(name);
        if (it == labelOffsets.end())
            throw std::logic_error("CaveBuilder: label '" + name + "' is never bound");
        return it->second;
    }

    const std::map<std::string, size_t> &labels() const { return labelOffsets; }

private:
    struct Fixup
    {
        size_t at;
        std::string label;
        enum Kind { Relative, Absolute } kind;
    };

    uintptr_t cave;
    uintptr_t hook;
    std::vector<BYTE> overwritten;
    std::vector<BYTE> code;
    std::map<std::string, size_t> labelOffsets;
    std::vector<Fixup> fixups;

    // On x86 a rel32 reaches the whole address space by wrapping; on x64 the cave has to be within
    // 2 GB of the target, which TrainerBase allocates it to be.
    CaveBuilder &externalRel32(uintptr_t target)
    {
        uintptr_t diff = target - (cave + code.size() + 4);
#if defined(_WIN64)
        int64_t rel = static_cast<int64_t>(diff);
        if (rel < INT32_MIN || rel > INT32_MAX)
            throw std::out_of_range("CaveBuilder: the cave is too far from its jump target for a rel32");
#endif
        return value(static_cast<int32_t>(diff));
    }
};
