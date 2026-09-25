// Enhanced Archangels -- more Archangels for Mad Max (2015, PC).
//
// Part 1: names and descriptions for the three new Ripper Archangels added by
// dropzone\vehicles\archetypes.xlsc.
// Part 2: the 16-entry limit of the garage and Collectibles screens (see
// FillArchangelGui below).
// Part 3 (diagnostics): the game's script VM output -- dbgout messages and
// script exceptions -- copied into this log (see ScriptPrint_hook below).
//
// The garage asks the string table for "gui_<archangel id>_name" and
// "gui_<archangel id>_desc" (e.g. gui_archangel_jack_name = "The Jack"). The
// string table is keyed by Jenkins(key), and every lookup goes through
// CLocator::GetLocalizedStringRaw(id) -> const char*. This plugin hooks that
// function and answers the ids of the new keys with its own text; every other
// id goes to the game untouched, so no language file is modified.
//
// Addresses are found by byte signature (taken from the GOG 2015-12-07 build),
// with the same "already hooked by another mod" tolerance as Wasteland Storms
// and Enhanced Convoys: MinHook leaves `E9 rel32` in the first 5 bytes and the
// rest of the prologue intact.

#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cstdarg>
#include <initializer_list>
#include "MinHook.h"

// EA_DEV 1: also copy the game's script output and script errors into the log
// (hooks two XVM functions and switches on the GUI VM's print flag). Release
// builds ship with 0.
#define EA_DEV 0
#define EA_VERSION "0.9.0-beta1"

// ---------------------------------------------------------------- logging --
static char g_logPath[MAX_PATH] = "EnhancedArchangels.log";
static DWORD g_logStart = 0;

static void InitLog(HMODULE self) {
    char dir[MAX_PATH];
    if (GetModuleFileNameA(self, dir, MAX_PATH)) {
        char* slash = strrchr(dir, '\\');
        if (slash) slash[1] = 0;
        char prev[MAX_PATH];
        snprintf(g_logPath, MAX_PATH, "%sEnhancedArchangels.log", dir);
        snprintf(prev, MAX_PATH, "%sEnhancedArchangels.previous.log", dir);
        MoveFileExA(g_logPath, prev, MOVEFILE_REPLACE_EXISTING);
    }
    g_logStart = GetTickCount();
}

static void LogLine(const char* fmt, ...) {
    FILE* f = nullptr;
    if (fopen_s(&f, g_logPath, "a") != 0 || !f) return;
    SYSTEMTIME st; GetLocalTime(&st);
    DWORD t = GetTickCount() - g_logStart;
    fprintf(f, "[%02d:%02d:%02d +%5lu.%01lus] ", st.wHour, st.wMinute, st.wSecond, (unsigned long)(t / 1000), (unsigned long)((t % 1000) / 100));
    va_list ap; va_start(ap, fmt); vfprintf(f, fmt, ap); va_end(ap);
    fputc('\n', f);
    fclose(f);
}

// ------------------------------------------------------ signature scanning --
struct GameSignature { const char* name; const char* pattern; bool required; uintptr_t resolved; int matches; bool alreadyHooked; };
static GameSignature g_sigs[] = {
    { "CLocator::GetLocalizedStringRaw", "40 53 48 83 EC 30 4C 8B 41 28 8B DA 49 8B 40 08 80 78 49 00 75 ?? 39 50 18", true, 0, 0, false },
    { "garage GUI fill (FUN_140272090)", "48 8B C4 55 56 57 41 54 41 55 41 56 41 57 48 8D A8 38 FC FF FF 48 81 EC 90 04 00 00", true, 0, 0, false },
    { "g_GarageState load in the GUI fill", "48 8B 05 ?? ?? ?? ?? 48 85 C0 0F 84 ?? ?? ?? ?? 48 8B 88 50 01 00 00 48 2B 88 48 01 00 00", true, 0, 0, false },
    { "xvm_ava_win_print", "48 89 4C 24 08 48 89 54 24 10 4C 89 44 24 18 4C 89 4C 24 20 53 57 48 81 EC 38 04 00 00", false, 0, 0, false },
    { "XvmRaiseException", "48 89 5C 24 08 57 48 83 EC 20 49 8B C0 8B FA 48 8B D9 4D 85 C0 74 ?? 48 81 C1 B0 01 00 00", false, 0, 0, false },
    { "XvmExceptionToString", "83 F9 14 0F 87 ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? 48 63 C1 8B 8C 82 ?? ?? ?? ??", false, 0, 0, false },
};
enum { SIG_LOCRAW = 0, SIG_GUIFILL, SIG_GARAGESTATE, SIG_XVMPRINT, SIG_XVMRAISE, SIG_XVMEXCSTR, SIG_COUNT };
static uintptr_t g_textStart = 0, g_textEnd = 0;

static bool ScanBytes(const unsigned char* bytes, const bool* wild, int len, GameSignature& sig) {
    sig.matches = 0; sig.resolved = 0;
    for (const unsigned char* p = (const unsigned char*)g_textStart, *end = (const unsigned char*)g_textEnd - len; p <= end; p++) {
        if (!wild[0] && p[0] != bytes[0]) continue;
        int i = 1;
        for (; i < len; i++) if (!wild[i] && p[i] != bytes[i]) break;
        if (i == len) { if (++sig.matches == 1) sig.resolved = (uintptr_t)p; else break; }
    }
    return sig.matches == 1;
}

static bool ScanPattern(GameSignature& sig) {
    unsigned char bytes[128]; bool wild[128]; int len = 0;
    for (const char* c = sig.pattern; *c && len < 128; ) {
        while (*c == ' ') c++;
        if (!*c) break;
        if (c[0] == '?') { wild[len] = true; bytes[len] = 0; len++; c += 2; continue; }
        unsigned int v = 0; sscanf_s(c, "%2x", &v); bytes[len] = (unsigned char)v; wild[len] = false; len++; c += 2;
    }
    sig.alreadyHooked = false;
    if (ScanBytes(bytes, wild, len, sig)) return true;
    if (sig.matches != 0 || len < 12) return false;
    bytes[0] = 0xE9; wild[0] = false;
    for (int i = 1; i < 5; i++) wild[i] = true;
    if (!ScanBytes(bytes, wild, len, sig)) return false;
    sig.alreadyHooked = true;
    return true;
}

static bool FindText() {
    uintptr_t base = (uintptr_t)GetModuleHandleA(NULL);
    const IMAGE_NT_HEADERS64* nt = (const IMAGE_NT_HEADERS64*)(base + ((const IMAGE_DOS_HEADER*)base)->e_lfanew);
    const IMAGE_SECTION_HEADER* sec = IMAGE_FIRST_SECTION(nt);
    for (unsigned i = 0; i < nt->FileHeader.NumberOfSections; i++, sec++)
        if (memcmp(sec->Name, ".text", 5) == 0) { g_textStart = base + sec->VirtualAddress; g_textEnd = g_textStart + sec->Misc.VirtualSize; return true; }
    return false;
}

// Jenkins lookup3 hashlittle, seed 0 -- the engine's string hash.
static uint32_t Jenkins(const char* str) {
    const uint8_t* k = (const uint8_t*)str; uint32_t length = (uint32_t)strlen(str);
    uint32_t a, b, c; a = b = c = 0xDEADBEEF + length;
    #define ROT(x, n) (((x) << (n)) | ((x) >> (32 - (n))))
    while (length > 12) {
        a += k[0] | (k[1] << 8) | (k[2] << 16) | ((uint32_t)k[3] << 24);
        b += k[4] | (k[5] << 8) | (k[6] << 16) | ((uint32_t)k[7] << 24);
        c += k[8] | (k[9] << 8) | (k[10] << 16) | ((uint32_t)k[11] << 24);
        a -= c; a ^= ROT(c, 4); c += b;  b -= a; b ^= ROT(a, 6);  a += c;
        c -= b; c ^= ROT(b, 8); b += a;  a -= c; a ^= ROT(c, 16); c += b;
        b -= a; b ^= ROT(a, 19); a += c; c -= b; c ^= ROT(b, 4);  b += a;
        length -= 12; k += 12;
    }
    switch (length) {
    case 12: c += (uint32_t)k[11] << 24; [[fallthrough]];
    case 11: c += k[10] << 16; [[fallthrough]];
    case 10: c += k[9] << 8; [[fallthrough]];
    case 9:  c += k[8]; [[fallthrough]];
    case 8:  b += (uint32_t)k[7] << 24; [[fallthrough]];
    case 7:  b += k[6] << 16; [[fallthrough]];
    case 6:  b += k[5] << 8; [[fallthrough]];
    case 5:  b += k[4]; [[fallthrough]];
    case 4:  a += (uint32_t)k[3] << 24; [[fallthrough]];
    case 3:  a += k[2] << 16; [[fallthrough]];
    case 2:  a += k[1] << 8; [[fallthrough]];
    case 1:  a += k[0]; break;
    case 0:  return c;
    }
    c ^= b; c -= ROT(b, 14); a ^= c; a -= ROT(c, 11); b ^= a; b -= ROT(a, 25);
    c ^= b; c -= ROT(b, 16); a ^= c; a -= ROT(c, 4);  b ^= a; b -= ROT(a, 14);
    c ^= b; c -= ROT(b, 24);
    #undef ROT
    return c;
}

// ------------------------------------------------------------ text table --
struct ModText { const char* key; const char* text; uint32_t id; };
static ModText g_texts[] = {
    { "gui_archangel_crusader_of_torments_name", "Crusader of Torments", 0 },
    { "gui_archangel_crusader_of_torments_desc",
      "Behold the Crusader of Torments! Rammer and plate from grille to tail, she marches on the heathen convoys "
      "and they break against her like dust against the Angel. No boarder dares her, no blade reaches her.", 0 },
    { "gui_archangel_skull_collector_name", "Skull Collector", 0 },
    { "gui_archangel_skull_collector_desc",
      "Swifter than a dying scream, the Skull Collector swallows the long road in one breath and leaves only trophies "
      "behind. None climbs aboard her... but mind the curves, for she forgives none.", 0 },
    { "gui_archangel_noble_vagrant_name", "Noble Vagrant", 0 },
    { "gui_archangel_noble_vagrant_desc",
      "The Noble Vagrant wanders where no road dares. Sand, rock or ruin, she dances over it all with the grace of "
      "the blessed, humble of engine yet sure of every step.", 0 },
    { "gui_archangel_penitent_stalker_name", "Penitent Stalker", 0 },
    { "gui_archangel_penitent_stalker_desc",
      "The Penitent Stalker hides her holy shape beneath the colors of the waste. Up on her tall springs she creeps "
      "through dune and scrub, waits in silence, then strikes with the thunder of a Big Chief V8 and a heart full of faith.", 0 },
};
static const int TEXT_COUNT = sizeof(g_texts) / sizeof(g_texts[0]);
static volatile LONG g_textHits[TEXT_COUNT];

typedef const char* (*LocRawFn)(void* locator, uint32_t id);
static LocRawFn LocRaw_orig = nullptr;

static const char* LocRaw_hook(void* locator, uint32_t id) {
    for (int i = 0; i < TEXT_COUNT; i++) {
        if (g_texts[i].id == id) {
            if (InterlockedIncrement(&g_textHits[i]) == 1) LogLine("text served: %s", g_texts[i].key);
            return g_texts[i].text;
        }
    }
    return LocRaw_orig(locator, id);
}

// ------------------------------------------------- garage screen limit --
// CGarageState keeps the Archangels in a std::vector<SArchetype> (0x64 bytes
// each, begin/end at +0x148/+0x150) -- that part grows fine -- but the garage
// screen's copy is a FIXED array SGuiArchetype[16] (0xA0 each) at +0x170.
// FUN_140272090, the upgrade menu's GUI update, sets the menu count to the
// vector size and calls SetArchetypeGuiData for every entry with no clamp, so
// entries 17+ overwrite +0xB70 (the selected-archetype pointer, dereferenced
// right after) and +0xB80: a crash as soon as the garage opens with more than
// 16 Archangels. Checked in Ghidra (2026-09-24): no other code writes that
// array -- CForceArchetype::HandleEvent and CVehicleModule::GetNrArchetypes
// only search/count the vector, CGarageState::UpdateGuiData only reads +0xB70.
//
// The GUI scripts of the garage and of the Collectibles page do not read the
// vector: their Game.GetNrArchetypes / GetArchetype(i) read the GUI data block
// this function fills (count byte at +0x3709, SGuiArchetype* at +0x3710). And
// they run INSIDE this function, so 0.2.0's trick (shortening the vector for
// the whole call) also hid the new Archangels from both screens.
//
// Fix (0.3.0): replace the fill block in place (0x1402723DE-0x140272468:
// count, array pointer, selected entry, SetArchetypeGuiData loop) with a call
// to FillArchangelGui, which fills a bigger array of our own when there are
// more than 16 Archangels, points the GUI block at it, and keeps the stock
// array (the first 16) up to date for any code that still reads it.
static const int GUI_ARCHANGEL_SLOTS = 16, ARCHETYPE_SIZE = 0x64, GUI_ARCHETYPE_SIZE = 0xA0, MAX_ARCHANGELS = 64;
static void** g_garageStateVar = nullptr;
typedef void (*SetArchetypeGuiDataFn)(void* garageState, const void* archetype, void* guiArchetype);
static SetArchetypeGuiDataFn SetArchetypeGuiData = nullptr;
static uint8_t g_guiArchetypes[MAX_ARCHANGELS * GUI_ARCHETYPE_SIZE];
static volatile LONG g_bigListLogged = 0;

static bool g_bonusReady = false;
static void UpdateArchangelBonus();

static void FillArchangelGui(uint8_t* gui) {
    if (g_bonusReady) UpdateArchangelBonus();   // the pause menu stops UpdatePostSim, the garage screen still fills
    uint8_t* gs = (uint8_t*)*g_garageStateVar;          // non-null: checked by the game just before
    uint8_t* begin = *(uint8_t**)(gs + 0x148);
    uint8_t* end = *(uint8_t**)(gs + 0x150);
    int n = begin ? (int)((end - begin) / ARCHETYPE_SIZE) : 0;
    if (n > MAX_ARCHANGELS) n = MAX_ARCHANGELS;
    for (int i = 0; i < n && i < GUI_ARCHANGEL_SLOTS; i++)
        SetArchetypeGuiData(gs, begin + i * ARCHETYPE_SIZE, gs + 0x170 + i * GUI_ARCHETYPE_SIZE);
    uint8_t* list = gs + 0x170;
    if (n > GUI_ARCHANGEL_SLOTS) {
        for (int i = 0; i < n; i++)
            SetArchetypeGuiData(gs, begin + i * ARCHETYPE_SIZE, g_guiArchetypes + i * GUI_ARCHETYPE_SIZE);
        list = g_guiArchetypes;
        if (InterlockedIncrement(&g_bigListLogged) == 1)
            LogLine("garage/collectibles screens: %d Archangels, served from the extended list", n);
    }
    gui[0x3709] = (uint8_t)n;
    *(uint8_t**)(gui + 0x3710) = list;
    *(uint8_t**)(gui + 0x3718) = *(void**)(gs + 0xB70) ? gs + 0xB80 : nullptr;
}

// Offsets from the g_GarageState load (0x1402723A2) in the GOG build.
static const int FILL_START = 0x3C, FILL_CALL = 0xB3, FILL_END = 0xC7;

static bool InstallGuiFillPatch(uintptr_t site) {
    static const uint8_t countWrite[] = { 0x41, 0x88, 0x94, 0x24, 0x09, 0x37, 0x00, 0x00 };   // mov [r12+3709h],dl
    static const uint8_t afterLoop[] = { 0x48, 0x8B, 0x0D };                                  // mov rcx,[rip+x]
    uint8_t* p = (uint8_t*)site;
    if (memcmp(p + FILL_START, countWrite, sizeof(countWrite)) != 0 || p[FILL_CALL] != 0xE8 ||
        memcmp(p + FILL_END, afterLoop, sizeof(afterLoop)) != 0) {
        LogLine("  fill block does not look as expected, not patched");
        return false;
    }
    SetArchetypeGuiData = (SetArchetypeGuiDataFn)(site + FILL_CALL + 5 + *(int32_t*)(p + FILL_CALL + 1));
    uint8_t code[FILL_END - FILL_START];
    memset(code, 0xCC, sizeof(code));
    int k = 0;
    const uint8_t pre[] = { 0x49, 0x8B, 0xCC,              // mov rcx, r12
                            0x48, 0x83, 0xEC, 0x20,        // sub rsp, 20h (rsp stays 16-aligned)
                            0x48, 0xB8 };                  // mov rax, imm64
    memcpy(code + k, pre, sizeof(pre)); k += sizeof(pre);
    uint64_t fn = (uint64_t)&FillArchangelGui; memcpy(code + k, &fn, 8); k += 8;
    const uint8_t post[] = { 0xFF, 0xD0,                   // call rax
                             0x48, 0x83, 0xC4, 0x20,       // add rsp, 20h
                             0xE9 };                       // jmp rel32 -> end of the old loop
    memcpy(code + k, post, sizeof(post)); k += sizeof(post);
    int32_t rel = (int32_t)((site + FILL_END) - (site + FILL_START + k + 4)); memcpy(code + k, &rel, 4); k += 4;
    DWORD old;
    if (!VirtualProtect(p + FILL_START, sizeof(code), PAGE_EXECUTE_READWRITE, &old)) return false;
    memcpy(p + FILL_START, code, sizeof(code));
    VirtualProtect(p + FILL_START, sizeof(code), old, &old);
    FlushInstructionCache(GetCurrentProcess(), p + FILL_START, sizeof(code));
    return true;
}

// ------------------------------------------------------------ script log --
// The game's scripts (GUI screens included) have a `dbgout` instruction that
// prints through xvm_ava_win_print(fmt, ...) -- in the retail build it only
// reaches OutputDebugString -- and runtime errors go through
// XvmRaiseException(state, EXvmException, message). Both are copied here, as
// "[script]" and "[script error]" lines, so a GUI script that hangs can be
// diagnosed from the log. Repeats of the same line are folded, and the total
// is capped so a chatty script cannot flood the file.
static const int SCRIPT_LOG_MAX = 3000;
static CRITICAL_SECTION g_scriptLogLock;
static int g_scriptLines = 0, g_scriptRepeat = 0;
static char g_scriptLast[512] = "";
typedef void (*XvmPrintFn)(const char* fmt, ...);
typedef void (*XvmRaiseFn)(void* state, int exception, const char* message);
typedef const char* (*XvmExcStrFn)(int exception);
static XvmPrintFn ScriptPrint_orig = nullptr;
static XvmRaiseFn ScriptRaise_orig = nullptr;
static XvmExcStrFn XvmExceptionToString = nullptr;

static void ScriptLog(const char* kind, const char* text) {
    EnterCriticalSection(&g_scriptLogLock);
    if (strcmp(text, g_scriptLast) == 0) {
        g_scriptRepeat++;
    } else {
        if (g_scriptRepeat) LogLine("  (previous line repeated %d more times)", g_scriptRepeat);
        g_scriptRepeat = 0;
        strncpy_s(g_scriptLast, text, _TRUNCATE);
        if (g_scriptLines < SCRIPT_LOG_MAX) LogLine("%s %s", kind, text);
        else if (g_scriptLines == SCRIPT_LOG_MAX) LogLine("[script] log cap of %d lines reached, further script output dropped", SCRIPT_LOG_MAX);
        g_scriptLines++;
    }
    LeaveCriticalSection(&g_scriptLogLock);
}

static void ScriptPrint_hook(const char* fmt, ...) {
    char buf[512];
    va_list ap; va_start(ap, fmt); vsnprintf(buf, sizeof(buf), fmt ? fmt : "(null)", ap); va_end(ap);
    for (char* c = buf; *c; c++) if (*c == '\n' || *c == '\r') *c = ' ';
    ScriptLog("[script]", buf);
}

static void ScriptRaise_hook(void* state, int exception, const char* message) {
    char buf[512];
    const char* name = XvmExceptionToString ? XvmExceptionToString(exception) : nullptr;
    snprintf(buf, sizeof(buf), "%s (%d): %s", name ? name : "exception", exception, message ? message : "");
    ScriptLog("[script error]", buf);
    ScriptRaise_orig(state, exception, message);
}

static void InstallScriptLog() {
    InitializeCriticalSection(&g_scriptLogLock);
    if (g_sigs[SIG_XVMEXCSTR].matches == 1) XvmExceptionToString = (XvmExcStrFn)g_sigs[SIG_XVMEXCSTR].resolved;
    MH_STATUS a = MH_ERROR_FUNCTION_NOT_FOUND, b = MH_ERROR_FUNCTION_NOT_FOUND;
    if (g_sigs[SIG_XVMPRINT].matches == 1) {
        a = MH_CreateHook((LPVOID)g_sigs[SIG_XVMPRINT].resolved, (LPVOID)ScriptPrint_hook, (LPVOID*)&ScriptPrint_orig);
        if (a == MH_OK) a = MH_EnableHook((LPVOID)g_sigs[SIG_XVMPRINT].resolved);
    }
    if (g_sigs[SIG_XVMRAISE].matches == 1) {
        b = MH_CreateHook((LPVOID)g_sigs[SIG_XVMRAISE].resolved, (LPVOID)ScriptRaise_hook, (LPVOID*)&ScriptRaise_orig);
        if (b == MH_OK) b = MH_EnableHook((LPVOID)g_sigs[SIG_XVMRAISE].resolved);
    }
    LogLine("script log: dbgout %s, exceptions %s", MH_StatusToString(a), MH_StatusToString(b));
}

// The GUI screens' scripts do not run in the exe's VM but in a copy of it
// inside GuiXvmFragments_F.dll (loaded at run time). Same functions there,
// plus one difference that matters: dbgout only prints when the shared
// state's print flag (+0x91) is set, which it is not in the retail game, so
// that check is skipped (2-byte je -> nop). Found by pattern inside the DLL.
static XvmPrintFn GuiPrint_orig = nullptr;
static XvmRaiseFn GuiRaise_orig = nullptr;
static XvmExcStrFn GuiExceptionToString = nullptr;

static uintptr_t ScanModule(HMODULE mod, const char* pattern, int* matches) {
    unsigned char bytes[256]; bool wild[256]; int len = 0;
    for (const char* c = pattern; *c && len < 256; ) {
        while (*c == ' ') c++;
        if (!*c) break;
        if (c[0] == '?') { wild[len] = true; bytes[len] = 0; len++; c += 2; continue; }
        unsigned int v = 0; sscanf_s(c, "%2x", &v); bytes[len] = (unsigned char)v; wild[len] = false; len++; c += 2;
    }
    uintptr_t base = (uintptr_t)mod, found = 0;
    const IMAGE_NT_HEADERS64* nt = (const IMAGE_NT_HEADERS64*)(base + ((const IMAGE_DOS_HEADER*)base)->e_lfanew);
    const IMAGE_SECTION_HEADER* sec = IMAGE_FIRST_SECTION(nt);
    *matches = 0;
    for (int k = 0; k < nt->FileHeader.NumberOfSections; k++, sec++) {
        if (!(sec->Characteristics & IMAGE_SCN_MEM_EXECUTE)) continue;
        const unsigned char* p = (const unsigned char*)(base + sec->VirtualAddress);
        const unsigned char* end = p + sec->Misc.VirtualSize - len;
        for (; p <= end; p++) {
            int i = 0;
            for (; i < len; i++) if (!wild[i] && p[i] != bytes[i]) break;
            if (i == len && ++*matches == 1) found = (uintptr_t)p;
        }
    }
    return *matches == 1 ? found : 0;
}

// dbgout prints each argument with its own call and then a newline, so the
// pieces are collected into one line and logged at the newline.
static char g_guiLine[512] = "";
static bool ScanModuleAt(uintptr_t at, const char* pattern) {
    const unsigned char* p = (const unsigned char*)at;
    for (const char* c = pattern; *c; ) {
        while (*c == ' ') c++;
        if (!*c) break;
        if (c[0] == '?') { p++; c += 2; continue; }
        unsigned int v = 0; sscanf_s(c, "%2x", &v);
        if (*p != (unsigned char)v) return false;
        p++; c += 2;
    }
    return true;
}

static void GuiPrint_hook(const char* fmt, ...) {
    char buf[512];
    va_list ap; va_start(ap, fmt); vsnprintf(buf, sizeof(buf), fmt ? fmt : "(null)", ap); va_end(ap);
    bool eol = false;
    for (char* c = buf; *c; c++) if (*c == '\n' || *c == '\r') { *c = 0; eol = true; break; }
    if (buf[0]) {
        if (g_guiLine[0]) strcat_s(g_guiLine, " ");
        strncat_s(g_guiLine, buf, _TRUNCATE);
    }
    if (eol || strlen(g_guiLine) > 400) {
        if (g_guiLine[0]) ScriptLog("[gui script]", g_guiLine);
        g_guiLine[0] = 0;
    }
}

static void GuiRaise_hook(void* state, int exception, const char* message) {
    char buf[512];
    const char* name = GuiExceptionToString ? GuiExceptionToString(exception) : nullptr;
    snprintf(buf, sizeof(buf), "%s (%d): %s", name ? name : "exception", exception, message ? message : "");
    ScriptLog("[gui script error]", buf);
    GuiRaise_orig(state, exception, message);
}

static DWORD WINAPI GuiScriptLogThread(LPVOID) {
    HMODULE dll = nullptr;
    for (int i = 0; i < 1200 && !(dll = GetModuleHandleA("GuiXvmFragments_F.dll")); i++) Sleep(100);
    if (!dll) { LogLine("gui script log: GuiXvmFragments_F.dll never loaded"); return 0; }
    int mp, mr, me, mg;
    uintptr_t print = ScanModule(dll, "48 89 4C 24 08 48 89 54 24 10 4C 89 44 24 18 4C 89 4C 24 20 53 57 48 81 EC 38 04 00 00", &mp);
    uintptr_t raise = ScanModule(dll, "48 89 5C 24 08 57 48 83 EC 20 49 8B C0 8B FA 48 8B D9 4D 85 C0 74 ?? 48 81 C1 B0 01 00 00", &mr);
    uintptr_t excs  = ScanModule(dll, "83 F9 14 0F 87 ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? 48 63 C1 8B 8C 82 ?? ?? ?? ??", &me);
    uintptr_t gate  = ScanModule(dll, "41 D1 EC 80 B8 91 00 00 00 00 74 ??", &mg);
    GuiExceptionToString = (XvmExcStrFn)excs;
    MH_STATUS a = MH_ERROR_FUNCTION_NOT_FOUND, b = MH_ERROR_FUNCTION_NOT_FOUND;
    if (print && (a = MH_CreateHook((LPVOID)print, (LPVOID)GuiPrint_hook, (LPVOID*)&GuiPrint_orig)) == MH_OK) a = MH_EnableHook((LPVOID)print);
    if (raise && (b = MH_CreateHook((LPVOID)raise, (LPVOID)GuiRaise_hook, (LPVOID*)&GuiRaise_orig)) == MH_OK) b = MH_EnableHook((LPVOID)raise);
    bool gated = false;
    if (gate) {
        DWORD old; uint8_t* je = (uint8_t*)gate + 10;
        if (VirtualProtect(je, 2, PAGE_EXECUTE_READWRITE, &old)) { je[0] = 0x90; je[1] = 0x90; VirtualProtect(je, 2, old, &old); FlushInstructionCache(GetCurrentProcess(), je, 2); gated = true; }
    }
    LogLine("gui script log: dbgout %s (matches %d), print flag bypass %s (matches %d), exceptions %s (matches %d)",
        MH_StatusToString(a), mp, gated ? "ok" : "FAILED", mg, MH_StatusToString(b), mr);
    return 0;
}

// ------------------------------------ InstallArchetype graph node (T flow) --
// Picking an Archangel from a stronghold's "Choose Vehicle" screen runs the
// graph death_run_equip_archangel, whose InstallArchetype node
// (NGSONodes::InstallArchetype) finds the Archangel's index in the vector and
// then -- unless the node is forced -- checks its "can be installed" flag
// straight in the FIXED GUI array: byte [gs + 0x170 + i*0xA0 + 0x97]. For
// i >= 16 that reads past the array, the check fails, the node takes its
// "failed" exit ("failed to install archangel!") and the screen stays black.
// The pause-menu garage installs by name and never hits this.
// Fix: that check (26 bytes at 0x1402D6B27) jumps to a small stub that asks
// ArchetypeCanInstall(i), which builds the GUI entry for index i in a scratch
// buffer with the game's own SetArchetypeGuiData and returns its +0x97 flag.
static volatile LONG g_installChecksLogged = 0;
static bool ArchetypeCanInstall(int index) {
    uint8_t* gs = g_garageStateVar ? (uint8_t*)*g_garageStateVar : nullptr;
    if (!gs || !SetArchetypeGuiData) return false;
    uint8_t* begin = *(uint8_t**)(gs + 0x148);
    uint8_t* end = *(uint8_t**)(gs + 0x150);
    int n = begin ? (int)((end - begin) / ARCHETYPE_SIZE) : 0;
    if (index < 0 || index >= n) return false;
    uint8_t entry[GUI_ARCHETYPE_SIZE] = {};
    SetArchetypeGuiData(gs, begin + index * ARCHETYPE_SIZE, entry);
    bool ok = entry[0x97] != 0;
    if (index >= GUI_ARCHANGEL_SLOTS && InterlockedIncrement(&g_installChecksLogged) <= 10)
        LogLine("install node: Archangel #%d can be installed: %s", index + 1, ok ? "yes" : "no");
    return ok;
}

static bool InstallInstallNodePatch() {
    static const uint8_t site_bytes[] = { 0x45,0x84,0xF6,0x75,0x15,0x48,0x63,0xC6,0x48,0x8D,0x14,0x80,0x48,0xC1,0xE2,0x05,
                                          0x46,0x38,0xB4,0x0A,0x07,0x02,0x00,0x00,0x74,0x19 };
    int matches = 0;
    char pat[26 * 3 + 1] = ""; for (int i = 0; i < 26; i++) sprintf_s(pat + i * 3, 4, "%02X ", site_bytes[i]);
    uint8_t* site = (uint8_t*)ScanModule(GetModuleHandleA(NULL), pat, &matches);
    if (!site) { LogLine("install node fix: site not found (matches %d)", matches); return false; }
    uintptr_t ok = (uintptr_t)site + 26, fail = (uintptr_t)site + 0x33;
    uint8_t* cave = (uint8_t*)VirtualAlloc(nullptr, 128, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!cave) return false;
    uint8_t* c = cave;
    auto emit = [&](std::initializer_list<uint8_t> b) { for (uint8_t x : b) *c++ = x; };
    auto emit64 = [&](uint64_t v) { memcpy(c, &v, 8); c += 8; };
    emit({ 0x45, 0x84, 0xF6 });                 // test r14b, r14b   (forced install: skip the check)
    emit({ 0x75, 0x1E });                       // jnz ok (skips the 30 bytes up to "ok")
    emit({ 0x41, 0x51 });                       // push r9           (g_GarageState, used right after)
    emit({ 0x48, 0x83, 0xEC, 0x28 });           // sub rsp, 28h      (keeps rsp 16-aligned for the call)
    emit({ 0x8B, 0xCE });                       // mov ecx, esi      (archetype index)
    emit({ 0x48, 0xB8 }); emit64((uint64_t)&ArchetypeCanInstall);
    emit({ 0xFF, 0xD0 });                       // call rax
    emit({ 0x48, 0x83, 0xC4, 0x28 });           // add rsp, 28h
    emit({ 0x41, 0x59 });                       // pop r9
    emit({ 0x84, 0xC0 });                       // test al, al
    emit({ 0x74, 0x0C });                       // jz fail
    emit({ 0x48, 0xB8 }); emit64(ok); emit({ 0xFF, 0xE0 });     // ok:   jmp site+26
    emit({ 0x48, 0xB8 }); emit64(fail); emit({ 0xFF, 0xE0 });   // fail: jmp site+0x33
    FlushInstructionCache(GetCurrentProcess(), cave, c - cave);
    uint8_t jump[26]; memset(jump, 0xCC, sizeof(jump));
    jump[0] = 0x48; jump[1] = 0xB8; uint64_t target = (uint64_t)cave; memcpy(jump + 2, &target, 8);
    jump[10] = 0xFF; jump[11] = 0xE0;           // mov rax, cave ; jmp rax
    DWORD old;
    if (!VirtualProtect(site, sizeof(jump), PAGE_EXECUTE_READWRITE, &old)) return false;
    memcpy(site, jump, sizeof(jump));
    VirtualProtect(site, sizeof(jump), old, &old);
    FlushInstructionCache(GetCurrentProcess(), site, sizeof(jump));
    LogLine("install node fix: patched at %p (stub %p)", site, cave);
    return true;
}

// ------------------------------------------------------- Archangel bonus --
// Every Archangel gets the bonus of one hood ornament of the group that fits
// its build best (strongest of ramming, armor+spikes, grip+suspension,
// exhaust+engine; "Weapons" when none reaches half): the same effects the
// game applies for an ornament (lib_hoodornament Activate<Group>Upgrades:
// grip, top speed/acceleration, fire resistance, ramming collision, harpoon
// reload...) plus one "+" on the stat. Neither ornament slot is touched: the
// plugin runs the front ornament's upgrade script itself --
// ActivateEnhacementLevel(level, false) / DeactivateEnhacementLevel -- with
// a level of the wanted group, exactly the call CVehicleUpgradeManager makes
// (XvmCall on the attribute's script module), but without changing the
// attribute or sending its model events. A real ornament of the same group
// stacks on top ("++").
// Checked twice a second from CVehicleUpgradeManager::UpdatePostSim: which
// Archangel is installed (CGarageState::ArchetypeIsInstalled) and on which
// signature vehicle; the bonus follows both.
struct HXvmObject { uint64_t type, value; };
typedef void  (*UpdatePostSimFn)(void* mgr, void* ctx);
typedef void* (*GetSignatureVehicleFn)();
typedef bool  (*ArchetypeIsInstalledFn)(void* garageState, const uint32_t* id);
typedef void  (*ScriptStateCtorFn)(void** state);
typedef void  (*ScriptStateDtorFn)(void** state);
typedef int   (*ResourceCacheGetPointerFn)(void* handle, void** out);
typedef int   (*XvmCall2Fn)(void* state, void* module, void* self, uint32_t fn, uint32_t argc, HXvmObject a, HXvmObject b);
typedef void  (*HandleXvmReturnStatusFn)(int result, void* module, void* state, void* state2);
static UpdatePostSimFn UpdatePostSim_orig = nullptr;
static GetSignatureVehicleFn GetSignatureVehicle = nullptr;
static ArchetypeIsInstalledFn ArchetypeIsInstalled = nullptr;
static ScriptStateCtorFn ScriptStateCtor = nullptr;
static ScriptStateDtorFn ScriptStateDtor = nullptr;
static ResourceCacheGetPointerFn ResourceCacheGetPointer = nullptr;
static XvmCall2Fn XvmCall2 = nullptr;
static HandleXvmReturnStatusFn HandleXvmReturnStatus = nullptr;

enum { GROUP_NONE = 0, GROUP_ENGINE = 1, GROUP_RAMMING = 2, GROUP_WEAPONS = 3, GROUP_TRACTION = 4, GROUP_ARMOR = 5 };
static const char* GroupName(int g) {
    switch (g) { case GROUP_ENGINE: return "Engine"; case GROUP_RAMMING: return "Ramming"; case GROUP_WEAPONS: return "Weapons";
                 case GROUP_TRACTION: return "Traction"; case GROUP_ARMOR: return "Armor"; default: return "none"; }
}
// The group number is also a front hood ornament level of that group
// (lib_hoodornament DetermineUpgradeGroup: 1 Engine, 2 Ramming, 3 Weapons,
// 4 Traction, 5 Armor), which is what the ornament script takes.
static struct { const char* id; int group; uint32_t hash; } g_bonus[] = {
    { "archangel_jack", GROUP_WEAPONS }, { "archangel_pinky_finger", GROUP_WEAPONS }, { "archangel_bare_bones", GROUP_WEAPONS },
    { "archangel_pumpkin_eater", GROUP_ARMOR }, { "archangel_kill_box", GROUP_RAMMING }, { "archangel_mr_spikes", GROUP_ARMOR },
    { "archangel_speed_demon", GROUP_ENGINE }, { "archangel_fire_eater", GROUP_ENGINE }, { "archangel_golden_flower", GROUP_TRACTION },
    { "archangel_bomber", GROUP_ENGINE }, { "archangel_ops", GROUP_TRACTION }, { "archangel_off_roadster", GROUP_TRACTION },
    { "archangel_side_grinder", GROUP_ENGINE }, { "archangel_battling_ram", GROUP_RAMMING }, { "archangel_jugger_tank", GROUP_RAMMING },
    { "archangel_speed_freak", GROUP_ENGINE }, { "archangel_crusader_of_torments", GROUP_RAMMING }, { "archangel_skull_collector", GROUP_ENGINE },
    { "archangel_noble_vagrant", GROUP_TRACTION }, { "archangel_penitent_stalker", GROUP_TRACTION },
};
static const int BONUS_COUNT = sizeof(g_bonus) / sizeof(g_bonus[0]);
static uint32_t g_hoodAttrHash = 0, g_activateHash = 0, g_deactivateHash = 0;
static void* g_bonusVehicle = nullptr;
static int g_bonusGroup = GROUP_NONE, g_bonusIndex = -1;
static ULONGLONG g_bonusNextCheck = 0;

// Runs hood_ornament.xvmc's Activate/DeactivateEnhacementLevel(level, false).
static bool RunOrnamentScript(uint8_t* gs, bool activate, int level) {
    uint8_t* a = *(uint8_t**)(gs + 0x128);
    uint8_t* e = *(uint8_t**)(gs + 0x130);
    uint8_t* attr = nullptr;
    for (; a && a < e; a += 0x60) if (*(uint32_t*)(a + 8) == g_hoodAttrHash) { attr = a; break; }
    if (!attr || !*(void**)attr) { LogLine("bonus: hood ornament attribute/script not found"); return false; }
    void* module = nullptr;
    if (ResourceCacheGetPointer(*(void**)attr, &module) != 0 || !module) { LogLine("bonus: hood ornament script not loaded"); return false; }
    void* st = nullptr;
    ScriptStateCtor(&st);
    if (!st) { LogLine("bonus: no script state"); return false; }
    float f = (float)level; uint32_t bits; memcpy(&bits, &f, 4);
    HXvmObject lv = { 0x30000, bits }, keepBuff = { 0x10000, 0 };
    int r = XvmCall2(st, module, nullptr, activate ? g_activateHash : g_deactivateHash, 2, lv, keepBuff);
    if (HandleXvmReturnStatus) HandleXvmReturnStatus(r, module, st, nullptr);
    ScriptStateDtor(&st);
    if (r != 0) LogLine("bonus: script call returned %d", r);
    return r == 0;
}

static void UndoPreviewBuff();
static void BumpBuff(int group, int delta);

static void UpdateArchangelBonus() {
    uint8_t* gs = g_garageStateVar ? (uint8_t*)*g_garageStateVar : nullptr;
    void* veh = GetSignatureVehicle ? GetSignatureVehicle() : nullptr;
    if (!gs || !veh) return;
    int index = -1, group = GROUP_NONE;
    for (int i = 0; i < BONUS_COUNT; i++)
        if (ArchetypeIsInstalled(gs, &g_bonus[i].hash)) { index = i; group = g_bonus[i].group; break; }
    if (veh != g_bonusVehicle) {            // a new signature vehicle: its upgrades were set up fresh, without our bonus
        if (g_bonusVehicle && g_bonusGroup != GROUP_NONE) {
            // the old car took its effects with it, but the "+" counters are global
            UndoPreviewBuff();
            BumpBuff(g_bonusGroup, -1);
            LogLine("bonus: vehicle changed, %s \"+\" cleared", GroupName(g_bonusGroup));
        }
        g_bonusVehicle = veh;
        g_bonusGroup = GROUP_NONE; g_bonusIndex = -1;
    }
    if (group == g_bonusGroup && index == g_bonusIndex) return;
    UndoPreviewBuff();
    if (g_bonusGroup != GROUP_NONE) {
        RunOrnamentScript(gs, false, g_bonusGroup);
        LogLine("bonus: %s removed (%s)", GroupName(g_bonusGroup), g_bonusIndex >= 0 ? g_bonus[g_bonusIndex].id : "?");
    }
    g_bonusGroup = GROUP_NONE; g_bonusIndex = index;
    if (group != GROUP_NONE && RunOrnamentScript(gs, true, group)) {
        g_bonusGroup = group;
        LogLine("bonus: %s applied for %s", GroupName(group), g_bonus[index].id);
    }
}

// The Archangels screen previews each Archangel's stats, but the "+" icons
// come from one buff counter per stat that only knows the installed car. While
// an Archangel is previewed (CVehicleUpgradeManager::SetArchAngelPreview) its
// group's "+" is shown instead of the installed one's -- only the counter
// moves, not the effects -- and ClearPreviewModel puts it back.
typedef void (*SetArchAngelPreviewFn)(void* mgr, const uint32_t* id);
typedef void (*ClearPreviewModelFn)(void* mgr);
typedef void (*ChangeStatBuffValueFn)(void* mgr, const uint32_t* stat, int delta);
static SetArchAngelPreviewFn SetArchAngelPreview_orig = nullptr;
static ClearPreviewModelFn ClearPreviewModel_orig = nullptr;
static ChangeStatBuffValueFn ChangeStatBuffValue = nullptr;
static int g_previewAdded = GROUP_NONE, g_previewRemoved = GROUP_NONE;
// stat whose "+" each group raises (lib_hoodornament Activate<Group>Upgrades)
static uint32_t GroupStat(int g) {
    switch (g) { case GROUP_ENGINE: return 0x555B281B; case GROUP_TRACTION: return 0x29169278; case GROUP_RAMMING: return 0x0BBCBB37;
                 case GROUP_WEAPONS: return 0x9E433D2D; case GROUP_ARMOR: return 0x358D358A; default: return 0; }
}
static void BumpBuff(int group, int delta) {
    uint32_t stat = GroupStat(group);
    if (stat && ChangeStatBuffValue) ChangeStatBuffValue(nullptr, &stat, delta);
}
static void UndoPreviewBuff() {
    if (g_previewAdded) BumpBuff(g_previewAdded, -1);
    if (g_previewRemoved) BumpBuff(g_previewRemoved, +1);
    g_previewAdded = g_previewRemoved = GROUP_NONE;
}
static void SetArchAngelPreview_hook(void* mgr, const uint32_t* id) {
    SetArchAngelPreview_orig(mgr, id);
    UndoPreviewBuff();
    int group = GROUP_NONE;
    for (int i = 0; id && i < BONUS_COUNT; i++) if (g_bonus[i].hash == *id) { group = g_bonus[i].group; break; }
    if (group == GROUP_NONE || group == g_bonusGroup) return;
    if (g_bonusGroup != GROUP_NONE) { BumpBuff(g_bonusGroup, -1); g_previewRemoved = g_bonusGroup; }
    BumpBuff(group, +1); g_previewAdded = group;
}
static void ClearPreviewModel_hook(void* mgr) {
    UndoPreviewBuff();
    ClearPreviewModel_orig(mgr);
}

static void UpdatePostSim_hook(void* mgr, void* ctx) {
    UpdatePostSim_orig(mgr, ctx);
    ULONGLONG now = GetTickCount64();
    if (now < g_bonusNextCheck) return;
    g_bonusNextCheck = now + 500;
    UpdateArchangelBonus();
}

// GetSignatureVehicle and GetSignatureVehicleCompanion have the same code,
// each reading its own static weak_ptr (lea rcx,[rip+x] at +15). The vehicle's
// static (s_SignatureVehicle) sits 0x10 above the companion's in the GOG
// build; take the getter whose static is the higher one.
static uintptr_t PickSignatureVehicleGetter(const char* pattern) {
    uintptr_t base = (uintptr_t)GetModuleHandleA(NULL), found[2] = { 0, 0 }; int n = 0;
    const IMAGE_NT_HEADERS64* nt = (const IMAGE_NT_HEADERS64*)(base + ((const IMAGE_DOS_HEADER*)base)->e_lfanew);
    const IMAGE_SECTION_HEADER* sec = IMAGE_FIRST_SECTION(nt);
    for (int k = 0; k < nt->FileHeader.NumberOfSections && n < 2; k++, sec++) {
        if (!(sec->Characteristics & IMAGE_SCN_MEM_EXECUTE)) continue;
        for (uintptr_t p = base + sec->VirtualAddress, end = p + sec->Misc.VirtualSize - 32; p < end && n < 2; p++) {
            if (*(uint32_t*)p != 0x245C8948) continue;               // 48 89 5C 24
            if (ScanModuleAt(p, pattern)) found[n++] = p;
        }
    }
    if (n != 2) return 0;
    auto target = [](uintptr_t f) { return f + 22 + *(int32_t*)(f + 18); };  // lea rcx,[rip+disp32] at +15
    uintptr_t pick = target(found[0]) > target(found[1]) ? found[0] : found[1];
    LogLine("bonus: GetSignatureVehicle candidates %p (static %p) / %p (static %p), using %p",
        (void*)found[0], (void*)target(found[0]), (void*)found[1], (void*)target(found[1]), (void*)pick);
    return pick;
}

static void InstallArchangelBonus() {
    struct { const char* name; const char* pat; void** out; bool required; } s[] = {
        { "UpgradeManager::UpdatePostSim", "48 83 EC 28 F3 0F 10 81 0C 01 00 00 0F 57 C9 0F 2F C1 76 ?? 80 B9 0A 01 00 00 00 75 ?? F3 0F 5C 02 0F 2F C1 F3 0F 11 81 0C 01 00 00 73 ?? 66 C7 81 08 01 00 00 00 00 E8 ?? ?? ?? ?? 48 83 C4 28 C3 CC CC CC CC CC CC CC CC CC CC CC CC CC CC CC 4C 8B 81 F0 00 00 00", nullptr, true },
        { "UpgradeManager::GetSignatureVehicle (twin-aware, see below)", "48 89 5C 24 10 57 48 83 EC 30 48 8D 54 24 20 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B 7C 24 28 48 8B 18 48 85 FF 74 ?? F0 FF 4F 08 48 89 74 24 40 75 ?? 48 8B 17 48 8B CF FF 52 08 F0 FF 4F 0C 75 ?? 48 8B 17 48 8B CF FF 52 10 48 8B C3 48 8B 74 24 40 48 8B 5C 24 48 48 83 C4 30 5F C3", (void**)&GetSignatureVehicle, true },
        { "GarageState::ArchetypeIsInstalled", "40 53 4C 8B 81 50 01 00 00 48 8B 81 48 01 00 00 49 3B C0 74 ?? 8B 12 39 10 74 ?? 48 83 C0 64 49 3B C0 75 ?? 32 C0 5B C3 48 89 74 24 10 48 89 7C 24 18 0F B6 78 5E 45 33 C9 85 FF 7E ?? 48 8B B1 28 01 00 00 48 8B 89 30 01 00 00 4C 8D 58 50 4C 8D 50 18 4C 8B C6 48 3B F1 74 ?? 41 8B 02 66 90 41 39 40 08 74 ?? 49 83 C0 60 4C 3B C1 75 ?? 4C 8B C6 41 0F B6 03 48 8D 14 C0 49 8B 40 38 48 03 D2 83 BC D0 84 00 00 00 02 75", (void**)&ArchetypeIsInstalled, true },
        { "XvmScriptState ctor", "40 53 48 83 EC 20 48 8B D9 E8 ?? ?? ?? ?? 48 89 03 48 8B C3 48 83 C4 20 5B C3 CC CC CC CC CC CC 40 53", (void**)&ScriptStateCtor, true },
        { "XvmScriptState dtor", "40 53 48 83 EC 20 48 8B D9 48 8B 09 48 85 C9 74 ?? E8 ?? ?? ?? ?? 48 C7 03 00 00 00 00 48 83 C4 20 5B C3 CC CC CC CC CC CC CC CC CC CC CC CC CC 40 53 48 83 EC 30 48 C7 44 24 20 FE FF FF FF 48 8B D9 B9 40 00 00 00", (void**)&ScriptStateDtor, true },
        { "ResourceCacheGetPointer", "0F B7 01 4C 8D 04 80 48 8B 41 08 48 8B 40 58 4A 8B 04 C0 48 89 02 33 C0", (void**)&ResourceCacheGetPointer, true },
        { "XvmCall (2 args)", "48 83 EC 58 4C 8B 94 24 88 00 00 00 49 8B 02 48 89 44 24 30 49 8B 42 08", (void**)&XvmCall2, true },
        { "HandleXvmReturnStatus", "48 89 5C 24 08 48 89 74 24 20 57 48 81 EC 10 02 00 00 48 8B 05", (void**)&HandleXvmReturnStatus, false },
        { "UpgradeManager::ChangeStatBuffValue", "45 33 C9 48 8D 0D ?? ?? ?? ?? 66 0F 1F 44 00 00 8B 02 39 01 75 ?? 0F B6 41 04", (void**)&ChangeStatBuffValue, false },
        { "UpgradeManager::SetArchAngelPreview", "40 53 55 48 81 EC 88 00 00 00 48 83 3D ?? ?? ?? ?? 00 48 8B DA 48 8B E9", (void**)&SetArchAngelPreview_orig, false },
        { "UpgradeManager::ClearPreviewModel", "48 89 74 24 10 57 48 83 EC 40 83 B9 E0 00 00 00 00 48 8B F9 0F 84 ?? ?? ?? ??", (void**)&ClearPreviewModel_orig, false },
    };
    uintptr_t updatePostSim = 0; bool ok = true;
    for (auto& e : s) {
        int m = 0; uintptr_t a = ScanModule(GetModuleHandleA(NULL), e.pat, &m);
        if (e.out == (void**)&GetSignatureVehicle && m == 2) a = PickSignatureVehicleGetter(e.pat);
        if (!a) { LogLine("bonus: %s not found (matches %d)", e.name, m); if (e.required) ok = false; continue; }
        if (e.out) *e.out = (void*)a; else updatePostSim = a;
    }
    if (!ok || !updatePostSim) { LogLine("bonus: disabled"); return; }
    g_hoodAttrHash = Jenkins("veh_attribute_hood_ornament");
    g_activateHash = Jenkins("ActivateEnhacementLevel");
    g_deactivateHash = Jenkins("DeactivateEnhacementLevel");
    for (int i = 0; i < BONUS_COUNT; i++) g_bonus[i].hash = Jenkins(g_bonus[i].id);
    MH_STATUS c = MH_CreateHook((LPVOID)updatePostSim, (LPVOID)UpdatePostSim_hook, (LPVOID*)&UpdatePostSim_orig);
    MH_STATUS e = (c == MH_OK) ? MH_EnableHook((LPVOID)updatePostSim) : c;
    LogLine("bonus: %s (%d Archangels mapped)", MH_StatusToString(e), BONUS_COUNT);
    g_bonusReady = (e == MH_OK);
    // preview "+" (optional: without it the "+" just stays on the installed car's stat)
    if (ChangeStatBuffValue && SetArchAngelPreview_orig && ClearPreviewModel_orig) {
        void* sp = (void*)SetArchAngelPreview_orig; void* cp = (void*)ClearPreviewModel_orig;
        MH_STATUS a = MH_CreateHook(sp, (LPVOID)SetArchAngelPreview_hook, (LPVOID*)&SetArchAngelPreview_orig);
        if (a == MH_OK) a = MH_EnableHook(sp);
        MH_STATUS b = MH_CreateHook(cp, (LPVOID)ClearPreviewModel_hook, (LPVOID*)&ClearPreviewModel_orig);
        if (b == MH_OK) b = MH_EnableHook(cp);
        LogLine("bonus preview: SetArchAngelPreview %s, ClearPreviewModel %s", MH_StatusToString(a), MH_StatusToString(b));
    }
}

// Rows in a dropzone Archangel table, or -1 if there is none. Used to warn
// when a >16 table is installed but the screen fix could not be.
static int DropzoneArchangelRows() {
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    char* slash = strrchr(path, '\\'); if (slash) slash[1] = 0;
    strcat_s(path, "dropzone\\vehicles\\archetypes.xlsc");
    FILE* f = nullptr;
    if (fopen_s(&f, path, "rb") != 0 || !f) return -1;
    static uint8_t buf[256 * 1024];
    size_t n = fread(buf, 1, sizeof(buf), f); fclose(f);
    if (n < 0x40 || memcmp(buf, " FDA", 4) != 0) return -1;
    uint32_t instTable = *(uint32_t*)(buf + 12);
    if (instTable + 16 > n) return -1;
    uint32_t data = *(uint32_t*)(buf + instTable + 8);
    if (data + 16 > n) return -1;
    uint32_t sheets = *(uint32_t*)(buf + data);                 // XLSBook.Sheet.offset
    if (data + sheets + 8 > n) return -1;
    return (int)*(uint32_t*)(buf + data + sheets + 4) - 1;     // first sheet rows minus the header row
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_DETACH) {
        LogLine("session end");
        return TRUE;
    }
    if (reason != DLL_PROCESS_ATTACH) return TRUE;
    DisableThreadLibraryCalls(hModule);
    InitLog(hModule);
    LogLine("Enhanced Archangels %s", EA_VERSION);
    if (!FindText()) { LogLine("no .text section, disabled"); return TRUE; }
    bool ok = true;
    for (int i = 0; i < SIG_COUNT; i++) {
        bool found = ScanPattern(g_sigs[i]);
        LogLine("  %-40s %s (matches %d)%s", g_sigs[i].name, found ? "OK" : "--", g_sigs[i].matches,
            g_sigs[i].alreadyHooked ? " -- already hooked by another mod, chaining behind it" : "");
        if (!found && g_sigs[i].required) ok = false;
    }
    if (!ok && DropzoneArchangelRows() > GUI_ARCHANGEL_SLOTS) {
        MessageBoxA(NULL, "Enhanced Archangels is DISABLED on this game version, but an Archangel table with more than 16 entries "
            "is installed (dropzone\\vehicles\\archetypes.xlsc).\n\nThe garage WILL crash. Remove that file before playing.",
            "Mad Max - Enhanced Archangels", MB_OK | MB_ICONERROR);
        return TRUE;
    }
    if (!ok) {
        MessageBoxA(NULL, "Enhanced Archangels is DISABLED: it could not find the game functions it needs in this executable.\n\n"
            "The game is safe to play. Please report your game version to the mod author.", "Mad Max - Enhanced Archangels", MB_OK | MB_ICONWARNING);
        return TRUE;
    }
    for (int i = 0; i < TEXT_COUNT; i++) g_texts[i].id = Jenkins(g_texts[i].key);
    MH_Initialize();
    {
        uintptr_t site = g_sigs[SIG_GARAGESTATE].resolved;              // mov rax,[rip+disp32]
        g_garageStateVar = (void**)(site + 7 + *(int32_t*)(site + 3));
        bool patched = InstallGuiFillPatch(site);
        MH_STATUS e2 = patched ? MH_OK : MH_ERROR_NOT_EXECUTABLE;
        LogLine("garage screen limit fix: %s (g_GarageState at %p, SetArchetypeGuiData at %p)",
            patched ? "patched" : "FAILED", (void*)g_garageStateVar, (void*)SetArchetypeGuiData);
        int rows = DropzoneArchangelRows();
        LogLine("dropzone Archangel table: %s", rows < 0 ? "none (vanilla 16)" : "present");
        if (rows > GUI_ARCHANGEL_SLOTS) LogLine("  %d Archangels in the table", rows);
        if (e2 != MH_OK && rows > GUI_ARCHANGEL_SLOTS)
            MessageBoxA(NULL, "Enhanced Archangels could not install its garage fix, but an Archangel table with more than 16 "
                "entries is installed (dropzone\\vehicles\\archetypes.xlsc).\n\nThe garage WILL crash. Remove that file before playing.",
                "Mad Max - Enhanced Archangels", MB_OK | MB_ICONERROR);
    }
#if EA_DEV
    InstallScriptLog();                 // diagnostics: game/GUI script output and errors into this log
    CloseHandle(CreateThread(nullptr, 0, GuiScriptLogThread, nullptr, 0, nullptr));
#endif
    InstallInstallNodePatch();
    InstallArchangelBonus();
    MH_STATUS c = MH_CreateHook((LPVOID)g_sigs[SIG_LOCRAW].resolved, (LPVOID)LocRaw_hook, (LPVOID*)&LocRaw_orig);
    MH_STATUS e = (c == MH_OK) ? MH_EnableHook((LPVOID)g_sigs[SIG_LOCRAW].resolved) : c;
    LogLine("string lookup hook: %s (%d texts)", MH_StatusToString(e), TEXT_COUNT);
    return TRUE;
}
