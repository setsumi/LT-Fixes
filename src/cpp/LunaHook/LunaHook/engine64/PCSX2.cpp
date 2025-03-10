﻿#include "PCSX2.h"
#include "JIT_Keeper.hpp"
namespace GameList
{
    enum class EntryType
    {
        PS2Disc,
        // ……
    };

    enum class Region
    {
        NTSC_B,
        // ……
    };
    struct Entry
    {
        EntryType type = EntryType::PS2Disc;
        Region region; //= Region::Other;

        std::string path;
        std::string serial;
        std::string title;
        std::string title_sort;
        std::string title_en;
        // ……
    };
}
namespace
{
    std::string current_serial;
    using namespace PCSX2Types;
    // BASEBLOCKEX* BaseBlocks::New(u32 startpc, uptr fnptr)->void __fastcall BaseBlocks::New(BaseBlocks *this, unsigned int startpc, unsigned __int64 fnptr)
    // static void iopRecRecompile(u32 startpc);
    // static void recRecompile(const u32 startpc);-> void __fastcall recRecompile(VMManager::Internal *startpc)
    // void CBreakPoints::AddBreakPoint(BreakPointCpu cpu, u32 addr, bool temp, bool enabled)-》 void __fastcall CBreakPoints::AddBreakPoint(BreakPointCpu cpu, unsigned int addr, unsigned __int8 a3, bool a4)
    // void MainWindow::startGameListEntry(const GameList::Entry* entry, std::optional<s32> save_slot, std::optional<bool> fast_boot)
    uint64_t BaseBlocksNew, recRecompile, iopRecRecompile;
    uint64_t dynarecCheckBreakpoint, psxDynarecCheckBreakpoint; // 用于防止AddBreakPoint使游戏卡住
    uint64_t startGameListEntry;
    typedef void(__fastcall *tAddBreadPoint)(BreakPointCpu cpu, u32 addr, bool temp, bool enabled);
    tAddBreadPoint AddBreakPoint = nullptr;

    constexpr BYTE sig_dynarecCheckBreakpoint[] = {
        0x48, 0x83, 0xec, XX,
        0x8b, 0x35, XX4,
        0x8b, 0x05, XX4,
        0x48, 0x39, 0x05, XX4,
        0x75, 0x0d,
        0x83, 0x3d, XX4, 0x00,
        0x0f, 0x85, XX4,
        0xb9, 0x01, 0x00, 0x00, 0x00,
        0x89, 0xf2};
    constexpr BYTE sig_BaseBlocksNew[] = {
        0x41, 0x57, 0x41, 0x56, 0x41, 0x55, 0x41, 0x54, 0x56, 0x57, 0x55, 0x53,
        0x48, 0x83, 0xEC, 0x28,
        0x4c, 0x89, 0xc7,
        0x89, 0xd3,
        0x48, 0x89, 0xce,
        0x48, 0x8b, 0x01,
        0x48, 0x8d, 0x50, 0x08,
        0x4c, 0x8b, 0x40, 0x08,
        0x41, 0x80, 0x78, 0x19, 0x00};
    constexpr BYTE sig_recRecompile[] = {
        0x89, 0xcd,
        0x48, 0x8b, 0x05, XX4,
        0x48, 0x3b, 0x05, XX4,
        0x72, 0x07,
        0xc6, 0x05, XX4, 0x01,
        XX, 0x89, XX,
        0x41, 0xc1, XX, 0x10,
        0x4c, 0x8d, XX, XX4};
    constexpr BYTE sig_AddBreakPoint[] = {
        0x89, 0xd0,
        0x25, 0x00, 0x00, 0x00, 0x60,
        0x3d, 0x00, 0x00, 0x00, 0x20,
        0xb8, 0xff, 0xff, 0xff, 0x0f,
        0xb9, 0xff, 0xff, 0xff, 0x7f};
    BYTE sig_OpCodeImpl_CACHE[] = {
        0x48, 0x83, 0xec, XX,
        0x8b, 0x05, XX4,
        0x89, 0xc2,
        0xc1, 0xea, 0x10,
        0x83, 0xe2, 0x1f,
        0x8d, 0x4a, 0xf9,
        0x83, 0xf9, 0x15,
        0x0f, 0x87, XX4};
#define FINDALIGN(X)                                                     \
    X = (decltype(X))MemDbg::findEnclosingAlignedFunction((uintptr_t)X); \
    if (!X)                                                              \
        return false;
#define FINDFUNCTION(X)                                                                                    \
    X = (decltype(X))MemDbg::findBytes(sig_##X, sizeof(sig_##X), processStartAddress, processStopAddress); \
    if (!X)                                                                                                \
        return false;
    bool findstartGameListEntry()
    {
        char fmtstr[] = "This save state does not exist.";
        auto fmtstrptr = MemDbg::findBytes(fmtstr, sizeof(fmtstr), processStartAddress, processStopAddress);
        if (!fmtstrptr)
            return false;
        fmtstrptr = MemDbg::findleaaddr(fmtstrptr, processStartAddress, processStopAddress);
        if (!fmtstrptr)
            return false;
        startGameListEntry = MemDbg::findEnclosingAlignedFunction(fmtstrptr);
        return startGameListEntry;
    }
    bool hookFunctions()
    {
        if (!findstartGameListEntry())
            return false;
        FINDFUNCTION(dynarecCheckBreakpoint);
        FINDALIGN(dynarecCheckBreakpoint);
        FINDFUNCTION(BaseBlocksNew);
        FINDFUNCTION(recRecompile);
        FINDALIGN(recRecompile);
        FINDFUNCTION(AddBreakPoint);
        FINDALIGN(AddBreakPoint);
        uint64_t OpCodeImpl_CACHE;
        FINDFUNCTION(OpCodeImpl_CACHE);
        _cpuRegistersPack = (decltype(_cpuRegistersPack))(OpCodeImpl_CACHE + 4 + 6 + *(int *)(OpCodeImpl_CACHE + 4 + 2) - 0x2ac);
        auto EEmem = GetProcAddress(GetModuleHandle(NULL), "EEmem");
        if (!EEmem)
            return false;
        eeMem = *(decltype(eeMem) *)EEmem;
        ConsoleOutput("eeMem %p", eeMem);
        return true;
    }
    void SafeAddBreakPoint(u32 addr, bool enable = true)
    {
        __try
        {
            AddBreakPoint(BREAKPOINT_EE, addr, false, enable);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }
    struct emfuncinfo
    {
        uint64_t type;
        int offset;
        int padding;
        decltype(HookParam::text_fun) hookfunc;
        decltype(HookParam::filter_fun) filterfun;
        const char *_id;
    };
    std::unordered_map<uintptr_t, emfuncinfo> emfunctionhooks;

    void CheckForHook(uint64_t em_address, uintptr_t entrypoint)
    {
        if (emfunctionhooks.find(em_address) == emfunctionhooks.end())
            return;
        auto op = emfunctionhooks.at(em_address);
        if (current_serial.size() && (current_serial != op._id))
            return;
        HookParam hpinternal;
        hpinternal.address = entrypoint;
        hpinternal.emu_addr = em_address; // 用于生成hcode
        hpinternal.type = NO_CONTEXT | BREAK_POINT | op.type;
        if (!(op.type & USING_CHAR))
            hpinternal.type |= USING_STRING;
        hpinternal.codepage = 932;
        hpinternal.text_fun = op.hookfunc;
        hpinternal.filter_fun = op.filterfun;
        hpinternal.offset = op.offset;
        hpinternal.padding = op.padding;
        hpinternal.jittype = JITTYPE::PCSX2;
        NewHook(hpinternal, op._id);
    }
    std::set<uint32_t> recRecompileReady;
}
bool PCSX2_UserHook_delayinsert(uint32_t addr)
{
    if (recRecompileReady.find(addr) == recRecompileReady.end())
    {
        SafeAddBreakPoint(addr);
        return true;
    }
    return false;
}
bool PCSX2::attach_function()
{
    if (!hookFunctions())
        return false;
    bool succ = true;
    {
        HookParam hp;
        hp.address = BaseBlocksNew;
        hp.text_fun = [](hook_context *context, HookParam *hp, TextBuffer *buffer, uintptr_t *split)
        {
            unsigned int startpc = context->argof(2);
            uint64_t fnptr = context->argof(3);
            jitaddraddr(startpc, fnptr, JITTYPE::PCSX2);
        };
        succ = succ && NewHook(hp, "BaseBlocks::New");
    }
    {
        HookParam hp;
        hp.address = startGameListEntry;
        hp.text_fun = [](hook_context *context, HookParam *hp, TextBuffer *buffer, uintptr_t *split)
        {
            const GameList::Entry *entry = (GameList::Entry *)context->rdx;
            current_serial = entry->serial;
            jitaddrclear();
            for (auto &&[addr, op] : emfunctionhooks)
            {
                if ((op._id == current_serial) && (op.type & DIRECT_READ))
                {
                    HookParam hpinternal;
                    hpinternal.address = emu_addr(addr);
                    hpinternal.emu_addr = addr;
                    hpinternal.jittype = JITTYPE::PCSX2;
                    hpinternal.type = op.type;
                    hpinternal.codepage = 932;
                    hpinternal.text_fun = op.hookfunc;
                    hpinternal.filter_fun = op.filterfun;
                    hpinternal.offset = op.offset;
                    hpinternal.padding = op.padding;
                    NewHook(hpinternal, op._id);
                }
            }
            return HostInfo(HOSTINFO::EmuGameName, "%s %s", entry->serial.c_str(), entry->title.c_str());
        };
        succ = succ && NewHook(hp, "startGameListEntry");
    }
    auto hookrecompile = [&](auto addr, auto name)
    {
        HookParam hp;
        hp.address = addr;
        hp.user_value = (uintptr_t)new uintptr_t;
        hp.text_fun = [](hook_context *context, HookParam *hp, auto *, auto *)
        {
            unsigned int startpc = context->argof(1);
            *(uintptr_t *)(hp->user_value) = startpc;

            if (*(BYTE *)(context->retaddr) != 0xe9) // 会被谜之取消
            {
                HookParam hpinternal;
                hpinternal.user_value = hp->user_value;
                hpinternal.address = context->retaddr;
                hpinternal.text_fun = [](hook_context *context, HookParam *hp, auto *, auto *)
                {
                    unsigned int startpc = *(uintptr_t *)(hp->user_value);
                    std::lock_guard _(maplock);
                    if (emuaddr2jitaddr.find(startpc) == emuaddr2jitaddr.end())
                        return;
                    recRecompileReady.insert(startpc);
                    auto fnptr = emuaddr2jitaddr[startpc].second;
                    CheckForHook(startpc, fnptr);
                    delayinsertNewHook(startpc);
                };
                NewHook(hpinternal, "Ret");
            }
        };
        succ = succ && NewHook(hp, name);
    };
    if (!succ)
        return false;
    dont_detach = true; // 不可以detach，detach后会导致游戏卡住，可能是dynarecCheckBreakpoint被恢复的原因。
    hookrecompile(recRecompile, "recRecompile");
    // hookrecompile(iopRecRecompile, "iopRecRecompile");
    patch_fun_ptrs = {
        {(void *)dynarecCheckBreakpoint, +[]() {}},
        //{(void *)psxDynarecCheckBreakpoint, +[]() {}},
    };
    patch_fun_ptrs_patch_once();
    for (auto &&[addr, op] : emfunctionhooks)
    {
        if (!(op.type & DIRECT_READ))
            SafeAddBreakPoint(addr);
    }
    return true;
}

namespace
{
    void FSLPM66136(TextBuffer *buffer, HookParam *hp)
    {
        auto s = buffer->strA();
        strReplace(s, "#cr0");
        s = re::sub(s, R"((\\n)+)", " ");
        s = re::sub(s, R"(\\d$|^\@[a-z]+|#.*?#|\$)");
        s = re::sub(s, R"(@w|\\c)");
        buffer->from(s);
    }
    void FSLPM65914(TextBuffer *buffer, HookParam *hp)
    {
        auto s = buffer->strAW();
        strReplace(s, L"①ｎ");
        if (startWith(s, L"Ｖ＿") || std::all_of(s.begin(), s.end(), [](wchar_t c)
                                                 { return (c == L'±') || (c == L'-') || (c == L'\\') || (c == L'．') || (c == L'，') || (c == L'－') || (c == L'　') || (c == L' ') || ((c >= L'０') && (c <= L'９')) || ((c >= L'0') && (c <= L'9')); }))
            return buffer->clear();
        s = re::sub(s, LR"(^([±\s]+[０-９０-９，．\-]+|[０-９0-9，．\-]+)$)");
        buffer->fromWA(s);
    }
    void SLPM55227(TextBuffer *buffer, HookParam *hp)
    {
        auto s = buffer->strAW();
        s = re::sub(s, LR"(@(.*?)@)", L"$1");
        strReplace(s, L"%n");
        buffer->fromWA(s);
    }
    void FSLPM66045(TextBuffer *buffer, HookParam *hp)
    {
        auto s = buffer->strA();
        s = re::sub(s, R"(%\w+\d*\w*)", " ");
        s = re::sub(s, R"(%N+)");
        buffer->from(s);
    }
    void FSLPM66302(TextBuffer *buffer, HookParam *hp)
    {
        auto s = buffer->strAW();
        s = strSplit(s, L"\");")[0];
        s = re::sub(s, LR"((\\n)+)", L" ");
        s = re::sub(s, LR"(\\d$|^\@[a-z]+|#.*?#|\$)");
        s = re::sub(s, LR"(@w|\\c)");
        strReplace(s, L"＊Ａ", L"岡崎");
        strReplace(s, L"＊Ｂ", L"朋也");
        buffer->fromWA(s);
    }
    void FSLPS25677(TextBuffer *buffer, HookParam *hp)
    {
        auto s = buffer->strAW();
        s = re::sub(s, LR"(^\[.*$)");
        s = re::sub(s, LR"(\_0C|\_1_5C|\_1C)");
        s = re::sub(s, LR"(^([a-zA-Z]+)|([a-zA-Z]+)$)");
        buffer->fromWA(s);
    }
    void SLPM66272(TextBuffer *buffer, HookParam *hp)
    {
        auto ss = buffer->strA();
        auto s = buffer->strAW();
        if (WideStringToString(s, 932) != ss)
            return buffer->clear();
        strReplace(s, L"#");
        strReplace(s, L"ﾞ");
        strReplace(s, L"!");
        buffer->fromWA(s);
        if (buffer->strAW().size() <= 4)
            buffer->clear();
    }
    void SLPM65396(TextBuffer *buffer, HookParam *hp)
    {
        CharFilter(buffer, '\n');
    }
    void SLPM55014(TextBuffer *buffer, HookParam *hp)
    {
        StringFilter(buffer, TEXTANDLEN("%FS"));
        StringFilter(buffer, TEXTANDLEN("%FE"));
        StringFilter(buffer, TEXTANDLEN("%LC"));
        StringFilter(buffer, TEXTANDLEN("%N"));
    }
    void SLPM55170(TextBuffer *buffer, HookParam *hp)
    {
        StringFilter(buffer, TEXTANDLEN("%P"));
        StringFilter(buffer, TEXTANDLEN("%K"));
    }
    void SLPM62343(TextBuffer *buffer, HookParam *hp)
    {
        CharFilter(buffer, '\n');
        StringFilter(buffer, TEXTANDLEN("/K"));
        StringFilter(buffer, TEXTANDLEN("/L"));
        StringFilter(buffer, TEXTANDLEN("\x81\x40"));
    }
    void SLPL25871(TextBuffer *buffer, HookParam *hp)
    {
        if (buffer->size <= 4)
            return buffer->clear();
        CharFilter(buffer, '\n');
        StringFilter(buffer, TEXTANDLEN("\x81\x40"));
    }
    void FSLPS25547(TextBuffer *buffer, HookParam *hp)
    {
        CharFilter(buffer, '\n');
        FSLPS25677(buffer, hp);
    }
    void SLPS25809(TextBuffer *buffer, HookParam *hp)
    {
        StringFilter(buffer, TEXTANDLEN("/K"));
        CharFilter(buffer, '\n');
        auto s = buffer->strA();
        static std::string last;
        if (last == s)
            return buffer->clear();
        last = s;
    }
    template <int i>
    void SLPS25897(TextBuffer *buffer, HookParam *hp)
    {
        CharFilter(buffer, '\n');
        auto s = buffer->strA();
        static std::string last;
        if (startWith(s, last))
        {
            buffer->from(s.substr(last.size()));
        }
        last = s;
    }
    void FSLPM66332(TextBuffer *buffer, HookParam *hp)
    {
        CharFilter(buffer, '\x01');
        SLPS25897<3>(buffer, hp);
    }
    void SLPS25897_1(TextBuffer *buffer, HookParam *hp)
    {
        CharFilter(buffer, '\n');
        auto s = buffer->strA();
        static std::string last;
        if (last.size() && startWith(s, last))
        {
            buffer->from(s.substr(last.size()));
        }
        else
        {
            std::string name = (char *)emu_addr(0x1869C60);
            if (name.size())
                name = "\x81\x79" + name + "\x81\x7a";
            buffer->from(name + buffer->strA());
        }
        last = s;
    }
    void FSLPM55195(TextBuffer *buffer, HookParam *hp)
    {
        StringFilter(buffer, TEXTANDLEN("%n\x81\x40"));
        StringFilter(buffer, TEXTANDLEN("%n"));
    }
    void FSLPM66293(TextBuffer *buffer, HookParam *hp)
    {
        StringFilter(buffer, TEXTANDLEN("#cr0"));
        CharFilter(buffer, '\n');
        std::string name = (char *)emu_addr(0x712260);
        buffer->from(name + buffer->strA());
    }
    void FSLPM65997(TextBuffer *buffer, HookParam *hp)
    {
        auto s = buffer->strA();
        s = re::sub(s, R"(#\w+?\[\d\])");
        strReplace(s, "#n");
        buffer->from(s);
    }
    void SLPM66437(TextBuffer *buffer, HookParam *hp)
    {
        auto s = buffer->strAW();
        strReplace(s, L"d");
        buffer->fromWA(s);
    }
    void SLPM66905(TextBuffer *buffer, HookParam *hp)
    {
        auto s = buffer->strA();
        s = re::sub(s, R"(\x81\x6f(.*?)\x81\x5e(.*?)\x81\x70)", "$2");
        strReplace(s, "\n");
        strReplace(s, "\x81\x40");
        buffer->from(s);
    }
    void SLPS25150(hook_context *context, HookParam *hp1, TextBuffer *buffer, uintptr_t *split)
    {
        static std::string last;
        static std::string lasts[4];
        std::string collect;
        auto addrs = {0x461F38, 0x462000, 0x4620CA, 0x462192};
        for (auto str : addrs)
            collect += (char *)emu_addr(str);
        if (last == collect)
            return;
        last = collect;
        int i = -1;
        collect = "";
        for (auto str : addrs)
        {
            i++;
            std::string x = (char *)emu_addr(str);
            if (i && (lasts[i] == x))
                break;
            lasts[i] = x;
            collect += x;
        }
        static std::string last1;
        if (startWith(collect, last1))
        {
            buffer->from(collect.substr(last1.size()));
        }
        else
        {
            buffer->from(collect);
        }
        last1 = collect;
    }
    template <int a, int b, int c, int d>
    void SLPS20394(hook_context *context, HookParam *hp1, TextBuffer *buffer, uintptr_t *split)
    {
        static std::string last;
        static std::string lasts[4];
        std::string collect;
        auto addrs = {a, b, c, d};
        for (auto str : addrs)
            collect += (char *)emu_addr(str);
        if (last == collect)
            return;
        last = collect;
        int i = -1;
        collect = "";
        for (auto str : addrs)
        {
            i++;
            std::string x = (char *)emu_addr(str);
            if ((x[0] == 'y') || (x[0] == 'u'))
            {
                x = '\x81' + x;
            }
            if (i && (lasts[i] == x))
                break;
            lasts[i] = x;
            if (endWith(x, "\r"))
                x = x.substr(0, x.size() - 1);
            collect += x;
        }
        strReplace(collect, "\x99\xea", "\x98\xa3");
        strReplace(collect, "\x81\x40");
        buffer->from(collect);
    }
    void SLPM66458(TextBuffer *buffer, HookParam *hp)
    {
        StringFilter(buffer, TEXTANDLEN("\\n"));
        CharFilter(buffer, '\n');
        CharFilter(buffer, '\x01');
    }
    void SLPM65843(TextBuffer *buffer, HookParam *hp)
    {
        StringFilter(buffer, TEXTANDLEN("\\n"));
    }
    void SLPS25679(TextBuffer *buffer, HookParam *hp)
    {
        StringReplacer(buffer, TEXTANDLEN("\x87\x4e"), TEXTANDLEN("\x81\x60"));
        StringReplacer(buffer, TEXTANDLEN("\x87\x4f"), TEXTANDLEN("\x81\x60"));
        StringReplacer(buffer, TEXTANDLEN("\x87\x50"), TEXTANDLEN("\x81\x60"));
        StringReplacer(buffer, TEXTANDLEN("\x87\x51"), TEXTANDLEN("\x81\x60"));
        StringReplacer(buffer, TEXTANDLEN("\x87\x52"), TEXTANDLEN("\x81\x60"));
        StringReplacer(buffer, TEXTANDLEN("\x87\x5b"), TEXTANDLEN("\x81\x5b"));

        auto s = buffer->strAW();
        s = re::sub(s, LR"(\[(.*?)\*(.*?)\])", L"$1");
        buffer->fromWA(s);
    }
    auto _ = []()
    {
        emfunctionhooks = {
            // アカイイト
            {0x136800, {0, PCSX2_REG_OFFSET(t0), 0, 0, FSLPM66136, "SLPM-65732"}},
            // Nana
            {0x15036C, {0, PCSX2_REG_OFFSET(a3), 0, 0, FSLPM65914, "SLPM-65914"}},
            // My Merry May with be
            {0x1DB7DC, {0, PCSX2_REG_OFFSET(a3), 0, 0, FSLPM66045, "SLPM-66045"}},
            // CLANNAD - ゲオオンラインストア
            {0x1DB7DC, {0, PCSX2_REG_OFFSET(s4), 0, 0, FSLPM66302, "SLPM-66302"}},
            // 苺ましまろ
            {0x1439F4, {0, PCSX2_REG_OFFSET(s1), 0, 0, FSLPS25547, "SLPS-25547"}},
            // ブラッドプラス ワン ナイト キス
            {0x267B58, {0, PCSX2_REG_OFFSET(a3), 0, 0, FSLPS25677, "SLPS-25677"}},
            {0x268260, {0, PCSX2_REG_OFFSET(a3), 0, 0, FSLPS25677, "SLPS-25677"}},
            // プリンセスラバー！ Eternal Love For My Lady
            {0x92748C, {DIRECT_READ, 0, 0, 0, FSLPM55195, "SLPM-55195"}},
            // 破滅のマルス
            {0x308460, {DIRECT_READ, 0, 0, 0, FSLPM65997, "SLPM-65997"}},
            // フレンズ ～青春の輝き～
            {0x456048, {DIRECT_READ, 0, 0, 0, 0, "SLPS-25385"}},
            // SAMURAI 7
            {0x190FDac, {DIRECT_READ, 0, 0, 0, FSLPM65997, "SLPM-66399"}},
            // 高円寺女子サッカー
            {0x53FA10, {DIRECT_READ | CODEC_UTF8, 0, 0, 0, FSLPM66332, "SLPM-66332"}},
            // 銀魂 銀さんと一緒！ボクのかぶき町日記
            {0x8DA13A, {DIRECT_READ, 0, 0, 0, SLPS25809, "SLPS-25809"}},
            // THE 恋愛ホラーアドベンチャー～漂流少女～
            {0x1A1640, {DIRECT_READ, 0, 0, 0, SLPM62343, "SLPM-62343"}},
            // 好きなものは好きだからしょうがない！！ -FIRST LIMIT & TARGET†NIGHTS- Sukisho！ Episode ＃01+＃02
            {0x268CE9, {DIRECT_READ, 0, 0, SLPS20394<0x268CE9, 0x268D2A, 0x268D6B, 0x268DAC>, 0, "SLPS-20352"}}, //[ディスク 1]
            {0x2690EA, {DIRECT_READ, 0, 0, SLPS20394<0x2690EA, 0x26912A, 0x26916B, 0x2691AC>, 0, "SLPS-20353"}}, //[ディスク 2]
            // 好きなものは好きだからしょうがない！！ -RAIN- Sukisyo！ Episode #03
            {0x2AF161, {DIRECT_READ, 0, 0, SLPS20394<0x2AF161, 0x2AFAA8, 0x2AEFA4, 0x2AEFE5>, 0, "SLPS-20394"}},
            // ドラスティックキラー
            {0x1AC5D40, {DIRECT_READ, 0, 0, 0, SLPL25871, "SLPS-25871"}},
            {0x1AC6970, {DIRECT_READ, 0, 0, 0, SLPL25871, "SLPS-25871"}},
            // 風雨来記
            {0x1FFACA0, {DIRECT_READ, 0, 0, 0, SLPM66458, "SLPM-66458"}},
            // うたわれるもの 散りゆく者への子守唄
            {0x50574C, {DIRECT_READ, 0, 0, 0, SLPS25679, "SLPS-25679"}},
            // Only you リベルクルス ドラマCD付き
            {0x461F38, {DIRECT_READ, 0, 0, SLPS25150, 0, "SLPS-25150"}},
            // D.C. ～ダ・カーポ～ the Origin
            {0x517688, {DIRECT_READ, 0, 0, 0, SLPM66905, "SLPM-66905"}},
            // Soul Link EXTENSION
            {0x1E14A3C, {DIRECT_READ, 0, 0, 0, SLPM66437, "SLPM-66437"}},
            // デ・ジ・キャラット ファンタジー エクセレント
            {0x10E2C80, {DIRECT_READ, 0, 0, 0, SLPM65396, "SLPM-65396"}},
            // I/O
            {0xF0BF80, {DIRECT_READ, 0, 0, 0, SLPM66272, "SLPM-66272"}},
            // シークレット・オブ・エヴァンゲリオン
            {0x842E48, {DIRECT_READ, 0, 0, 0, SLPS25897<2>, "SLPM-66569"}},
            // PANDORA ～君の名前を僕は知る～
            {0x1690C50, {DIRECT_READ, 0, 0, 0, FSLPM65997, "SLPM-55269"}},
            // 学園アリス ～きらきら★メモリーキッス～
            {0x711FC0, {DIRECT_READ, 0, 0, 0, FSLPM66293, "SLPM-66293"}},
            // After...～忘れえぬ絆～
            {0x15DA4c, {DIRECT_READ, 0, 0, 0, SLPS25897<1>, "SLPM-65481"}},
            // ゼロの使い魔 迷子の終止符と幾千の交響曲
            {0x1FFD934, {DIRECT_READ, 0, 0, 0, SLPS25897_1, "SLPS-25897"}},
            // スキップ・ビート
            {0x1CF70F0, {DIRECT_READ, 0, 0, 0, SLPM55170, "SLPM-55170"}},
            // Myself; Yourself それぞれのfinale
            {0x1C785A8, {DIRECT_READ, 0, 0, 0, 0, "SLPM-55163"}},
            // ARIA The ORIGINATION ～蒼い惑星のエルシエロ～
            {0x10F8488, {DIRECT_READ, 0, 0, 0, SLPM55014, "SLPM-55014"}},
            // ARIA The NATURAL ～遠い記憶のミラージュ～
            {0x1137428, {DIRECT_READ, 0, 0, 0, 0, "SLPM-66536"}},
            // 猛獣使いと王子様
            {0x16681E0, {DIRECT_READ, 0, 0, 0, FSLPM65997, "SLPM-55264"}},
            // 120円の春
            {0x1CEAF56, {DIRECT_READ, 0, 0, 0, SLPM65843, "SLPM-65843"}},
            // ゲームになったよ！ドクロちゃん～健康診断大作戦～
            {0x1B9Bbec, {DIRECT_READ, 0, 0, 0, FSLPM65997, "SLPM-66186"}},
            // 地獄少女 澪縁
            {0x2A078F, {DIRECT_READ, 0, 0, 0, SLPM65396, "SLPM-55213"}},
            // エーデルブルーメ
            {0x46631C, {DIRECT_READ, 0, 0, 0, FSLPM65997, "SLPM-66975"}},
            // セキレイ ～未来からのおくりもの～
            {0x4639C5, {DIRECT_READ, 0, 0, 0, SLPM55227, "SLPM-55227"}},
            // Fate/stay night[Realta Nua]
            {0x2C0DD5, {DIRECT_READ, 0, 0, 0, 0, "SLPM-66513"}},
        };
        return 0;
    }();
}