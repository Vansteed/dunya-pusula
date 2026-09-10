#include "CleanWorker.h"

#include "core/MemoryCleaner.h"

#include <thread>
#include <chrono>

void CleanWorker::calistir()
{
    // Adim 1: hazirlik. PowerShell surumunde .NET GC.Collect() burada
    // calisiyor - native C++'ta yonetilen yigin olmadigi icin dogrudan
    // karsiligi yok, kisa bir hazirlik adimiyla ayni 3-adim akisi korunur.
    emit adimBasladi(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(350));
    emit adimBitti(1, "[1/3] Hazırlık tamam.");

    // Adim 2: WorkingSet kirpma.
    emit adimBasladi(2);
    const int n = MemoryCleaner::tumWorkingSetleriKirp();
    emit adimBitti(2, QString("[2/3] %1 işlem kırpıldı.").arg(n));

    // Adim 3: standby bellek bosaltma.
    emit adimBasladi(3);
    const int res = MemoryCleaner::standbyBosalt();
    if (res == 0) {
        emit adimBitti(3, "[3/3] Standby bellek boşaltıldı.");
    } else {
        emit adimBitti(3, QString("[3/3] Standby atlandı (kod %1). Yönetici modda tam temizler.").arg(res));
    }

    emit tamamlandi();
}
