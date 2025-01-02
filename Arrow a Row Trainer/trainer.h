#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <cstdint>
#include <string>
#include <map>
#include <vector>
#include <functional>
#include <iostream>

/***********************************************************************
 * HOOKINFO: track data about one named hook
 ***********************************************************************/
struct HookInfo
{
    std::string hookName;  // e.g. "ArrowFrequency"
    uintptr_t hookAddress; // Where we overwrite bytes
    size_t overwriteLen;   // # of bytes overwritten
    std::vector<BYTE> originalBytes;
    bool active = false;

    LPVOID allocatedMem = nullptr;
    size_t allocSize = 0;
};

/***********************************************************************
 * WILDCARD PATTERN BYTE
 *  If text == "??" => wildcard
 *  Otherwise, parse hex
 ***********************************************************************/
struct PatternByte
{
    bool wildcard;
    BYTE value;
};

class GameTrainer
{
public:
    static inline const wchar_t *PROCESS_NAME = L"Arrow a Row.exe"; // x64
    static inline const wchar_t *MODULE_NAME = L"GameAssembly.dll";

    HANDLE hProcess = nullptr;
    DWORD procId = 0;

    // store named hooks in a map:  hookName -> HookInfo
    std::map<std::string, HookInfo> hooks;

public:
    ~GameTrainer()
    {
        // automatically disable all hooks if we exit
        disableAllHooks();
        if (hProcess)
            CloseHandle(hProcess);
    }

    bool isProcessRunning()
    {
        procId = getProcId(PROCESS_NAME);
        if (procId == 0)
        {
            std::wcerr << L"[!] Could not find process: " << PROCESS_NAME << std::endl;
            return false;
        }

        if (!hProcess)
        {
            hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procId);
            if (!hProcess)
            {
                std::cerr << "[!] Failed to open process. Error: " << GetLastError() << std::endl;
                return false;
            }
        }
        return true;
    }

    void setHealth(float health)
    {
        // Example offsets
        std::vector<unsigned int> healthOffsets = {
            0x01918328, 0xC0, 0x90, 0x40, 0xB8, 0x20, 0x18, 0x28};

        uintptr_t finalAddress = resolveModuleDynamicAddress(MODULE_NAME, healthOffsets);
        if (finalAddress == 0)
        {
            std::cerr << "[setHealth] Failed to resolve health value address via pointers." << std::endl;
            return;
        }

        if (!WriteProcessMemory(hProcess, (LPVOID)finalAddress, &health, sizeof(health), nullptr))
        {
            std::cerr << "[setHealth] Failed to write health value." << std::endl;
        }
        else
        {
            std::cout << "[+] Health set to " << health << std::endl;
        }
    }

    bool setArrowFrequency(float newVal)
    {
        // Actual target instruction bytes: F3 0F 11 73 4C
        std::vector<std::string> bigPat = {"F3", "0F", "11", "73", "4C"};

        size_t patternOffset = 0; // hooking instruction
        size_t overwriteLen = 5;  // we detour with E9 <rel32>
        size_t codeSize = 0x600;  // enough for code + float

        // We'll do a jump-back to `hookAddr + 5` at the end.
        auto buildFunc = [newVal, overwriteLen](uintptr_t base, uintptr_t hookAddr) -> std::vector<BYTE>
        {
            // We'll place the float at offset 0x400
            std::vector<BYTE> code(0x600, 0x90);

            float *pFloat = reinterpret_cast<float *>(&code[0x400]);
            *pFloat = newVal;

            // Injection stub:
            //   sub rsp,0x10
            //   movss xmm6,[RIP+disp32]   ; read float from code block
            //   movss [rbx+4C], xmm6
            //   add rsp,0x10
            //   jmp hookAddr+5  ; jump back to original code
            size_t wPos = 0;

            // sub rsp,0x10 => 48 83 EC 10
            code[wPos++] = 0x48;
            code[wPos++] = 0x83;
            code[wPos++] = 0xEC;
            code[wPos++] = 0x10;

            // movss xmm6,[RIP+disp32] => F3 0F 10 35 <newVal>
            code[wPos++] = 0xF3;
            code[wPos++] = 0x0F;
            code[wPos++] = 0x10;
            code[wPos++] = 0x35;
            {
                uintptr_t floatAddr = base + 0x400;
                uintptr_t nextInstr = base + wPos + 4;
                int64_t diff = (int64_t)floatAddr - (int64_t)nextInstr;
                int32_t disp32 = (int32_t)diff;
                BYTE *pDisp = reinterpret_cast<BYTE *>(&disp32);
                for (int i = 0; i < 4; i++)
                    code[wPos++] = pDisp[i];
            }

            // movss [rbx+4C], xmm6 => F3 0F 11 73 4C
            code[wPos++] = 0xF3;
            code[wPos++] = 0x0F;
            code[wPos++] = 0x11;
            code[wPos++] = 0x73;
            code[wPos++] = 0x4C;

            // add rsp,0x10 => 48 83 C4 10
            code[wPos++] = 0x48;
            code[wPos++] = 0x83;
            code[wPos++] = 0xC4;
            code[wPos++] = 0x10;

            // jmp hookAddr+[overwriteLen] => E9 <rel32>
            {
                code[wPos++] = 0xE9;
                uintptr_t retAddr = hookAddr + overwriteLen;
                uintptr_t nextInstr = (uintptr_t)(base + wPos + 4);
                int32_t rel = (int32_t)(retAddr - nextInstr);
                BYTE *pRel = reinterpret_cast<BYTE *>(&rel);
                for (int i = 0; i < 4; i++)
                    code[wPos++] = pRel[i];
            }

            return code;
        };

        return createNamedHook(
            "ArrowFrequency",
            bigPat,
            patternOffset,
            overwriteLen,
            codeSize,
            buildFunc);

        // disableNamedHook("ArrowFrequency");
    }

    bool setArrowDamage(float newVal)
    {
        // Actual target instruction bytes: F3 0F 11 43 44
        std::vector<std::string> bigPat = {"F3", "0F", "11", "43", "44", "0F"};

        size_t patternOffset = 0;
        size_t overwriteLen = 5;
        size_t codeSize = 0x600;

        auto buildFunc = [newVal, overwriteLen](uintptr_t base, uintptr_t hookAddr) -> std::vector<BYTE>
        {
            std::vector<BYTE> code(0x600, 0x90);

            float *pFloat = reinterpret_cast<float *>(&code[0x400]);
            *pFloat = newVal;

            // Injection stub:
            //   movss xmm0,[RIP+disp32]
            //   movss [rbx+44],xmm0
            //   jmp hookAddr+[overwriteLen]
            size_t wPos = 0;

            // movss xmm0,[RIP+disp32]
            code[wPos++] = 0xF3;
            code[wPos++] = 0x0F;
            code[wPos++] = 0x10;
            code[wPos++] = 0x05;
            {
                uintptr_t floatAddr = base + 0x400;
                uintptr_t nextInstr = base + wPos + 4;
                int64_t diff = (int64_t)floatAddr - (int64_t)nextInstr;
                int32_t disp32 = (int32_t)diff;
                BYTE *pDisp = reinterpret_cast<BYTE *>(&disp32);
                for (int i = 0; i < 4; i++)
                    code[wPos++] = pDisp[i];
            }

            // movss [rbx+44],xmm0
            code[wPos++] = 0xF3;
            code[wPos++] = 0x0F;
            code[wPos++] = 0x11;
            code[wPos++] = 0x43;
            code[wPos++] = 0x44;

            // jmp hookAddr+[overwriteLen]
            {
                code[wPos++] = 0xE9;
                uintptr_t retAddr = hookAddr + overwriteLen;
                uintptr_t nextInstr = (uintptr_t)(base + wPos + 4);
                int32_t rel = (int32_t)(retAddr - nextInstr);
                BYTE *pRel = reinterpret_cast<BYTE *>(&rel);
                for (int i = 0; i < 4; i++)
                    code[wPos++] = pRel[i];
            }

            return code;
        };

        return createNamedHook(
            "ArrowDamage",
            bigPat,
            patternOffset,
            overwriteLen,
            codeSize,
            buildFunc);

        // disableNamedHook("ArrowDamage");
    }

    bool setArrowSpeed(float newVal)
    {
        // Actual target instruction bytes: F3 0F 11 7B 38
        std::vector<std::string> bigPat = {"F3", "0F", "11", "7B", "38", "83"};

        size_t patternOffset = 0;
        size_t overwriteLen = 5;
        size_t codeSize = 0x600;

        auto buildFunc = [newVal, overwriteLen](uintptr_t base, uintptr_t hookAddr) -> std::vector<BYTE>
        {
            std::vector<BYTE> code(0x600, 0x90);

            float *pFloat = reinterpret_cast<float *>(&code[0x400]);
            *pFloat = newVal;

            // Injection stub:
            //   movss xmm7,[RIP+disp32]
            //   movss [rbx+38],xmm7
            //   jmp hookAddr+[overwriteLen]
            size_t wPos = 0;

            // movss xmm7,[RIP+disp32]
            code[wPos++] = 0xF3;
            code[wPos++] = 0x0F;
            code[wPos++] = 0x10;
            code[wPos++] = 0x3D;
            {
                uintptr_t floatAddr = base + 0x400;
                uintptr_t nextInstr = base + wPos + 4;
                int64_t diff = (int64_t)floatAddr - (int64_t)nextInstr;
                int32_t disp32 = (int32_t)diff;
                BYTE *pDisp = reinterpret_cast<BYTE *>(&disp32);
                for (int i = 0; i < 4; i++)
                    code[wPos++] = pDisp[i];
            }

            // movss [rbx+38],xmm7
            code[wPos++] = 0xF3;
            code[wPos++] = 0x0F;
            code[wPos++] = 0x11;
            code[wPos++] = 0x7B;
            code[wPos++] = 0x38;

            // jmp hookAddr+[overwriteLen]
            {
                code[wPos++] = 0xE9;
                uintptr_t retAddr = hookAddr + overwriteLen;
                uintptr_t nextInstr = (uintptr_t)(base + wPos + 4);
                int32_t rel = (int32_t)(retAddr - nextInstr);
                BYTE *pRel = reinterpret_cast<BYTE *>(&rel);
                for (int i = 0; i < 4; i++)
                    code[wPos++] = pRel[i];
            }

            return code;
        };

        return createNamedHook(
            "ArrowSpeed",
            bigPat,
            patternOffset,
            overwriteLen,
            codeSize,
            buildFunc);

        // disableNamedHook("ArrowSpeed");
    }

    bool setArrowDistance(float newVal)
    {
        // Actual target instruction bytes: F3 0F 11 73 40
        std::vector<std::string> bigPat = {"F3", "0F", "11", "73", "40"};

        size_t patternOffset = 0;
        size_t overwriteLen = 5;
        size_t codeSize = 0x600;

        auto buildFunc = [newVal, overwriteLen](uintptr_t base, uintptr_t hookAddr) -> std::vector<BYTE>
        {
            std::vector<BYTE> code(0x600, 0x90);

            float *pFloat = reinterpret_cast<float *>(&code[0x400]);
            *pFloat = newVal;

            // Injection stub:
            //   movss xmm6,[RIP+disp32]
            //   movss [rbx+40],xmm6
            //   jmp hookAddr+[overwriteLen]
            size_t wPos = 0;

            // xmm6,[RIP+disp32]
            code[wPos++] = 0xF3;
            code[wPos++] = 0x0F;
            code[wPos++] = 0x10;
            code[wPos++] = 0x35;
            {
                uintptr_t floatAddr = base + 0x400;
                uintptr_t nextInstr = base + wPos + 4;
                int64_t diff = (int64_t)floatAddr - (int64_t)nextInstr;
                int32_t disp32 = (int32_t)diff;
                BYTE *pDisp = reinterpret_cast<BYTE *>(&disp32);
                for (int i = 0; i < 4; i++)
                    code[wPos++] = pDisp[i];
            }

            // movss [rbx+40],xmm6
            code[wPos++] = 0xF3;
            code[wPos++] = 0x0F;
            code[wPos++] = 0x11;
            code[wPos++] = 0x73;
            code[wPos++] = 0x40;

            // jmp hookAddr+[overwriteLen]
            {
                code[wPos++] = 0xE9;
                uintptr_t retAddr = hookAddr + overwriteLen;
                uintptr_t nextInstr = (uintptr_t)(base + wPos + 4);
                int32_t rel = (int32_t)(retAddr - nextInstr);
                BYTE *pRel = reinterpret_cast<BYTE *>(&rel);
                for (int i = 0; i < 4; i++)
                    code[wPos++] = pRel[i];
            }

            return code;
        };

        return createNamedHook(
            "ArrowDistance",
            bigPat,
            patternOffset,
            overwriteLen,
            codeSize,
            buildFunc);

        // disableNamedHook("ArrowDistance");
    }

private:
    DWORD getProcId(const wchar_t *exeName)
    {
        DWORD pid = 0;
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE)
            return 0;

        PROCESSENTRY32W pe;
        pe.dwSize = sizeof(pe);
        if (Process32FirstW(snap, &pe))
        {
            do
            {
                if (!_wcsicmp(pe.szExeFile, exeName))
                {
                    pid = pe.th32ProcessID;
                    break;
                }
            } while (Process32NextW(snap, &pe));
        }
        CloseHandle(snap);
        return pid;
    }

    bool getModuleInfo(const wchar_t *modName, uintptr_t &modBase, size_t &modSize)
    {
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procId);
        if (snap == INVALID_HANDLE_VALUE)
            return false;

        MODULEENTRY32W me;
        me.dwSize = sizeof(me);
        bool found = false;
        if (Module32FirstW(snap, &me))
        {
            do
            {
                if (!_wcsicmp(me.szModule, modName))
                {
                    modBase = (uintptr_t)me.modBaseAddr;
                    modSize = (size_t)me.modBaseSize;
                    found = true;
                    break;
                }
            } while (Module32NextW(snap, &me));
        }
        CloseHandle(snap);
        return found;
    }

    /***********************************************************************
     * resolveModuleDynamicAddress (Pointer-chaining approach)
     ***********************************************************************/
    uintptr_t resolveModuleDynamicAddress(
        const wchar_t *moduleName,
        const std::vector<unsigned int> &offsets)
    {
        uintptr_t modBase = 0;
        size_t modSize = 0;
        if (!getModuleInfo(moduleName, modBase, modSize))
            return 0;

        // First offset is relative to moduleBase
        uintptr_t currentAddr = modBase + offsets[0];

        // subsequent offsets are pointer dereferences
        for (size_t i = 1; i < offsets.size(); i++)
        {
            if (!ReadProcessMemory(hProcess, (LPCVOID)currentAddr, &currentAddr, sizeof(currentAddr), nullptr))
            {
                std::cerr << "[resolveModuleDynamicAddress] Failed deref at 0x"
                          << std::hex << currentAddr << std::dec << std::endl;
                return 0;
            }
            currentAddr += offsets[i];
        }
        return currentAddr;
    }

    /***********************************************************************
     * findPatternWild - scan with wildcard strings
     ***********************************************************************/
    uintptr_t findPatternWild(const wchar_t *moduleName, const std::vector<std::string> &pattern)
    {
        uintptr_t base = 0;
        size_t modSize = 0;
        if (!getModuleInfo(moduleName, base, modSize))
            return 0;

        // parse pattern
        std::vector<PatternByte> pat;
        for (auto &token : pattern)
        {
            if (token == "??")
            {
                PatternByte pb{true, 0x00};
                pat.push_back(pb);
            }
            else
            {
                unsigned int val = 0;
                sscanf_s(token.c_str(), "%x", &val);
                PatternByte pb{false, (BYTE)val};
                pat.push_back(pb);
            }
        }

        // read module bytes
        std::vector<BYTE> buf(modSize);
        SIZE_T bytesRead = 0;
        if (!ReadProcessMemory(hProcess, (LPCVOID)base, buf.data(), modSize, &bytesRead))
            return 0;

        size_t patSize = pat.size();
        for (size_t i = 0; i + patSize <= bytesRead; i++)
        {
            bool matched = true;
            for (size_t j = 0; j < patSize; j++)
            {
                if (!pat[j].wildcard)
                {
                    if (buf[i + j] != pat[j].value)
                    {
                        matched = false;
                        break;
                    }
                }
            }
            if (matched)
            {
                return base + i;
            }
        }
        return 0;
    }

    /***********************************************************************
     * allocNearAddress - ±2GB from 'addr'
     ***********************************************************************/
    LPVOID allocNearAddress(uintptr_t addr, size_t sizeNeeded)
    {
        const size_t TWO_GB = (1ULL << 31);
        const size_t step = 0x10000; // 64k

        uintptr_t start = (addr > TWO_GB) ? (addr - TWO_GB) : 0;
        uintptr_t end = addr + TWO_GB;

        auto alignDown = [&](uintptr_t x)
        { return (x / step) * step; };
        start = alignDown(start);
        end = alignDown(end);

        for (uintptr_t curr = start; curr + sizeNeeded < end; curr += step)
        {
            LPVOID p = VirtualAllocEx(
                hProcess,
                (LPVOID)curr,
                sizeNeeded,
                MEM_COMMIT | MEM_RESERVE,
                PAGE_EXECUTE_READWRITE);
            if (p)
            {
                uintptr_t diff = (curr > addr) ? (curr - addr) : (addr - curr);
                if (diff <= TWO_GB)
                    return p;

                VirtualFreeEx(hProcess, p, 0, MEM_RELEASE);
            }
        }
        return nullptr;
    }

    /***********************************************************************
     * createNamedHook
     *    - name: unique string name, e.g. "ArrowFrequency"
     *    - pattern: e.g. {"0F", "28", "CE", "F3", "0F", "11", "73", "4C", ...}
     *      with "??" for wildcards
     *    - patternOffset: how far into that found pattern we actually place the hook
     *      e.g. if the pattern is 12 bytes, but the instruction to hook starts at offset 3
     *    - overwriteLen: how many bytes we overwrite
     *    - codeSize: how many bytes to allocate for code cave
     *    - buildFunc: a lambda that, given the base address of the allocated block,
     *      and the hookAddr where the original instruction started,
     *      returns the final code bytes (which can also embed data).
     ***********************************************************************/
    bool createNamedHook(
        const std::string &name,
        const std::vector<std::string> &pattern,
        size_t patternOffset,
        size_t overwriteLen,
        size_t codeSize,
        std::function<std::vector<BYTE>(uintptr_t base, uintptr_t hookAddr)> buildFunc)
    {
        // A) find pattern w/wildcards
        uintptr_t matchAddr = findPatternWild(MODULE_NAME, pattern);
        if (!matchAddr)
        {
            std::cerr << "[!] Could not find pattern for hook '" << name << "'.\n";
            return false;
        }
        // The actual instruction address is matchAddr + patternOffset
        uintptr_t hookAddr = matchAddr + patternOffset;

        // B) read original bytes
        std::vector<BYTE> orig(overwriteLen);
        if (!ReadProcessMemory(hProcess, (LPCVOID)hookAddr, orig.data(), overwriteLen, nullptr))
        {
            std::cerr << "[!] Failed to read original bytes for hook '" << name << "'.\n";
            return false;
        }

        // C) allocate near memory
        LPVOID nearMem = allocNearAddress(hookAddr, codeSize);
        if (!nearMem)
        {
            std::cerr << "[!] allocNearAddress failed for hook '" << name << "'.\n";
            return false;
        }

        // D) build injection code
        uintptr_t base = (uintptr_t)nearMem;
        std::vector<BYTE> code = buildFunc(base, hookAddr);
        if (code.empty())
        {
            std::cerr << "[!] buildFunc returned empty code for '" << name << "'.\n";
            VirtualFreeEx(hProcess, nearMem, 0, MEM_RELEASE);
            return false;
        }

        // E) write code to nearMem
        SIZE_T written = 0;
        if (!WriteProcessMemory(hProcess, nearMem, code.data(), code.size(), &written))
        {
            std::cerr << "[!] WriteProcessMemory code failed for hook '" << name << "'.\n";
            VirtualFreeEx(hProcess, nearMem, 0, MEM_RELEASE);
            return false;
        }
        FlushInstructionCache(hProcess, nearMem, code.size());

        // F) overwrite the original instructions with a JMP
        //    If overwriteLen > 5, fill the rest with NOP
        {
            std::vector<BYTE> patch(overwriteLen, 0x90); // fill w/NOP
            // short jump
            patch[0] = 0xE9;
            int32_t rel = (int32_t)((uintptr_t)nearMem - (hookAddr + overwriteLen));
            memcpy(&patch[1], &rel, 4);

            if (!WriteProcessMemory(hProcess, (LPVOID)hookAddr, patch.data(), patch.size(), nullptr))
            {
                std::cerr << "[!] Overwrite instruction failed for '" << name << "'.\n";
                VirtualFreeEx(hProcess, nearMem, 0, MEM_RELEASE);
                return false;
            }
            FlushInstructionCache(hProcess, (LPCVOID)hookAddr, patch.size());
        }

        // G) store HookInfo
        HookInfo hi;
        hi.hookName = name;
        hi.hookAddress = hookAddr;
        hi.overwriteLen = overwriteLen;
        hi.active = true;
        hi.originalBytes = orig;
        hi.allocatedMem = nearMem;
        hi.allocSize = codeSize;

        hooks[name] = hi; // store in map by name

        std::cout << "[+] Hook '" << name << "' created at "
                  << std::hex << hookAddr << std::dec << std::endl;
        return true;
    }

    bool disableNamedHook(const std::string &name)
    {
        auto it = hooks.find(name);
        if (it == hooks.end())
            return false; // not found

        HookInfo &hi = it->second;
        if (!hi.active)
            return false;

        // restore original
        WriteProcessMemory(hProcess, (LPVOID)hi.hookAddress, hi.originalBytes.data(), hi.overwriteLen, nullptr);
        FlushInstructionCache(hProcess, (LPCVOID)hi.hookAddress, hi.overwriteLen);

        // free memory
        if (hi.allocatedMem)
        {
            VirtualFreeEx(hProcess, hi.allocatedMem, 0, MEM_RELEASE);
            hi.allocatedMem = nullptr;
        }

        hi.active = false;
        std::cout << "[+] Disabled hook '" << name << "'\n";
        return true;
    }

    void disableAllHooks()
    {
        for (auto &kv : hooks)
        {
            HookInfo &hi = kv.second;
            if (hi.active)
            {
                WriteProcessMemory(hProcess, (LPVOID)hi.hookAddress, hi.originalBytes.data(), hi.overwriteLen, nullptr);
                FlushInstructionCache(hProcess, (LPCVOID)hi.hookAddress, hi.overwriteLen);

                if (hi.allocatedMem)
                {
                    VirtualFreeEx(hProcess, hi.allocatedMem, 0, MEM_RELEASE);
                    hi.allocatedMem = nullptr;
                }
                hi.active = false;
                std::cout << "[+] Disabled hook '" << kv.first << "'\n";
            }
        }
        hooks.clear();
    }
};
