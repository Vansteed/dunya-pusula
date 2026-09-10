// DiskCleaner.h - Get-TemizlikHedefleri + Get-KlasorBoyut (moduller\10-disk.ps1)
// esdegeri. 7 hedef ayni sirada.
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

enum class HedefTuru { Klasor, Desen, GeriDonusum };

struct TemizlikHedefi {
    std::string ad;
    std::string yol;         // GeriDonusum icin bos
    HedefTuru tur;
    std::string desen;       // sadece Desen turunde dolu
};

struct TemizlikSonucSatiri {
    std::string konum;
    int64_t boyut = 0;
    int64_t sayi = 0;
    std::string durum;       // "Bulundu" / "Yok" / "Erişim engellendi - yönetici gerekli"
};

struct KlasorBoyutSonuc {
    int64_t boyut = 0;
    int64_t sayi = 0;
};

namespace DiskCleaner {

// 7 hedef, moduller\10-disk.ps1 Get-TemizlikHedefleri ile AYNI sira ve
// ortam degiskenleri. Not: GeriDonusum (Geri donusum kutusu) COM/Shell
// gerektirdigi icin bu katmanda TARANMAZ, UI/Qt katmaninda ele alinacak
// (STL-only bu katmanda kapsam disi) - burada sadece hedef listesinde yer alir,
// taramaAlHedefler onu "Yok"/0 olarak gecer.
std::vector<TemizlikHedefi> temizlikHedefleri();

// moduller\10-disk.ps1 Get-KlasorBoyut esdegeri. Yol yoksa {0,0} doner.
// Erisim engellenirse filesystem_error firlar (caller yakalar).
KlasorBoyutSonuc klasorBoyutu(const std::string &yol, const std::string &desen = "*");

// Tum hedefleri tarar, her biri icin klasorBoyutu cagirir; erisim hatasinda
// "Erişim engellendi - yönetici gerekli" durumunu satira yazar (istisna
// disariya firlamaz).
std::vector<TemizlikSonucSatiri> tara();

}
