# Dünya Pusula

Windows için sistem izleme ve disk analiz aracı. Bellek, işlemci/ekran kartı
sensörleri, disk sağlığı (SMART) ve disk alanı analizi tek pencerede.

Qt 6 / QML ile yazılmış, native Win32 API kullanır. Donanım sensörleri için
arka planda LibreHardwareMonitor tabanlı ayrı bir süreç çalışır.

## Ekranlar

| Sekme | İçerik |
|---|---|
| **Panel** | Dört kartlık özet: bellek, işlemci/GPU, sağlık puanı, depolama kırılımı |
| **Bellek** | RAM kullanımı, süreç listesi, seçilenleri kapatma, öncelik değiştirme |
| **İşlemci** | CPU kullanım/frekans/sıcaklık, GPU adı/kullanım/sıcaklık/saat, canlı grafikler |
| **Sağlık** | Disk SMART verileri, aşınma, sıcaklık, güç açık saati; 100 üzerinden sistem sağlık puanı |
| **Büyük Dosyalar** | Paralel disk tarama, klasör ağacı, tür bazlı dosya arama, silme işlemleri |

## Öne çıkan teknik noktalar

- **Hızlı tarama** — `FindFirstFileExW` + `FIND_FIRST_EX_LARGE_FETCH` ile
  çekirdek sayısı kadar iş parçacığında paralel gezinti. Tek taramada tüm
  dizin ağacı bellekte kurulur, düğüm açmak yeniden tarama gerektirmez.
- **Evrensel sensör desteği** — Intel/AMD işlemciler ve NVIDIA/AMD ekran
  kartları. Üreticiye özel DLL ayrıştırma yok; LibreHardwareMonitorLib
  kullanan bir .NET yan süreci JSON satırlarıyla veri akıtır.
- **Silme koruması** — `Windows`, `Program Files`, kullanıcı profili kökü,
  `pagefile.sys` gibi hedefler silme yolunun en alt katmanında engellenir;
  program dosyaları için ayrıca uyarı gösterilir.
- **Disk sağlığı** — NVMe SMART sağlık günlüğü doğrudan IOCTL ile okunur.

## Derleme

Gereksinimler:

- Qt 6.11 (varsayılan yol: `C:/qtvcpkg/installed/x64-windows`)
- Visual Studio 2022+ Build Tools (MSVC, CMake, Ninja)
- .NET 8 SDK (sensör ajanı için)
- Inno Setup 7 (kurulum dosyası üretmek için, isteğe bağlı)

```bat
derle.bat
```

Sensör ajanını yayınlamak için:

```bat
cd sensor && yayinla.bat
```

Kurulum dosyası üretmek için:

```bat
cd kurulum && kur.bat
```

## Bilinen kısıtlar

- Yalnızca Windows x64.
- Sensör ajanı çekirdek sürücüsü yüklediği için uygulama yönetici hakkıyla
  çalışır. Kurulum yolu boşluk içerdiğinde sürücü kaydı başarısız olduğundan
  ajan `C:\ProgramData\DunyaPusula` altına kopyalanıp oradan çalıştırılır.
- Ölçülen toplam disk kullanımı, NTFS sabit bağları (özellikle `WinSxS`) ve
  sıkıştırılmış dosyalar nedeniyle gerçek dolu alandan bir miktar yüksek
  çıkabilir. Aynı davranış WinDirStat/FolderSize gibi araçlarda da vardır.

## Lisans

MIT — bkz. [LICENSE](LICENSE).

Sensör ajanı [LibreHardwareMonitorLib](https://github.com/LibreHardwareMonitor/LibreHardwareMonitor)
(MPL-2.0) kullanır.
