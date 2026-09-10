// DosyaIslem.h - Buyuk Dosyalar sayfasindaki sag tik menusunun Win32
// islemleri (sil / geri donusum / explorer / ozellikler / cmd). Kullaniciya
// gorunen metin YOK - onay/hata mesajlari QML tarafinda.
#pragma once

#include <QString>

namespace DosyaIslem {

// Klasor veya dosya - Geri Donusum Kutusu'na tasir (SHFileOperationW + FOF_ALLOWUNDO).
bool geriDonusumeGonder(const QString &yol);

// Klasor veya dosya - kalici siler (geri alinamaz).
bool kaliciSil(const QString &yol);

// Yol silinmesi sistemi bozacak korunmus bir hedef mi? Bos QString = serbest,
// dolu QString = kullaniciya gosterilecek engel sebebi.
QString korumaSebebi(const QString &yol);

// Silinmesi tehlikeli ama yasak degil (ör. Program Files altindaki dosyalar).
// Bos QString = uyari yok.
QString riskUyarisi(const QString &yol);

// Dosya ise Explorer'da secili acar (/select,), klasor ise klasoru acar.
bool explorerdaGoster(const QString &yol);

// Ust klasoru acar ve ogeyi SECILI getirir (explorer /select).
// explorerdaGoster klasorleri icinden acar, bu ise konumunu gosterir.
bool konumunuGoster(const QString &yol);

// Windows "Ozellikler" penceresini acar.
bool ozellikleriGoster(const QString &yol);

// Verilen klasorde cmd.exe penceresi acar.
bool komutIstemiAc(const QString &klasorYolu);

}
