// test_diskcleaner.cpp - moduller\10-disk.ps1 Get-KlasorBoyut senaryolarinin
// C++ birebir cevirisi. Framework gerekmez, assert() tabanli.
#include "core/DiskCleaner.h"

#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

int main()
{
    // Var olmayan yol -> boyut 0, sayi 0
    {
        auto r = DiskCleaner::klasorBoyutu("C:\\__bu_yol_kesinlikle_yok_12345__", "*");
        assert(r.boyut == 0);
        assert(r.sayi == 0);
    }

    // Bos yol string -> boyut 0
    {
        auto r = DiskCleaner::klasorBoyutu("", "*");
        assert(r.boyut == 0);
        assert(r.sayi == 0);
    }

    // Gecici bir klasor olustur, icine dosyalar koy, boyutu dogrula
    {
        fs::path tmp = fs::temp_directory_path() / "ram_temizleyici_test_dizin";
        std::error_code ec;
        fs::remove_all(tmp, ec);
        fs::create_directories(tmp);
        {
            std::ofstream f1(tmp / "a.txt", std::ios::binary);
            f1 << "12345"; // 5 bayt
        }
        {
            std::ofstream f2(tmp / "b.txt", std::ios::binary);
            f2 << "1234567890"; // 10 bayt
        }
        auto r = DiskCleaner::klasorBoyutu(tmp.string(), "*");
        assert(r.sayi == 2);
        assert(r.boyut == 15);
        fs::remove_all(tmp, ec);
    }

    // Desen filtresi: sadece .db uzantili dosyalar sayilir (thumbcache_*.db)
    {
        fs::path tmp = fs::temp_directory_path() / "ram_temizleyici_test_desen";
        std::error_code ec;
        fs::remove_all(tmp, ec);
        fs::create_directories(tmp);
        {
            std::ofstream f1(tmp / "thumbcache_100.db", std::ios::binary);
            f1 << "abcde"; // 5 bayt
        }
        {
            std::ofstream f2(tmp / "notes.txt", std::ios::binary);
            f2 << "should not count";
        }
        auto r = DiskCleaner::klasorBoyutu(tmp.string(), "thumbcache_*.db");
        assert(r.sayi == 1);
        assert(r.boyut == 5);
        fs::remove_all(tmp, ec);
    }

    // 7 hedef ayni sirada tanimli mi
    {
        auto hedefler = DiskCleaner::temizlikHedefleri();
        assert(hedefler.size() == 7);
        assert(hedefler[0].ad == "Kullanici gecici dosyalari");
        assert(hedefler[6].ad == "Geri donusum kutusu");
        assert(hedefler[6].tur == HedefTuru::GeriDonusum);
    }

    std::printf("test_diskcleaner: TUM TESTLER GECTI\n");
    return 0;
}
