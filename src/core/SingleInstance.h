// SingleInstance.h - RamTemizleyici.ps1 satir 10 ile AYNI mutex adi
// ("Global\RamTemizleyiciPRO"), boylece iki surum birbirini engeller.
#pragma once

#include <windows.h>

class SingleInstance {
public:
    SingleInstance();
    ~SingleInstance();

    SingleInstance(const SingleInstance &) = delete;
    SingleInstance &operator=(const SingleInstance &) = delete;

    // true: bu surecin ilk ornek oldugu (mutex edinildi), false: baska bir
    // ornek zaten calisiyor.
    bool edinildiMi() const { return edinildi_; }

private:
    HANDLE mutex_ = nullptr;
    bool edinildi_ = false;
};
