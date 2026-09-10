#include "SingleInstance.h"

SingleInstance::SingleInstance()
{
    mutex_ = CreateMutexW(nullptr, FALSE, L"Global\\RamTemizleyiciPRO");
    edinildi_ = (mutex_ != nullptr) && (GetLastError() != ERROR_ALREADY_EXISTS);
}

SingleInstance::~SingleInstance()
{
    if (mutex_) {
        if (edinildi_) ReleaseMutex(mutex_);
        CloseHandle(mutex_);
    }
}
