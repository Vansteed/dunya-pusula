// MemoryCleaner.h - EmptyWorkingSet + NtSetSystemInformation standby purge
// (RamTemizleyici.ps1 satir 29-52 P/Invoke tanimlarinin birebir karsiligi).
#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace MemoryCleaner {

// RamTemizleyici.ps1 satir 65-72 ile AYNI liste, kucuk harf.
extern const std::vector<std::string> ProtectedList;

bool korumaliMi(const std::string &islemAdiKucukHarf);

// Verilen PID'nin EmptyWorkingSet cagrisi. Korumali listede ise dokunmaz,
// false doner.
bool workingSetBosalt(uint32_t pid, const std::string &islemAdi);

// Tum acik islemler icin WorkingSet kirpar (korumalilar haric), kirpilan
// islem sayisini doner.
int tumWorkingSetleriKirp();

// NtSetSystemInformation(80, cmd=4) ile standby bellek bosaltma.
// Basarili ise 0 doner (PowerShell'deki $res ile ayni anlam).
int standbyBosalt();

}
