#include "MemoryCleaner.h"

#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <algorithm>
#include <cctype>

namespace MemoryCleaner {

// RamTemizleyici.ps1 satir 65-72 ile AYNI liste, kucuk harf.
const std::vector<std::string> ProtectedList = {
    "system", "registry", "smss", "csrss", "wininit", "services", "lsass", "lsaiso",
    "winlogon", "dwm", "fontdrvhost", "conhost", "svchost", "sihost", "taskhostw",
    "searchhost", "startmenuexperiencehost", "shellexperiencehost", "textinputhost",
    "runtimebroker", "applicationframehost", "explorer", "powershell", "pwsh",
    "ramtemizleyici", "memory compression", "secure system", "idle", "audiodg",
    "spoolsv", "ctfmon", "rdpclip", "msmpeng", "nissrv", "securityhealthservice"
};

namespace {
std::string kucukHarf(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}
}

bool korumaliMi(const std::string &islemAdiKucukHarf)
{
    return std::find(ProtectedList.begin(), ProtectedList.end(), islemAdiKucukHarf)
        != ProtectedList.end();
}

bool workingSetBosalt(uint32_t pid, const std::string &islemAdi)
{
    if (korumaliMi(kucukHarf(islemAdi))) return false;

    HANDLE h = OpenProcess(PROCESS_SET_QUOTA | PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!h) return false;
    const bool ok = EmptyWorkingSet(h) != 0;
    CloseHandle(h);
    return ok;
}

int tumWorkingSetleriKirp()
{
    int n = 0;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(pe);
    if (Process32FirstW(snap, &pe)) {
        do {
            std::wstring wname(pe.szExeFile);
            std::string name(wname.begin(), wname.end());
            // Uzanti ayikla ("explorer.exe" -> "explorer") - Get-Process ile ayni.
            const auto dot = name.rfind('.');
            if (dot != std::string::npos) name = name.substr(0, dot);
            if (korumaliMi(kucukHarf(name))) continue;

            HANDLE h = OpenProcess(PROCESS_SET_QUOTA | PROCESS_QUERY_INFORMATION, FALSE, pe.th32ProcessID);
            if (h) {
                if (EmptyWorkingSet(h)) ++n;
                CloseHandle(h);
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return n;
}

int standbyBosalt()
{
    using NtSetSystemInformationFn = LONG(WINAPI *)(int, PVOID, ULONG);

    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (!ntdll) return -1;
    auto fn = reinterpret_cast<NtSetSystemInformationFn>(
        GetProcAddress(ntdll, "NtSetSystemInformation"));
    if (!fn) return -1;

    int cmd = 4;
    return static_cast<int>(fn(80, &cmd, sizeof(cmd)));
}

}
