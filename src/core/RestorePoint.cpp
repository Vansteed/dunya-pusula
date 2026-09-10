#include "RestorePoint.h"

#include <windows.h>
#include <srrestoreptapi.h>

namespace RestorePoint {

bool yoneticiMi()
{
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) return false;

    TOKEN_ELEVATION elev;
    DWORD boyut = sizeof(elev);
    bool sonuc = false;
    if (GetTokenInformation(token, TokenElevation, &elev, sizeof(elev), &boyut)) {
        sonuc = elev.TokenIsElevated != 0;
    }
    CloseHandle(token);
    return sonuc;
}

RestorePointSonuc olustur(const std::string &aciklama)
{
    RestorePointSonuc sonuc;

    if (!yoneticiMi()) {
        sonuc.basarili = false;
        sonuc.mesaj = "Yonetici yetkisi gerekli.";
        return sonuc;
    }

    // Import lib genelde bulunmadigi icin dinamik yukleme (plan 2.9 geregi).
    HMODULE srclient = LoadLibraryW(L"srclient.dll");
    if (!srclient) {
        sonuc.basarili = false;
        sonuc.mesaj = "Sistem korumasi kapali. Denetim Masasi > Sistem > Sistem korumasi'ndan ac.";
        return sonuc;
    }

    using SRSetRestorePointWFn = BOOL(WINAPI *)(PRESTOREPOINTINFOW, PSTATEMGRSTATUS);
    auto fn = reinterpret_cast<SRSetRestorePointWFn>(GetProcAddress(srclient, "SRSetRestorePointW"));
    if (!fn) {
        FreeLibrary(srclient);
        sonuc.basarili = false;
        sonuc.mesaj = "Sistem korumasi kapali. Denetim Masasi > Sistem > Sistem korumasi'ndan ac.";
        return sonuc;
    }

    RESTOREPOINTINFOW bilgi{};
    bilgi.dwEventType = BEGIN_SYSTEM_CHANGE;
    bilgi.dwRestorePtType = MODIFY_SETTINGS;
    bilgi.llSequenceNumber = 0;
    std::wstring waciklama(aciklama.begin(), aciklama.end());
    wcsncpy_s(bilgi.szDescription, waciklama.c_str(), _TRUNCATE);

    STATEMGRSTATUS durum{};
    const BOOL basarili = fn(&bilgi, &durum);
    FreeLibrary(srclient);

    if (basarili) {
        sonuc.basarili = true;
        sonuc.mesaj = "Geri yukleme noktasi olusturuldu.";
    } else {
        sonuc.basarili = false;
        sonuc.mesaj = "Sistem korumasi kapali. Denetim Masasi > Sistem > Sistem korumasi'ndan ac.";
    }
    return sonuc;
}

}
