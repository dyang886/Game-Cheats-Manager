// TrainerBase.h
#pragma once

#include <Windows.h>
#include <TlHelp32.h>
#include <string>
#include <map>
#include <vector>
#include <functional>
#include <iostream>
#include <thread>
#include <any>

/***********************************************************************
 * HOOK_INFO: Track data about one named hook
 ***********************************************************************/
struct HookInfo
{
    std::string hookName;  // e.g., "ArrowFrequency"
    uintptr_t hookAddress; // Where we overwrite bytes
    size_t overwriteLen;   // Number of bytes overwritten
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

/***********************************************************************
 * POINTER_TOGGLE_INFO: Track data about one pointer-based toggle
 ***********************************************************************/
struct PointerToggleInfo
{
    std::string toggleName;
    std::vector<unsigned int> offsets;
    std::any desiredValue;
    size_t valueSize;
    bool active = false;
    std::thread toggleThread;

    // Destructor to ensure the thread is joined
    ~PointerToggleInfo()
    {
        active = false;
        if (toggleThread.joinable())
            toggleThread.join();
    }
};

class TrainerBase
{
public:
    // Constructor and Destructor
    TrainerBase(const std::wstring &processName) : processName(processName) {}

    virtual ~TrainerBase()
    {
        disableAllHooks();
        disableAllPointerToggles();
        if (hProcess)
            CloseHandle(hProcess);
    }

    std::wstring getProcessName() const
    {
        return processName;
    }

    DWORD getProcessId() const
    {
        return procId;
    }

    // Check if the target process is running and open a handle
    bool isProcessRunning()
    {
        procId = getProcId(processName.c_str());
        if (procId == 0)
        {
            std::wcerr << L"[!] Could not find process: " << processName << std::endl;
            disableAllHooks();
            disableAllPointerToggles();
            return false;
        }

        if (!hProcess)
        {
            hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procId);
            if (!hProcess)
            {
                std::cerr << "[!] Failed to open process. Error: " << GetLastError() << std::endl;
                disableAllHooks();
                disableAllPointerToggles();
                return false;
            }
        }
        return true;
    }

    // Disable a specific named hook
    inline bool disableNamedHook(const std::string &name)
    {
        auto it = hooks.find(name);
        if (it == hooks.end())
            return false; // Not found

        HookInfo &hi = it->second;
        if (!hi.active)
            return false;

        // Restore original bytes
        WriteProcessMemory(hProcess, reinterpret_cast<LPVOID>(hi.hookAddress), hi.originalBytes.data(), hi.overwriteLen, nullptr);
        FlushInstructionCache(hProcess, reinterpret_cast<LPCVOID>(hi.hookAddress), hi.overwriteLen);

        // Free allocated memory
        if (hi.allocatedMem)
        {
            VirtualFreeEx(hProcess, hi.allocatedMem, 0, MEM_RELEASE);
            hi.allocatedMem = nullptr;
        }

        hi.active = false;
        std::cout << "[-] Disabled hook '" << name << "'\n";
        return true;
    }

    // Disable all active hooks
    inline void disableAllHooks()
    {
        for (auto &kv : hooks)
        {
            HookInfo &hi = kv.second;
            if (hi.active)
            {
                // Restore original bytes
                WriteProcessMemory(hProcess, reinterpret_cast<LPVOID>(hi.hookAddress), hi.originalBytes.data(), hi.overwriteLen, nullptr);
                FlushInstructionCache(hProcess, reinterpret_cast<LPCVOID>(hi.hookAddress), hi.overwriteLen);

                // Free allocated memory
                if (hi.allocatedMem)
                {
                    VirtualFreeEx(hProcess, hi.allocatedMem, 0, MEM_RELEASE);
                    hi.allocatedMem = nullptr;
                }
                hi.active = false;
                std::cout << "[-] Disabled hook '" << kv.first << "'\n";
            }
        }
        hooks.clear();
    }

    // Disable a specific pointer-based toggle
    inline bool disableNamedPointerToggle(const std::string &name)
    {
        auto it = pointerToggles.find(name);
        if (it == pointerToggles.end())
            return false; // Not found

        auto pti = it->second;
        if (!pti->active)
            return false;

        pti->active = false; // Signal the thread to stop

        if (pti->toggleThread.joinable())
            pti->toggleThread.join();

        std::cout << "[-] Disabled pointer toggle '" << name << "'\n";
        pointerToggles.erase(it);
        return true;
    }

    // Disable all active pointer-based toggles
    inline void disableAllPointerToggles()
    {
        for (auto &kv : pointerToggles)
        {
            auto pti = kv.second;
            if (pti->active)
            {
                pti->active = false;
                if (pti->toggleThread.joinable())
                    pti->toggleThread.join();
                std::cout << "[-] Disabled pointer toggle '" << kv.first << "'\n";
            }
        }
        pointerToggles.clear();
    }

protected:
    // Process and module information
    const std::wstring processName;
    HANDLE hProcess = nullptr;
    DWORD procId = 0;

    // Maps to store hooks and pointer toggles by name
    std::map<std::string, HookInfo> hooks;
    std::map<std::string, std::shared_ptr<PointerToggleInfo>> pointerToggles;

    /***********************************************************************
     * Helper Functions
     ***********************************************************************/

    // Get Process ID by executable name
    inline DWORD getProcId(const wchar_t *exeName)
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

    // Get Module Base Address and Size
    inline bool getModuleInfo(const wchar_t *modName, uintptr_t &modBase, size_t &modSize)
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

    // Resolve dynamic address via pointer chaining
    inline uintptr_t resolveModuleDynamicAddress(const wchar_t *moduleName, const std::vector<unsigned int> &offsets)
    {
        uintptr_t modBase = 0;
        size_t modSize = 0;
        if (!getModuleInfo(moduleName, modBase, modSize))
            return 0;

        // First offset is relative to moduleBase
        uintptr_t currentAddr = modBase + offsets[0];

        // Subsequent offsets are pointer dereferences
        for (size_t i = 1; i < offsets.size(); i++)
        {
            if (!ReadProcessMemory(hProcess, (LPCVOID)currentAddr, &currentAddr, sizeof(currentAddr), nullptr))
            {
                std::cerr << "[!] Failed dereference pointer at 0x" << std::hex << currentAddr << std::dec << std::endl;
                return 0;
            }
            currentAddr += offsets[i];
        }

        std::cout << "[+] Pointer dereferenced to 0x" << std::hex << currentAddr << std::dec << std::endl;
        return currentAddr;
    }

    // Find pattern with wildcards in the specified module
    inline uintptr_t findPatternWild(const wchar_t *moduleName, const std::vector<std::string> &pattern)
    {
        uintptr_t base = 0;
        size_t modSize = 0;
        if (!getModuleInfo(moduleName, base, modSize))
            return 0;

        // Parse pattern
        std::vector<PatternByte> pat;
        for (const auto &token : pattern)
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
                PatternByte pb{false, static_cast<BYTE>(val)};
                pat.push_back(pb);
            }
        }

        // Read module bytes
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

    // Allocate memory near a specific address (±2GB)
    inline LPVOID allocNearAddress(uintptr_t addr, size_t sizeNeeded)
    {
        const size_t TWO_GB = (1ULL << 31);
        const size_t step = 0x10000; // 64KB

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
     *    - name: Unique string name, e.g., "ArrowFrequency"
     *    - pattern: e.g., {"0F", "28", "CE", "F3", "0F", "11", "73", "4C", ...}
     *      with "??" for wildcards
     *    - patternOffset: How far into that found pattern we actually place the hook
     *      e.g., if the pattern is 12 bytes, but the instruction to hook starts at offset 3
     *    - overwriteLen: How many bytes we overwrite
     *    - codeSize: How many bytes to allocate for code cave
     *    - buildFunc: A lambda that, given the base address of the allocated block,
     *      and the hookAddr where the original instruction started,
     *      returns the final code bytes (which can also embed data).
     ***********************************************************************/
    inline bool createNamedHook(
        const wchar_t *moduleName,
        const std::string &name,
        const std::vector<std::string> &pattern,
        size_t patternOffset,
        size_t overwriteLen,
        size_t codeSize,
        std::function<std::vector<BYTE>(uintptr_t base, uintptr_t hookAddr)> buildFunc)
    {
        // A) Find pattern with wildcards
        uintptr_t matchAddr = findPatternWild(moduleName, pattern);
        if (!matchAddr)
        {
            std::cerr << "[!] Could not find pattern for hook '" << name << "'.\n";
            return false;
        }
        // The actual instruction address is matchAddr + patternOffset
        uintptr_t hookAddr = matchAddr + patternOffset;

        // B) Read original bytes
        std::vector<BYTE> orig(overwriteLen);
        if (!ReadProcessMemory(hProcess, (LPCVOID)hookAddr, orig.data(), overwriteLen, nullptr))
        {
            std::cerr << "[!] Failed to read original bytes for hook '" << name << "'.\n";
            return false;
        }

        // C) Allocate near memory
        LPVOID nearMem = allocNearAddress(hookAddr, codeSize);
        if (!nearMem)
        {
            std::cerr << "[!] allocNearAddress failed for hook '" << name << "'.\n";
            return false;
        }

        // D) Build injection code
        uintptr_t base = reinterpret_cast<uintptr_t>(nearMem);
        std::vector<BYTE> code = buildFunc(base, hookAddr);
        if (code.empty())
        {
            std::cerr << "[!] buildFunc returned empty code for '" << name << "'.\n";
            VirtualFreeEx(hProcess, nearMem, 0, MEM_RELEASE);
            return false;
        }

        // E) Write code to nearMem
        SIZE_T written = 0;
        if (!WriteProcessMemory(hProcess, nearMem, code.data(), code.size(), &written))
        {
            std::cerr << "[!] WriteProcessMemory code failed for hook '" << name << "'.\n";
            VirtualFreeEx(hProcess, nearMem, 0, MEM_RELEASE);
            return false;
        }
        FlushInstructionCache(hProcess, nearMem, code.size());

        // F) Overwrite the original instructions with a JMP
        //    If overwriteLen > 5, fill the rest with NOP
        std::vector<BYTE> patch(overwriteLen, 0x90); // Fill with NOP
        patch[0] = 0xE9;                             // JMP opcode
        int32_t rel = static_cast<int32_t>(reinterpret_cast<uintptr_t>(nearMem) - (hookAddr + 5));
        std::memcpy(&patch[1], &rel, 4);

        if (!WriteProcessMemory(hProcess, reinterpret_cast<LPVOID>(hookAddr), patch.data(), patch.size(), nullptr))
        {
            std::cerr << "[!] Overwrite instruction failed for '" << name << "'.\n";
            VirtualFreeEx(hProcess, nearMem, 0, MEM_RELEASE);
            return false;
        }
        FlushInstructionCache(hProcess, reinterpret_cast<LPCVOID>(hookAddr), patch.size());

        // G) Store HookInfo
        HookInfo hi;
        hi.hookName = name;
        hi.hookAddress = hookAddr;
        hi.overwriteLen = overwriteLen;
        hi.active = true;
        hi.originalBytes = orig;
        hi.allocatedMem = nearMem;
        hi.allocSize = codeSize;

        hooks[name] = hi; // Store in map by name

        std::cout << "[+] Hook '" << name << "' created at " << std::hex << hookAddr << std::dec << std::endl;
        return true;
    }

    /***********************************************************************
     * createPointerToggle
     *    - name: Unique string name, e.g., "HealthToggle"
     *    - offsets: Pointer chain offsets to reach the target value
     *    - desiredValue: The value to maintain at the target address
     ***********************************************************************/
    template <typename T>
    inline bool createPointerToggle(
        const wchar_t *moduleName,
        const std::string &name,
        const std::vector<unsigned int> &offsets,
        const T desiredValue)
    {
        if (pointerToggles.find(name) != pointerToggles.end())
        {
            std::cerr << "[!] Pointer toggle with name '" << name << "' already exists.\n";
            return false;
        }

        // Create a shared_ptr for PointerToggleInfo
        auto pti = std::make_shared<PointerToggleInfo>();
        pti->toggleName = name;
        pti->offsets = offsets;
        pti->desiredValue = desiredValue;
        pti->active = true;
        pti->valueSize = sizeof(T); // Store the size of the desired value

        // Resolve the address once
        uintptr_t targetAddr = resolveModuleDynamicAddress(moduleName, offsets);
        if (targetAddr == 0)
        {
            std::cerr << "[!] Failed to resolve address for pointer toggle '" << name << "'.\n";
            return false;
        }

        // Insert into the map before starting the thread to ensure the pti remains valid
        pointerToggles[name] = pti;

        // Create a weak_ptr to avoid circular references
        std::weak_ptr<PointerToggleInfo> weak_pti = pti;

        // Start a thread to continuously write the desired value
        pti->toggleThread = std::thread([this, weak_pti, targetAddr]()
                                        {
        // Attempt to lock the weak_ptr
        if (auto shared_pti = weak_pti.lock())
        {
            while (shared_pti->active)
            {
                if (shared_pti->desiredValue.type() == typeid(int))
                {
                    int value = std::any_cast<int>(shared_pti->desiredValue);
                    WriteProcessMemory(hProcess, reinterpret_cast<LPVOID>(targetAddr), &value, sizeof(int), nullptr);
                }
                else if (shared_pti->desiredValue.type() == typeid(float))
                {
                    float value = std::any_cast<float>(shared_pti->desiredValue);
                    WriteProcessMemory(hProcess, reinterpret_cast<LPVOID>(targetAddr), &value, sizeof(float), nullptr);
                }
                else if (shared_pti->desiredValue.type() == typeid(double))
                {
                    double value = std::any_cast<double>(shared_pti->desiredValue);
                    WriteProcessMemory(hProcess, reinterpret_cast<LPVOID>(targetAddr), &value, sizeof(double), nullptr);
                }
                // Add more types as needed
                else
                {
                    std::cerr << "[!] Unsupported type for toggle '" << shared_pti->toggleName << "'.\n";
                }

                // Flush the instruction cache with the correct size
                FlushInstructionCache(hProcess, reinterpret_cast<LPCVOID>(targetAddr), shared_pti->valueSize);

                // Sleep for 100 milliseconds
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
        else
        {
            std::cerr << "[!] PointerToggleInfo for toggle has been destroyed.\n";
        } });

        std::cout << "[+] Pointer toggle '" << name << "' activated.\n";
        return true;
    }
};
