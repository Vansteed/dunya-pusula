#include "DiskScanWorker.h"

void DiskScanWorker::tara()
{
    m_sonuc = DiskCleaner::tara();
    emit tamamlandi();
}
