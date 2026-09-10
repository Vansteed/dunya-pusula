// Afterburner.cs - MSI Afterburner'in "MAHMSharedMemory" paylasimli bellegini
// okur. Neden gerekli: Intel CPU sicakligi MSR'den okunur, bu da cekirdek
// surucusu ister. LibreHardwareMonitor'un WinRing0 surucusu guncel Windows'ta
// yuklenmiyor (zafiyetli surucu engelleme listesi). Afterburner KENDI imzali
// surucusuyle ayni degeri zaten okuyup paylasimli bellege koyuyor - biz
// oradan bedava aliriz, kendi surucumuzu kurmamiza gerek kalmaz.
// Afterburner kapaliysa tum degerler null doner, bu normal bir durumdur.
using System.IO.MemoryMappedFiles;
using System.Text;

namespace SensorAjani;

internal static class Afterburner
{
    private const string BellekAdi = "MAHMSharedMemory";
    private const int MaxPath = 260;

    // MAHM_SHARED_MEMORY_ENTRY icindeki 5 adet char[260] alanin toplam boyutu.
    private const int MetinAlanlari = MaxPath * 5;

    // Bir kaynak adi -> son okunan deger.
    public static Dictionary<string, float> Oku()
    {
        var sonuc = new Dictionary<string, float>(StringComparer.OrdinalIgnoreCase);
        try
        {
            using MemoryMappedFile mmf =
                MemoryMappedFile.OpenExisting(BellekAdi, MemoryMappedFileRights.Read);
            using MemoryMappedViewAccessor gorunum = mmf.CreateViewAccessor(0, 0, MemoryMappedFileAccess.Read);

            uint imza = gorunum.ReadUInt32(0);
            // 'MAHM' imzasi; 0 ise Afterburner kapaniyor demektir.
            if (imza == 0) return sonuc;

            uint baslikBoyutu = gorunum.ReadUInt32(8);
            uint girdiSayisi = gorunum.ReadUInt32(12);
            uint girdiBoyutu = gorunum.ReadUInt32(16);

            // Akil kontrolu - bozuk basliktan devasa dongu cikmasin.
            if (baslikBoyutu == 0 || girdiBoyutu < MetinAlanlari + 4 ||
                girdiSayisi == 0 || girdiSayisi > 1000) {
                return sonuc;
            }

            var tampon = new byte[MaxPath];
            for (uint i = 0; i < girdiSayisi; i++)
            {
                long taban = baslikBoyutu + (long)i * girdiBoyutu;
                gorunum.ReadArray(taban, tampon, 0, MaxPath);

                int uzunluk = Array.IndexOf(tampon, (byte)0);
                if (uzunluk <= 0) continue;
                string ad = Encoding.Default.GetString(tampon, 0, uzunluk);

                // data alani 5 metin alanindan hemen sonra gelir.
                float deger = gorunum.ReadSingle(taban + MetinAlanlari);
                sonuc[ad] = deger;
            }
        }
        catch (Exception)
        {
            // Afterburner kapali / erisim yok - sessizce bos don.
        }
        return sonuc;
    }

    // Afterburner sonuclarindan CPU sicakligini sec. Isimler surume gore
    // degisebildigi icin birkac aday denenir; makul aralik disi deger atilir.
    public static double? CpuSicakligi(Dictionary<string, float> veriler)
    {
        string[] adaylar = { "CPU temperature", "CPU1 temperature", "CPU temp" };
        foreach (string ad in adaylar)
        {
            if (veriler.TryGetValue(ad, out float d) && d > 0 && d < 125) return d;
        }
        return null;
    }
}
