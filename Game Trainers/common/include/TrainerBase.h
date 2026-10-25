// TrainerBase.h
// The base for trainers that change the game from outside it (RUNTIME none), and the process layer
// the Mono, IL2CPP and UE bases build on.
//
// Every cheat changes the game through a named modification, and disable(name) undoes any of them:
//
//   patchCode(name, site, bytes)                 overwrite instructions in place
//   hookCode(name, site, overwriteLen, body)     jump to a code cave the body assembles (CaveBuilder.h)
//   holdValue(name, chain, value)                keep the value at a pointer chain's end at `value`
//   freezeValue<T>(name, chain)                  keep it at what it was when the cheat was enabled
//   scaleValues(name, chain, fields, factor)     keep floats at original x factor, restored on disable
//
// Code is found by a CodeSite - an AOB as Cheat Engine prints it, or an offset into a module - and
// data through a PointerChain. readValue / writeValue are one-off reads and writes through a chain,
// writeCaveData changes a value a live cave reads. Each failure writes its reason to std::cerr,
// which TrainerApp shows under "Failed to activate."; see docs/native_trainer_discovery.md for how
// sites and chains are found.
#pragma once

#include <Windows.h>
#include <TlHelp32.h>
#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cstring>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

#include "CaveBuilder.h"

/// Where a patch or hook goes.
///
/// A module name starting with "$re$" is a regular expression over module names, for games that
/// load their code under a generated name.
struct CodeSite
{
    /// The first match of an AOB, written as Cheat Engine prints it ("8B 40 ?? C3"); offset is how
    /// far into the match the instruction starts. A null module searches all executable memory,
    /// which is where JIT-compiled code lives.
    static CodeSite aob(const wchar_t *module, std::string signature, size_t offset = 0)
    {
        return CodeSite{module, std::move(signature), offset, 0};
    }

    /// A fixed offset into a module - for code no AOB pins down uniquely.
    static CodeSite rva(const wchar_t *module, uintptr_t rva)
    {
        return CodeSite{module, "", 0, rva};
    }

    const wchar_t *module = nullptr;
    std::string signature;
    size_t offset = 0;
    uintptr_t moduleOffset = 0; // used when signature is empty
};

/// [[module + offsets[0]] + offsets[1]] ... + offsets[n]. Each offset after the first is added to
/// the pointer read at the address before it, so a single offset is a static address in the module.
struct PointerChain
{
    const wchar_t *module = nullptr;
    std::vector<uintptr_t> offsets;
};

class TrainerBase
{
public:
    /// processIdentifier is the game's exe name, or its window title when useWindowTitle is set.
    TrainerBase(const std::wstring &processIdentifier, bool useWindowTitle = false)
        : processName(processIdentifier), useWindowTitle(useWindowTitle)
    {
        prepareConsole();
    }

    virtual ~TrainerBase()
    {
        cleanUp();
    }

    std::wstring getProcessName() const { return processName; }
    DWORD getProcessId() const { return procId; }

    /// Finds the game and opens it. A game that has exited or restarted drops whatever the trainer
    /// had changed in it.
    bool isProcessRunning()
    {
        DWORD pid = findProcessId();
        if (pid == 0)
        {
            std::wcerr << L"[!] Could not find process: " << processName << std::endl;
            cleanUp();
            procId = 0;
            return false;
        }

        if (hProcess && pid != procId)
            cleanUp();
        procId = pid;

        if (!hProcess)
        {
            hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procId);
            if (!hProcess)
            {
                std::cerr << "[!] Failed to open process. Error: " << GetLastError() << std::endl;
                cleanUp();
                return false;
            }
        }
        return true;
    }

    /// Undoes every modification and closes the process handle.
    virtual void cleanUp()
    {
        disableAll();
        if (hProcess)
            CloseHandle(hProcess);
        hProcess = nullptr;
    }

    /// Undoes the modification made under name: restores overwritten code and frees its cave, or
    /// stops a held value. False, with the reason on stderr, when nothing is active under that name.
    bool disable(const std::string &name)
    {
        auto it = modifications.find(name);
        if (it == modifications.end())
        {
            std::cerr << "[!] '" << name << "' is not active.\n";
            return false;
        }
        undo(it->second);
        modifications.erase(it);
        std::cout << "[-] Disabled '" << name << "'\n";
        return true;
    }

    void disableAll()
    {
        for (auto &[name, modification] : modifications)
            undo(modification);
        modifications.clear();
    }

    bool isActive(const std::string &name) const
    {
        return modifications.count(name) != 0;
    }

protected:
    const std::wstring processName;
    const bool useWindowTitle;
    HANDLE hProcess = nullptr;
    DWORD procId = 0;

    // ------------------------------------------------------------------------------------------------
    // Modules
    // ------------------------------------------------------------------------------------------------

    /// A "$re$" prefix makes the name a regular expression over module names.
    bool getModuleInfo(const wchar_t *module, uintptr_t &base, size_t &size)
    {
        if (!module)
            return false;
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procId);
        if (snap == INVALID_HANDLE_VALUE)
        {
            std::cerr << "[!] Failed to create module snapshot. Error: " << GetLastError() << std::endl;
            return false;
        }

        static const std::wstring regexPrefix = L"$re$";
        const std::wstring name(module);
        const bool isRegex = name.rfind(regexPrefix, 0) == 0;
        const std::wregex pattern(isRegex ? name.substr(regexPrefix.size()) : L"");

        MODULEENTRY32W me{};
        me.dwSize = sizeof(me);
        bool found = false;
        for (BOOL more = Module32FirstW(snap, &me); more && !found; more = Module32NextW(snap, &me))
        {
            found = isRegex ? std::regex_match(me.szModule, pattern) : _wcsicmp(me.szModule, module) == 0;
            if (found)
            {
                base = reinterpret_cast<uintptr_t>(me.modBaseAddr);
                size = me.modBaseSize;
            }
        }
        CloseHandle(snap);
        return found;
    }

    /// The module's base plus rva - an absolute address a cave compares against, such as a vtable.
    /// 0, with the reason on stderr, when the module is not loaded.
    uintptr_t moduleAddress(const wchar_t *module, uintptr_t rva)
    {
        uintptr_t base = 0;
        size_t size = 0;
        if (!getModuleInfo(module, base, size))
        {
            std::wcerr << L"[!] Module not loaded: " << (module ? module : L"(none)") << std::endl;
            return 0;
        }
        return base + rva;
    }

    // ------------------------------------------------------------------------------------------------
    // Memory
    // ------------------------------------------------------------------------------------------------

    bool readBytes(uintptr_t address, void *buffer, size_t size)
    {
        SIZE_T read = 0;
        return ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(address), buffer, size, &read) && read == size;
    }

    bool writeBytes(uintptr_t address, const void *buffer, size_t size)
    {
        SIZE_T written = 0;
        return WriteProcessMemory(hProcess, reinterpret_cast<LPVOID>(address), buffer, size, &written) && written == size;
    }

    template <typename T>
    bool readMemory(uintptr_t address, T &out)
    {
        static_assert(std::is_trivially_copyable_v<T>, "readMemory needs a plain value type");
        return readBytes(address, &out, sizeof(T));
    }

    template <typename T>
    bool writeMemory(uintptr_t address, const T &value)
    {
        static_assert(std::is_trivially_copyable_v<T>, "writeMemory needs a plain value type");
        return writeBytes(address, &value, sizeof(T));
    }

    /// The address a chain ends at, or 0 with the reason on stderr when the module is not loaded or a
    /// link reads as unreadable or null.
    uintptr_t resolve(const PointerChain &chain)
    {
        uintptr_t base = moduleAddress(chain.module, 0);
        return base ? followChain(base, chain.offsets, true) : 0;
    }

    template <typename T>
    bool readValue(const PointerChain &chain, T &out)
    {
        uintptr_t address = resolve(chain);
        if (address == 0)
            return false;
        if (!readMemory(address, out))
        {
            std::cerr << "[!] Could not read 0x" << std::hex << address << std::dec << ".\n";
            return false;
        }
        return true;
    }

    template <typename T>
    bool writeValue(const PointerChain &chain, const T &value)
    {
        uintptr_t address = resolve(chain);
        if (address == 0)
            return false;
        if (!writeMemory(address, value))
        {
            std::cerr << "[!] Could not write 0x" << std::hex << address << std::dec << ".\n";
            return false;
        }
        return true;
    }

    // ------------------------------------------------------------------------------------------------
    // Code
    // ------------------------------------------------------------------------------------------------

    /// The address a site names, or 0 with the reason on stderr. An AOB is matched against executable
    /// memory only - inside its module, or anywhere for a null module - and the first match wins;
    /// scripts/native/verify_aobs.py checks offline that a signature matches exactly once.
    uintptr_t findCode(const CodeSite &site)
    {
        if (site.signature.empty())
            return moduleAddress(site.module, site.moduleOffset);

        const std::vector<SignatureByte> signature = parseSignature(site.signature);

        uintptr_t start = 0, end = UINTPTR_MAX;
        if (site.module)
        {
            size_t size = 0;
            if (!getModuleInfo(site.module, start, size))
            {
                std::wcerr << L"[!] Module not loaded: " << site.module << std::endl;
                return 0;
            }
            end = start + size;
        }

        uintptr_t match = scanExecutable(start, end, signature);
        if (match == 0)
        {
            std::cerr << "[!] Signature not found: " << site.signature.substr(0, 47) << (site.signature.size() > 47 ? " ..." : "") << "\n";
            return 0;
        }
        return match + site.offset;
    }

    /// Overwrites the instructions at site with bytes.
    bool patchCode(const std::string &name, const CodeSite &site, const std::vector<BYTE> &bytes)
    {
        if (!claimName(name))
            return false;
        uintptr_t address = findCode(site);
        if (address == 0)
            return false;

        Modification patch;
        patch.address = address;
        patch.originalBytes.resize(bytes.size());
        if (!readBytes(address, patch.originalBytes.data(), bytes.size()) || !writeCode(address, bytes))
        {
            std::cerr << "[!] Could not patch '" << name << "' at 0x" << std::hex << address << std::dec << ".\n";
            return false;
        }

        modifications[name] = std::move(patch);
        std::cout << "[+] Patched '" << name << "' at 0x" << std::hex << address << std::dec << "\n";
        return true;
    }

    /// Replaces the first overwriteLen bytes at site with a jump to a code cave, whose code body
    /// assembles. overwriteLen covers whole instructions and is at least the jump's five bytes; what
    /// is left over is filled with NOPs. The cave is allocated within rel32 reach of site.
    bool hookCode(const std::string &name, const CodeSite &site, size_t overwriteLen, const std::function<void(CaveBuilder &)> &body)
    {
        if (overwriteLen < 5)
        {
            std::cerr << "[!] '" << name << "' overwrites " << overwriteLen << " bytes; a hook's jump needs 5.\n";
            return false;
        }
        if (!claimName(name))
            return false;
        uintptr_t address = findCode(site);
        if (address == 0)
            return false;

        Modification hook;
        hook.address = address;
        hook.originalBytes.resize(overwriteLen);
        if (!readBytes(address, hook.originalBytes.data(), overwriteLen))
        {
            std::cerr << "[!] Could not read the code '" << name << "' overwrites.\n";
            return false;
        }

        hook.cave = allocateCave(address);
        if (!hook.cave)
        {
            std::cerr << "[!] No free memory within reach of '" << name << "' for its cave.\n";
            return false;
        }

        // The cave is freed if building it throws, and the throw goes on to the alert.
        const uintptr_t caveAddress = reinterpret_cast<uintptr_t>(hook.cave);
        std::vector<BYTE> code;
        try
        {
            CaveBuilder cave(caveAddress, address, hook.originalBytes);
            body(cave);
            code = cave.build();
            hook.caveLabels = cave.labels();
        }
        catch (...)
        {
            VirtualFreeEx(hProcess, hook.cave, 0, MEM_RELEASE);
            throw;
        }

        std::vector<BYTE> jump(overwriteLen, 0x90);
        jump[0] = 0xE9;
        int32_t rel = static_cast<int32_t>(caveAddress - (address + 5));
        std::memcpy(&jump[1], &rel, 4);

        if (code.size() > CAVE_SIZE || !writeCode(caveAddress, code) || !writeCode(address, jump))
        {
            std::cerr << "[!] Could not install '" << name << "' (" << code.size() << " cave bytes).\n";
            writeCode(address, hook.originalBytes);
            VirtualFreeEx(hProcess, hook.cave, 0, MEM_RELEASE);
            return false;
        }

        modifications[name] = std::move(hook);
        std::cout << "[+] Hooked '" << name << "' at 0x" << std::hex << address << std::dec << "\n";
        return true;
    }

    /// Changes data a live cave reads - a label its body bound - without rebuilding the hook.
    template <typename T>
    bool writeCaveData(const std::string &name, const std::string &label, const T &value)
    {
        uintptr_t address = caveLabelAddress(name, label);
        if (address == 0)
            return false;
        if (!writeMemory(address, value))
        {
            std::cerr << "[!] Could not write '" << label << "' in the cave of '" << name << "'.\n";
            return false;
        }
        return true;
    }

    /// Reads data a live cave holds - for example a pointer the cave stashed there.
    template <typename T>
    bool readCaveData(const std::string &name, const std::string &label, T &out)
    {
        uintptr_t address = caveLabelAddress(name, label);
        return address != 0 && readMemory(address, out);
    }

    // ------------------------------------------------------------------------------------------------
    // Held values
    // ------------------------------------------------------------------------------------------------
    // A held value is rewritten every HOLD_INTERVAL until disabled, and its chain's pointers are
    // followed again each time, so an object the game frees and reallocates - a reload, a level
    // change - is tracked rather than written over. The module base is resolved once, when the
    // value starts being held.

    /// Keeps the value at chain's end at value. Disabling leaves the last value in place.
    template <typename T>
    bool holdValue(const std::string &name, const PointerChain &chain, T value)
    {
        static_assert(std::is_trivially_copyable_v<T>, "holdValue needs a plain value type");
        if (!claimName(name) || resolve(chain) == 0)
            return false;
        const uintptr_t base = moduleAddress(chain.module, 0);
        return startValueLock(name, [this, base, offsets = chain.offsets, value]()
                              {
                                  if (uintptr_t address = followChain(base, offsets, false))
                                      writeMemory(address, value);
                              },
                              nullptr);
    }

    /// Keeps the value at chain's end at what it is now.
    template <typename T>
    bool freezeValue(const std::string &name, const PointerChain &chain)
    {
        if (!claimName(name))
            return false;
        T current{};
        if (!readValue(chain, current))
            return false;
        return holdValue(name, chain, current);
    }

    /// Keeps each float at chain's end + field at its original value x factor - for tunables such as
    /// a speed scale, where one fixed value would be wrong for another object of the same kind. An
    /// object's originals are read the first time the chain reaches it and written back on disable.
    /// An object the chain has moved off is assumed freed and left alone.
    bool scaleValues(const std::string &name, const PointerChain &chain, const std::vector<uintptr_t> &fields, float factor)
    {
        if (!claimName(name) || resolve(chain) == 0)
            return false;

        struct Scaled
        {
            uintptr_t base = 0;
            std::vector<float> originals;
        };
        auto scaled = std::make_shared<Scaled>();
        const uintptr_t moduleBase = moduleAddress(chain.module, 0);

        auto tick = [this, moduleBase, offsets = chain.offsets, fields, factor, scaled]()
        {
            uintptr_t base = followChain(moduleBase, offsets, false);
            if (base == 0)
                return;
            if (base != scaled->base)
            {
                std::vector<float> originals(fields.size());
                for (size_t i = 0; i < fields.size(); i++)
                    if (!readMemory(base + fields[i], originals[i]))
                        return;
                scaled->base = base;
                scaled->originals = std::move(originals);
            }
            for (size_t i = 0; i < fields.size(); i++)
                writeMemory(base + fields[i], scaled->originals[i] * factor);
        };

        auto restore = [this, fields, scaled]()
        {
            for (size_t i = 0; scaled->base && i < fields.size(); i++)
                writeMemory(scaled->base + fields[i], scaled->originals[i]);
        };

        return startValueLock(name, tick, restore);
    }

    // ------------------------------------------------------------------------------------------------
    // Text
    // ------------------------------------------------------------------------------------------------

    std::string wstringToString(const std::wstring &wstr)
    {
        if (wstr.empty())
            return "";
        int size = WideCharToMultiByte(CP_ACP, 0, wstr.data(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
        std::string str(size, 0);
        WideCharToMultiByte(CP_ACP, 0, wstr.data(), static_cast<int>(wstr.size()), str.data(), size, nullptr, nullptr);
        return str;
    }

private:
    static constexpr size_t CAVE_SIZE = 0x1000;
    static constexpr std::chrono::milliseconds HOLD_INTERVAL{10};

    struct SignatureByte
    {
        bool wildcard;
        BYTE value;
    };

    struct ValueLock
    {
        std::atomic<bool> running{true};
        std::thread thread;
        std::function<void()> onStop;
    };

    /// What one name changed. Code modifications keep the bytes they overwrote and, for a hook, the
    /// cave; a held value keeps the thread that rewrites it.
    struct Modification
    {
        uintptr_t address = 0;
        std::vector<BYTE> originalBytes;
        LPVOID cave = nullptr;
        std::map<std::string, size_t> caveLabels;
        std::unique_ptr<ValueLock> lock;
    };

    std::map<std::string, Modification> modifications;

    /// base + offsets[0], then each later offset added to the pointer read at the address before it.
    uintptr_t followChain(uintptr_t base, const std::vector<uintptr_t> &offsets, bool reportFailure)
    {
        if (offsets.empty())
            return base;
        uintptr_t address = base + offsets[0];
        for (size_t i = 1; i < offsets.size(); i++)
        {
            uintptr_t next = 0;
            const bool readable = readMemory(address, next);
            if (!readable || next == 0)
            {
                if (reportFailure)
                    std::cerr << "[!] Pointer chain broke at link " << i << ": 0x" << std::hex << address << std::dec
                              << (readable ? " holds null" : " is unreadable") << " - is the game past its menus?\n";
                return 0;
            }
            address = next + offsets[i];
        }
        return address;
    }

    bool claimName(const std::string &name)
    {
        if (!isActive(name))
            return true;
        std::cerr << "[!] '" << name << "' is already active.\n";
        return false;
    }

    void undo(Modification &modification)
    {
        if (modification.lock)
        {
            modification.lock->running = false;
            if (modification.lock->thread.joinable())
                modification.lock->thread.join();
            if (modification.lock->onStop)
                modification.lock->onStop();
            return;
        }

        writeCode(modification.address, modification.originalBytes);
        if (modification.cave)
            VirtualFreeEx(hProcess, modification.cave, 0, MEM_RELEASE);
    }

    bool startValueLock(const std::string &name, std::function<void()> tick, std::function<void()> onStop)
    {
        Modification held;
        held.lock = std::make_unique<ValueLock>();
        held.lock->onStop = std::move(onStop);
        ValueLock *lock = held.lock.get();
        lock->thread = std::thread([lock, tick = std::move(tick)]()
                                   {
                                       while (lock->running)
                                       {
                                           tick();
                                           std::this_thread::sleep_for(HOLD_INTERVAL);
                                       }
                                   });
        modifications[name] = std::move(held);
        std::cout << "[+] Holding '" << name << "'\n";
        return true;
    }

    bool writeCode(uintptr_t address, const std::vector<BYTE> &bytes)
    {
        if (!writeBytes(address, bytes.data(), bytes.size()))
            return false;
        FlushInstructionCache(hProcess, reinterpret_cast<LPCVOID>(address), bytes.size());
        return true;
    }

    uintptr_t caveLabelAddress(const std::string &name, const std::string &label)
    {
        auto it = modifications.find(name);
        if (it == modifications.end() || !it->second.cave)
        {
            std::cerr << "[!] '" << name << "' is not an active hook.\n";
            return 0;
        }
        auto offset = it->second.caveLabels.find(label);
        if (offset == it->second.caveLabels.end())
        {
            std::cerr << "[!] The cave of '" << name << "' has no label '" << label << "'.\n";
            return 0;
        }
        return reinterpret_cast<uintptr_t>(it->second.cave) + offset->second;
    }

    static std::vector<SignatureByte> parseSignature(const std::string &signature)
    {
        std::vector<SignatureByte> bytes;
        std::istringstream in(signature);
        for (std::string token; in >> token;)
        {
            if (token == "??" || token == "?")
            {
                bytes.push_back({true, 0});
                continue;
            }
            if (token.size() != 2 || !std::isxdigit(static_cast<unsigned char>(token[0])) || !std::isxdigit(static_cast<unsigned char>(token[1])))
                throw std::invalid_argument("Bad AOB token '" + token + "' in \"" + signature + "\"");
            bytes.push_back({false, static_cast<BYTE>(std::stoul(token, nullptr, 16))});
        }
        if (bytes.empty() || bytes.front().wildcard)
            throw std::invalid_argument("An AOB has to start with a concrete byte: \"" + signature + "\"");
        return bytes;
    }

    /// The first match in committed executable memory between start and end.
    uintptr_t scanExecutable(uintptr_t start, uintptr_t end, const std::vector<SignatureByte> &signature)
    {
        constexpr DWORD executable = PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
        constexpr size_t chunk = 16 * 1024 * 1024;
        const size_t length = signature.size();
        std::vector<BYTE> buffer;

        MEMORY_BASIC_INFORMATION mbi{};
        for (uintptr_t at = start; at < end && VirtualQueryEx(hProcess, reinterpret_cast<LPCVOID>(at), &mbi, sizeof(mbi));)
        {
            const uintptr_t regionStart = (std::max)(reinterpret_cast<uintptr_t>(mbi.BaseAddress), start);
            const uintptr_t regionEnd = (std::min)(static_cast<uintptr_t>(reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize), end);
            at = static_cast<uintptr_t>(reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize);
            if (at <= reinterpret_cast<uintptr_t>(mbi.BaseAddress))
                break; // wrapped past the top of the address space
            if (mbi.State != MEM_COMMIT || !(mbi.Protect & executable) || (mbi.Protect & PAGE_GUARD))
                continue;

            // Chunks overlap by the signature's length so a match across a boundary is still seen.
            for (uintptr_t from = regionStart; from + length <= regionEnd; from += chunk)
            {
                const size_t size = (std::min)(chunk + length - 1, regionEnd - from);
                buffer.resize(size);
                SIZE_T read = 0;
                if (!ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(from), buffer.data(), size, &read))
                    continue;
                if (size_t found = findInBuffer(buffer.data(), read, signature); found != SIZE_MAX)
                    return from + found;
            }
        }
        return 0;
    }

    static size_t findInBuffer(const BYTE *data, size_t size, const std::vector<SignatureByte> &signature)
    {
        const size_t length = signature.size();
        const BYTE first = signature[0].value;
        for (size_t i = 0; i + length <= size;)
        {
            const void *hit = std::memchr(data + i, first, size - length + 1 - i);
            if (!hit)
                return SIZE_MAX;
            i = static_cast<const BYTE *>(hit) - data;
            size_t j = 1;
            while (j < length && (signature[j].wildcard || signature[j].value == data[i + j]))
                j++;
            if (j == length)
                return i;
            i++;
        }
        return SIZE_MAX;
    }

    /// One page for a cave, within rel32 reach of hookAddress: x86 reaches everything, x64 searches the
    /// free regions within 2 GB above hookAddress, then below it.
    LPVOID allocateCave(uintptr_t hookAddress)
    {
#if !defined(_WIN64)
        (void)hookAddress;
        return VirtualAllocEx(hProcess, nullptr, CAVE_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
#else
        SYSTEM_INFO si{};
        GetSystemInfo(&si);
        const uintptr_t granularity = si.dwAllocationGranularity;
        const uintptr_t reach = 0x7FFF0000;
        const uintptr_t lowest = (std::max)(reinterpret_cast<uintptr_t>(si.lpMinimumApplicationAddress), hookAddress > reach ? hookAddress - reach : 0);
        const uintptr_t highest = (std::min)(reinterpret_cast<uintptr_t>(si.lpMaximumApplicationAddress), hookAddress + reach);
        auto alignUp = [granularity](uintptr_t x) { return (x + granularity - 1) / granularity * granularity; };
        auto alignDown = [granularity](uintptr_t x) { return x / granularity * granularity; };
        auto tryAt = [this](uintptr_t candidate) -> LPVOID
        { return VirtualAllocEx(hProcess, reinterpret_cast<LPVOID>(candidate), CAVE_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE); };

        MEMORY_BASIC_INFORMATION mbi{};
        for (uintptr_t at = hookAddress; at < highest && VirtualQueryEx(hProcess, reinterpret_cast<LPCVOID>(at), &mbi, sizeof(mbi));)
        {
            const uintptr_t base = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
            if (mbi.State == MEM_FREE)
            {
                const uintptr_t candidate = alignUp(base);
                if (candidate + CAVE_SIZE <= base + mbi.RegionSize && candidate + CAVE_SIZE <= highest)
                    if (LPVOID cave = tryAt(candidate))
                        return cave;
            }
            at = base + mbi.RegionSize;
        }
        for (uintptr_t at = hookAddress; at > lowest && VirtualQueryEx(hProcess, reinterpret_cast<LPCVOID>(at - 1), &mbi, sizeof(mbi));)
        {
            const uintptr_t base = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
            if (mbi.State == MEM_FREE && base + mbi.RegionSize >= CAVE_SIZE)
            {
                const uintptr_t candidate = alignDown(base + mbi.RegionSize - CAVE_SIZE);
                if (candidate >= base && candidate >= lowest)
                    if (LPVOID cave = tryAt(candidate))
                        return cave;
            }
            at = base;
        }
        return nullptr;
#endif
    }

    /// UTF-8 output, and QuickEdit off until exit: a selection in a Debug trainer's console blocks
    /// its logging, which freezes the UI.
    static void prepareConsole()
    {
        struct QuickEditOff
        {
            HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
            DWORD saved = 0;
            bool changed = GetConsoleMode(input, &saved) &&
                           SetConsoleMode(input, (saved & ~ENABLE_QUICK_EDIT_MODE) | ENABLE_EXTENDED_FLAGS);

            ~QuickEditOff()
            {
                if (changed)
                    SetConsoleMode(input, saved | ENABLE_EXTENDED_FLAGS);
            }
        };
        static QuickEditOff quickEditOff;
        SetConsoleOutputCP(CP_UTF8);
    }

    DWORD findProcessId()
    {
        if (useWindowTitle)
        {
            WindowSearch search{&processName};
            EnumWindows(matchWindowTitle, reinterpret_cast<LPARAM>(&search));
            return search.pid;
        }

        DWORD pid = 0;
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE)
            return 0;
        PROCESSENTRY32W pe{};
        pe.dwSize = sizeof(pe);
        for (BOOL more = Process32FirstW(snap, &pe); more && pid == 0; more = Process32NextW(snap, &pe))
            if (_wcsicmp(pe.szExeFile, processName.c_str()) == 0)
                pid = pe.th32ProcessID;
        CloseHandle(snap);
        return pid;
    }

    struct WindowSearch
    {
        const std::wstring *title;
        DWORD pid = 0;
    };

    static BOOL CALLBACK matchWindowTitle(HWND hwnd, LPARAM param)
    {
        auto *search = reinterpret_cast<WindowSearch *>(param);
        wchar_t title[256] = {0};
        if (!IsWindowVisible(hwnd) || !GetWindowTextW(hwnd, title, 256) || *search->title != title)
            return TRUE;
        GetWindowThreadProcessId(hwnd, &search->pid);
        return FALSE;
    }
};
