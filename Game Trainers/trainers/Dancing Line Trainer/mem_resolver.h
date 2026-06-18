// mem_resolver.h - in-process IL2CPP resolver for the protected Dancing Line build.
// The il2cpp API exports are obfuscated and metadata-touching calls fail off-thread,
// so we resolve everything by direct memory reads of the live runtime structures.
//
// Verified on Unity 2021.3.45f2 / il2cpp metadata v29 (x64):
//   Il2CppImage : name@0x00, nameNoExt@0x08, assembly@0x10, typeCount@0x20(u32), stride 0x48
//   Il2CppClass : image@0x00, name@0x10, namespace@0x18, parent@0x58, fields@0x80,
//                 methods@0x98, method_count@0x11C(u16), field_count@0x120(u16)
//   MethodInfo  : methodPointer@0x00, name@0x18, klass@0x20, parameters_count@0x52(u8)
//   FieldInfo   : name@0x00, type@0x08, parent@0x10, offset@0x18(u32), stride 0x20
#pragma once

#include <windows.h>
#include <psapi.h>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <unordered_map>

namespace MemRes
{
    // ---- struct offsets ----
    constexpr uintptr_t IMG_NAME = 0x00, IMG_NAMENOEXT = 0x08, IMG_TYPECOUNT = 0x20, IMG_STRIDE = 0x48;
    constexpr uintptr_t CLS_IMAGE = 0x00, CLS_NAME = 0x10, CLS_NS = 0x18, CLS_PARENT = 0x58,
                        CLS_FIELDS = 0x80, CLS_METHODS = 0x98, CLS_STATIC_FIELDS = 0xB8,
                        CLS_METHOD_COUNT = 0x11C, CLS_FIELD_COUNT = 0x120;
    constexpr uintptr_t MI_PTR = 0x00, MI_NAME = 0x18, MI_KLASS = 0x20, MI_PARAMS = 0x30, MI_PARAMCOUNT = 0x52;
    constexpr uintptr_t FI_NAME = 0x00, FI_OFFSET = 0x18, FI_STRIDE = 0x20;
    constexpr int IL2CPP_TYPE_I4 = 0x08; // Il2CppTypeEnum

    inline uintptr_t g_gaBase = 0, g_gaEnd = 0;
    inline void *g_asmImage = nullptr;            // primary (Assembly-CSharp)
    inline std::vector<void *> g_images;          // game images scanned for classes
    // full key "namespace|name" -> klass ; also bare "name" -> klass (first wins)
    inline std::unordered_map<std::string, void *> g_classes;

    inline bool Readable(const void *p, size_t n = 1)
    {
        if (!p) return false;
        MEMORY_BASIC_INFORMATION mbi{};
        if (!VirtualQuery(p, &mbi, sizeof(mbi))) return false;
        if (mbi.State != MEM_COMMIT || (mbi.Protect & PAGE_GUARD)) return false;
        DWORD pr = mbi.Protect & 0xff;
        if (!(pr == PAGE_READONLY || pr == PAGE_READWRITE || pr == PAGE_WRITECOPY ||
              pr == PAGE_EXECUTE_READ || pr == PAGE_EXECUTE_READWRITE || pr == PAGE_EXECUTE_WRITECOPY))
            return false;
        return (uintptr_t)p + n <= (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
    }

    template <class T> inline T Read(uintptr_t a) { T v{}; if (Readable((void *)a, sizeof(T))) std::memcpy(&v, (void *)a, sizeof(T)); return v; }
    inline void *Ptr(uintptr_t a) { return Read<void *>(a); }

    inline bool IsPlainName(const char *s, int maxLen = 200)
    {
        if (!Readable(s, 1) || !*s) return false;
        for (int i = 0; i < maxLen; ++i)
        {
            if (!Readable(s + i, 1)) return false;
            unsigned char c = (unsigned char)s[i];
            if (c == 0) return i > 0;
            if (c < 0x20 || c > 0x7e) return false;
        }
        return false;
    }
    inline bool InGameAssembly(void *p)
    {
        auto a = (uintptr_t)p;
        return g_gaBase && a >= g_gaBase && a < g_gaEnd;
    }

    inline bool InitModuleRange()
    {
        HMODULE m = GetModuleHandleA("GameAssembly.dll");
        if (!m) return false;
        MODULEINFO mi{};
        if (!GetModuleInformation(GetCurrentProcess(), m, &mi, sizeof(mi))) return false;
        g_gaBase = (uintptr_t)mi.lpBaseOfDll;
        g_gaEnd = g_gaBase + mi.SizeOfImage;
        return true;
    }

    // scan committed private memory for an 8-byte value; invoke cb(addressOfMatch)
    template <class Cb>
    inline void ScanQword(uint64_t needle, Cb cb)
    {
        SYSTEM_INFO si{}; GetSystemInfo(&si);
        uintptr_t a = (uintptr_t)si.lpMinimumApplicationAddress, maxA = (uintptr_t)si.lpMaximumApplicationAddress;
        std::vector<uint8_t> buf(4 * 1024 * 1024 + 8);
        HANDLE proc = GetCurrentProcess();
        while (a < maxA)
        {
            MEMORY_BASIC_INFORMATION mbi{};
            if (!VirtualQuery((void *)a, &mbi, sizeof(mbi))) break;
            uintptr_t next = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
            DWORD pr = mbi.Protect & 0xff;
            bool ok = mbi.State == MEM_COMMIT && mbi.Type != MEM_IMAGE && !(mbi.Protect & PAGE_GUARD) &&
                      (pr == PAGE_READWRITE || pr == PAGE_READONLY || pr == PAGE_WRITECOPY);
            if (ok)
            {
                uintptr_t base = (uintptr_t)mbi.BaseAddress; size_t rem = mbi.RegionSize, off = 0;
                while (rem >= 8)
                {
                    size_t toRead = rem < buf.size() - 8 ? rem : buf.size() - 8;
                    SIZE_T got = 0;
                    if (ReadProcessMemory(proc, (void *)(base + off), buf.data(), toRead, &got) && got >= 8)
                        for (size_t i = 0; i + 8 <= got; i += 8)
                        {
                            uint64_t v; std::memcpy(&v, buf.data() + i, 8);
                            if (v == needle) cb(base + off + i);
                        }
                    if (got < toRead) break;
                    off += toRead; rem -= toRead;
                }
            }
            if (next <= a) break;
            a = next;
        }
    }

    // Fast path: il2cpp_get_corlib is exported (renamed). Calling it returns the corlib
    // Il2CppImage* (a plain global) and works even off-thread. Images are a contiguous
    // array (stride 0x48) with plaintext names; walk to find Assembly-CSharp.
    // The export name is build-specific; the string-scan fallback covers updates.
    inline void *TryCorlibWalk(const char *corlibExport, const char *targetDll = "Assembly-CSharp.dll")
    {
        HMODULE ga = GetModuleHandleA("GameAssembly.dll");
        if (!ga) return nullptr;
        auto fn = (void *(*)())GetProcAddress(ga, corlibExport);
        if (!fn) return nullptr;
        void *corlib = fn(); // get_corlib just returns a global; safe to call
        if (!Readable(corlib, IMG_STRIDE)) return nullptr;
        const char *cn = (const char *)Read<void *>((uintptr_t)corlib + IMG_NAME);
        if (!IsPlainName(cn)) return nullptr; // sanity: corlib has a plaintext name
        for (int k = -80; k < 300; ++k)
        {
            void *img = (void *)((uintptr_t)corlib + (intptr_t)k * IMG_STRIDE);
            if (!Readable(img, IMG_STRIDE)) continue;
            const char *in = (const char *)Read<void *>((uintptr_t)img + IMG_NAME);
            if (IsPlainName(in) && std::strcmp(in, targetDll) == 0) return img;
        }
        return nullptr;
    }

    // find the Assembly-CSharp Il2CppImage by locating its plaintext name then the struct
    inline void *FindAssemblyImage(const char *dllName = "Assembly-CSharp.dll", const char *noExt = "Assembly-CSharp")
    {
        // locate the name string (collect several candidates)
        std::vector<uintptr_t> strHits;
        size_t nlen = std::strlen(dllName) + 1;
        {
            SYSTEM_INFO si{}; GetSystemInfo(&si);
            uintptr_t a = (uintptr_t)si.lpMinimumApplicationAddress, maxA = (uintptr_t)si.lpMaximumApplicationAddress;
            std::vector<uint8_t> buf(4 * 1024 * 1024 + 64); HANDLE proc = GetCurrentProcess();
            while (a < maxA && strHits.size() < 64)
            {
                MEMORY_BASIC_INFORMATION mbi{};
                if (!VirtualQuery((void *)a, &mbi, sizeof(mbi))) break;
                uintptr_t next = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
                DWORD pr = mbi.Protect & 0xff;
                if (mbi.State == MEM_COMMIT && mbi.Type != MEM_IMAGE && !(mbi.Protect & PAGE_GUARD) &&
                    (pr == PAGE_READONLY || pr == PAGE_READWRITE || pr == PAGE_WRITECOPY))
                {
                    uintptr_t base = (uintptr_t)mbi.BaseAddress; size_t rem = mbi.RegionSize, off = 0;
                    while (rem >= nlen)
                    {
                        size_t toRead = rem < buf.size() - 64 ? rem : buf.size() - 64;
                        SIZE_T got = 0;
                        if (ReadProcessMemory(proc, (void *)(base + off), buf.data(), toRead, &got) && got >= nlen)
                            for (size_t i = 0; i + nlen <= got; ++i)
                                if (buf[i] == (uint8_t)dllName[0] && std::memcmp(buf.data() + i, dllName, nlen) == 0)
                                    strHits.push_back(base + off + i);
                        if (got < toRead) break;
                        off += toRead; rem -= toRead;
                    }
                }
                if (next <= a) break;
                a = next;
            }
        }
        if (strHits.empty()) return nullptr;
        // single pass: a qword equal to any string-hit, whose nameNoExt validates, is the image
        void *found = nullptr;
        SYSTEM_INFO si{}; GetSystemInfo(&si);
        uintptr_t a = (uintptr_t)si.lpMinimumApplicationAddress, maxA = (uintptr_t)si.lpMaximumApplicationAddress;
        std::vector<uint8_t> buf(4 * 1024 * 1024 + 8); HANDLE proc = GetCurrentProcess();
        while (a < maxA && !found)
        {
            MEMORY_BASIC_INFORMATION mbi{};
            if (!VirtualQuery((void *)a, &mbi, sizeof(mbi))) break;
            uintptr_t next = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
            DWORD pr = mbi.Protect & 0xff;
            if (mbi.State == MEM_COMMIT && mbi.Type != MEM_IMAGE && !(mbi.Protect & PAGE_GUARD) &&
                (pr == PAGE_READWRITE || pr == PAGE_READONLY || pr == PAGE_WRITECOPY))
            {
                uintptr_t base = (uintptr_t)mbi.BaseAddress; size_t rem = mbi.RegionSize, off = 0;
                while (rem >= 8 && !found)
                {
                    size_t toRead = rem < buf.size() - 8 ? rem : buf.size() - 8;
                    SIZE_T got = 0;
                    if (ReadProcessMemory(proc, (void *)(base + off), buf.data(), toRead, &got) && got >= 8)
                        for (size_t i = 0; i + 8 <= got; i += 8)
                        {
                            uint64_t v; std::memcpy(&v, buf.data() + i, 8);
                            for (uintptr_t s : strHits)
                                if (v == (uint64_t)s)
                                {
                                    uintptr_t img = base + off + i;
                                    const char *ne = (const char *)Read<void *>(img + IMG_NAMENOEXT);
                                    if (IsPlainName(ne) && std::strcmp(ne, noExt) == 0) { found = (void *)img; }
                                    break;
                                }
                            if (found) break;
                        }
                    if (got < toRead) break;
                    off += toRead; rem -= toRead;
                }
            }
            if (next <= a) break;
            a = next;
        }
        return found;
    }

    inline std::string Key(const char *ns, const char *name)
    {
        return std::string(ns ? ns : "") + "|" + (name ? name : "");
    }

    // scan for all classes whose image == one of g_images; populate g_classes
    inline int BuildClassMap()
    {
        g_classes.clear();
        int count = 0;
        for (void *image : g_images)
        {
            if (!image) continue;
            ScanQword((uint64_t)(uintptr_t)image, [&](uintptr_t at) {
                // 'at' is where the image ptr sits; klass = at (image is field 0)
                uintptr_t klass = at;
                const char *name = (const char *)Read<void *>(klass + CLS_NAME);
                const char *ns = (const char *)Read<void *>(klass + CLS_NS);
                if (!IsPlainName(name)) return;
                std::string nsS = IsPlainName(ns) ? ns : "";
                g_classes.emplace(Key(nsS.c_str(), name), (void *)klass); // first writer wins
                g_classes.emplace(Key("", name), (void *)klass);          // bare-name fallback
                ++count;
            });
        }
        return count;
    }

    // corlibExport: build-specific renamed il2cpp_get_corlib (fast path); may be null.
    // Scans both Assembly-CSharp.dll and Assembly-CSharp-firstpass.dll (eint lives in firstpass).
    inline bool Init(const char *corlibExport = "VvVyBTZChtV")
    {
        if (!InitModuleRange()) return false;
        g_images.clear();
        void *a = corlibExport ? TryCorlibWalk(corlibExport, "Assembly-CSharp.dll") : nullptr;
        if (!a) a = FindAssemblyImage("Assembly-CSharp.dll", "Assembly-CSharp");
        void *b = corlibExport ? TryCorlibWalk(corlibExport, "Assembly-CSharp-firstpass.dll") : nullptr;
        if (!b) b = FindAssemblyImage("Assembly-CSharp-firstpass.dll", "Assembly-CSharp-firstpass");
        g_asmImage = a;
        if (a) g_images.push_back(a);
        if (b) g_images.push_back(b);
        if (g_images.empty()) return false;
        return BuildClassMap() > 0;
    }

    // Singleton instance from a static field: scan the static_fields of the class AND its
    // bases for a slot holding an object whose klass == the leaf class. Walking the hierarchy
    // matters for MonoSingleton<T>-style bases, where the `instance` field lives on the base
    // (its own generic-instantiation static storage). Reliable (small, class-specific regions).
    inline void *GetSingleton(void *leafKlass)
    {
        if (!leafKlass) return nullptr;
        void *klass = leafKlass;
        while (Readable(klass, 0x130))
        {
            void *sf = Read<void *>((uintptr_t)klass + CLS_STATIC_FIELDS);
            if (Readable(sf, 8))
                for (int i = 0; i < 256; ++i)
                {
                    void *cand = Read<void *>((uintptr_t)sf + (uintptr_t)i * 8);
                    if (Readable(cand, 0x10) && Read<void *>((uintptr_t)cand) == leafKlass)
                        return cand;
                }
            klass = Read<void *>((uintptr_t)klass + CLS_PARENT);
        }
        return nullptr;
    }

    // type enum of parameter `idx` of a MethodInfo (MI_PARAMS -> array of Il2CppType*)
    inline int ParamTypeEnum(void *mi, int idx)
    {
        void *params = Read<void *>((uintptr_t)mi + MI_PARAMS);
        if (!Readable(params, (size_t)(idx + 1) * 8)) return -1;
        void *t = Read<void *>((uintptr_t)params + (uintptr_t)idx * 8);
        if (!Readable(t, 0x10)) return -1;
        return (Read<uint32_t>((uintptr_t)t + 8) >> 16) & 0xff;
    }

    // FindMethodInfo, additionally requiring parameter 0 to have a given type enum
    // (used to pick op_Implicit(int)->eint among the op_Implicit overloads).
    inline void *FindMethodInfoP0(void *klass, const char *name, int paramCount, int p0type)
    {
        while (Readable(klass, 0x130))
        {
            void **methods = (void **)Read<void *>((uintptr_t)klass + CLS_METHODS);
            uint16_t mc = Read<uint16_t>((uintptr_t)klass + CLS_METHOD_COUNT);
            if (methods && mc && mc < 4096 && Readable(methods, (size_t)mc * 8))
                for (int i = 0; i < mc; ++i)
                {
                    void *mi = methods[i];
                    if (!Readable(mi, 0x58)) continue;
                    const char *mn = (const char *)Read<void *>((uintptr_t)mi + MI_NAME);
                    if (!IsPlainName(mn) || std::strcmp(mn, name) != 0) continue;
                    if (paramCount >= 0 && Read<uint8_t>((uintptr_t)mi + MI_PARAMCOUNT) != paramCount) continue;
                    if (ParamTypeEnum(mi, 0) != p0type) continue;
                    if (InGameAssembly(Read<void *>((uintptr_t)mi + MI_PTR))) return mi;
                }
            klass = Read<void *>((uintptr_t)klass + CLS_PARENT);
        }
        return nullptr;
    }

    inline void *GetClass(const char *ns, const char *name)
    {
        if (auto it = g_classes.find(Key(ns, name)); it != g_classes.end()) return it->second;
        if (auto it = g_classes.find(Key("", name)); it != g_classes.end()) return it->second;
        return nullptr;
    }

    // method pointer by name (+ optional exact paramcount; -1 = any), walking parents
    inline void *FindMethodPtr(void *klass, const char *name, int paramCount = -1)
    {
        while (Readable(klass, 0x130))
        {
            void **methods = (void **)Read<void *>((uintptr_t)klass + CLS_METHODS);
            uint16_t mc = Read<uint16_t>((uintptr_t)klass + CLS_METHOD_COUNT);
            if (methods && mc && mc < 4096 && Readable(methods, (size_t)mc * 8))
                for (int i = 0; i < mc; ++i)
                {
                    void *mi = methods[i];
                    if (!Readable(mi, 0x58)) continue;
                    const char *mn = (const char *)Read<void *>((uintptr_t)mi + MI_NAME);
                    if (!IsPlainName(mn) || std::strcmp(mn, name) != 0) continue;
                    if (paramCount >= 0 && Read<uint8_t>((uintptr_t)mi + MI_PARAMCOUNT) != paramCount) continue;
                    void *ptr = Read<void *>((uintptr_t)mi + MI_PTR);
                    if (InGameAssembly(ptr)) return ptr;
                }
            klass = Read<void *>((uintptr_t)klass + CLS_PARENT);
        }
        return nullptr;
    }

    // raw MethodInfo* (needed as the trailing arg for il2cpp method thunks)
    inline void *FindMethodInfo(void *klass, const char *name, int paramCount = -1)
    {
        while (Readable(klass, 0x130))
        {
            void **methods = (void **)Read<void *>((uintptr_t)klass + CLS_METHODS);
            uint16_t mc = Read<uint16_t>((uintptr_t)klass + CLS_METHOD_COUNT);
            if (methods && mc && mc < 4096 && Readable(methods, (size_t)mc * 8))
                for (int i = 0; i < mc; ++i)
                {
                    void *mi = methods[i];
                    if (!Readable(mi, 0x58)) continue;
                    const char *mn = (const char *)Read<void *>((uintptr_t)mi + MI_NAME);
                    if (!IsPlainName(mn) || std::strcmp(mn, name) != 0) continue;
                    if (paramCount >= 0 && Read<uint8_t>((uintptr_t)mi + MI_PARAMCOUNT) != paramCount) continue;
                    if (InGameAssembly(Read<void *>((uintptr_t)mi + MI_PTR))) return mi;
                }
            klass = Read<void *>((uintptr_t)klass + CLS_PARENT);
        }
        return nullptr;
    }

    inline int FindFieldOffset(void *klass, const char *name)
    {
        while (Readable(klass, 0x130))
        {
            uintptr_t fields = (uintptr_t)Read<void *>((uintptr_t)klass + CLS_FIELDS);
            uint16_t fc = Read<uint16_t>((uintptr_t)klass + CLS_FIELD_COUNT);
            if (fields && fc && fc < 4096)
                for (int i = 0; i < fc; ++i)
                {
                    uintptr_t fi = fields + (uintptr_t)i * FI_STRIDE;
                    const char *fn = (const char *)Read<void *>(fi + FI_NAME);
                    if (IsPlainName(fn) && std::strcmp(fn, name) == 0)
                        return (int)Read<uint32_t>(fi + FI_OFFSET);
                }
            klass = Read<void *>((uintptr_t)klass + CLS_PARENT);
        }
        return -1;
    }
}
