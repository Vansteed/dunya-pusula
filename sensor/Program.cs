// SensorAjani - LibreHardwareMonitor'u sarmalayip CPU/GPU sensor verilerini
// tek satirlik JSON olarak stdout'a basan kucuk yardimci surec. C++ tarafi
// (SensorAjaniIstemcisi) bu ciktiyi QProcess ile okur. Alan yoksa null.
using System.Text.Json;
using LibreHardwareMonitor.Hardware;

namespace SensorAjani;

// LibreHardwareMonitor ornekleri gibi - alt donanimlari da gezmek icin.
internal sealed class UpdateVisitor : IVisitor
{
    public void VisitComputer(IComputer computer) => computer.Traverse(this);

    public void VisitHardware(IHardware hardware)
    {
        hardware.Update();
        foreach (IHardware sub in hardware.SubHardware) sub.Accept(this);
    }

    public void VisitSensor(ISensor sensor) { }
    public void VisitParameter(IParameter parameter) { }
}

internal sealed class SensorVerisi
{
    public double? cpuSicaklik { get; set; }
    public double? cpuKullanim { get; set; }
    public int? cpuMhz { get; set; }
    public string gpuAd { get; set; }
    public double? gpuSicaklik { get; set; }
    public double? gpuKullanim { get; set; }
    public int? gpuCekirdekMhz { get; set; }
    public int? gpuBellekMhz { get; set; }
    public double? gpuFanRpm { get; set; }
    public double? gpuBellekKullanilanMB { get; set; }
    public List<DiskVerisi> diskler { get; set; } = new();
}

internal sealed class DiskVerisi
{
    public string ad { get; set; }
    public double? sicaklikC { get; set; }
    public double? omurYuzde { get; set; }
    public double? yazilanGB { get; set; }
    public double? okunanGB { get; set; }
    public long? acilmaSayisi { get; set; }
    public long? calismaSaati { get; set; }
    public double? kullanilanYuzde { get; set; }
    public double? yedekYuzde { get; set; }
    public double? toplamGB { get; set; }
    public double? bosGB { get; set; }
}

internal static class Program
{
    private static ISensor BulSensor(IHardware hw, SensorType tur, Func<string, bool> adUyar)
    {
        ISensor secilen = null;
        foreach (ISensor s in hw.Sensors)
        {
            if (s.SensorType != tur || !s.Value.HasValue) continue;
            if (adUyar(s.Name)) return s;
            secilen ??= s;
        }
        return secilen;
    }

    private static ISensor EnYuksekSensor(IHardware hw, SensorType tur)
    {
        ISensor enYuksek = null;
        foreach (ISensor s in hw.Sensors)
        {
            if (s.SensorType != tur || !s.Value.HasValue) continue;
            if (enYuksek == null || s.Value > enYuksek.Value) enYuksek = s;
        }
        return enYuksek;
    }

    private static void CpuVerisiDoldur(IHardware cpu, SensorVerisi v)
    {
        ISensor sicaklik = BulSensor(cpu, SensorType.Temperature,
            ad => ad.Contains("Package") || ad.Contains("Tmax") || ad.Contains("Core Average"));
        sicaklik ??= EnYuksekSensor(cpu, SensorType.Temperature);
        v.cpuSicaklik = sicaklik?.Value;

        ISensor kullanim = BulSensor(cpu, SensorType.Load, ad => ad.Contains("Total"));
        v.cpuKullanim = kullanim?.Value;

        ISensor saat = EnYuksekSensor(cpu, SensorType.Clock);
        v.cpuMhz = saat?.Value != null ? (int)saat.Value.Value : null;
    }

    private static void GpuVerisiDoldur(IHardware gpu, SensorVerisi v)
    {
        v.gpuAd = gpu.Name;

        foreach (ISensor s in gpu.Sensors)
        {
            if (!s.Value.HasValue) continue;
            switch (s.SensorType)
            {
                case SensorType.Temperature when s.Name.Contains("GPU Core"):
                    v.gpuSicaklik = s.Value;
                    break;
                case SensorType.Clock when s.Name.Contains("GPU Core"):
                    v.gpuCekirdekMhz = (int)s.Value.Value;
                    break;
                case SensorType.Clock when s.Name.Contains("GPU Memory"):
                    v.gpuBellekMhz = (int)s.Value.Value;
                    break;
                case SensorType.Load when s.Name.Contains("GPU Core"):
                    v.gpuKullanim = s.Value;
                    break;
                case SensorType.Fan:
                    v.gpuFanRpm = s.Value;
                    break;
                case SensorType.SmallData when s.Name.Contains("GPU Memory Used"):
                    v.gpuBellekKullanilanMB = s.Value;
                    break;
            }
        }
    }

    private static DiskVerisi DiskVerisiDoldur(IHardware disk)
    {
        var d = new DiskVerisi { ad = disk.Name };
        foreach (ISensor s in disk.Sensors)
        {
            if (!s.Value.HasValue) continue;
            switch (s.SensorType)
            {
                case SensorType.Temperature when s.Name == "Composite Temperature":
                    d.sicaklikC = s.Value;
                    break;
                case SensorType.Level when s.Name == "Life":
                    d.omurYuzde = s.Value;
                    break;
                case SensorType.Data when s.Name == "Data Read":
                    d.okunanGB = s.Value;
                    break;
                case SensorType.Data when s.Name == "Data Written":
                    d.yazilanGB = s.Value;
                    break;
                case SensorType.Factor when s.Name == "Power On Count":
                    d.acilmaSayisi = (long)s.Value.Value;
                    break;
                case SensorType.Factor when s.Name == "Power On Hours":
                    d.calismaSaati = (long)s.Value.Value;
                    break;
                case SensorType.Level when s.Name == "Percentage Used":
                    d.kullanilanYuzde = s.Value;
                    break;
                case SensorType.Level when s.Name == "Available Spare":
                    d.yedekYuzde = s.Value;
                    break;
                case SensorType.Data when s.Name == "Total Space":
                    d.toplamGB = s.Value;
                    break;
                case SensorType.Data when s.Name == "Free Space":
                    d.bosGB = s.Value;
                    break;
            }
        }
        return d;
    }

    // Teshis modu: "SensorAjani.exe --dokum" tum donanim/sensor adlarini basar.
    private static void Dokum(Computer bilgisayar)
    {
        foreach (IHardware hw in bilgisayar.Hardware)
        {
            Console.WriteLine($"[{hw.HardwareType}] {hw.Name}");
            foreach (ISensor s in hw.Sensors)
                Console.WriteLine($"   {s.SensorType,-12} {s.Name,-40} {s.Value}");
            foreach (IHardware alt in hw.SubHardware)
            {
                Console.WriteLine($"   -- alt: {alt.Name}");
                foreach (ISensor s in alt.Sensors)
                    Console.WriteLine($"      {s.SensorType,-12} {s.Name,-40} {s.Value}");
            }
        }
    }

    private static int Main(string[] args)
    {
        var bilgisayar = new Computer
        {
            IsCpuEnabled = true,
            IsGpuEnabled = true,
            IsMotherboardEnabled = true,
            IsStorageEnabled = true,
        };
        bilgisayar.Open();

        var gezici = new UpdateVisitor();

        if (args.Length > 0 && args[0] == "--dokum")
        {
            bilgisayar.Accept(gezici);
            System.Threading.Thread.Sleep(1000);
            bilgisayar.Accept(gezici);
            Dokum(bilgisayar);
            Console.WriteLine("[Afterburner paylasimli bellek]");
            var ab = Afterburner.Oku();
            if (ab.Count == 0) Console.WriteLine("   (bos - Afterburner kapali ya da okunamadi)");
            foreach (var c in ab) Console.WriteLine($"   {c.Key,-40} {c.Value}");
            bilgisayar.Close();
            return 0;
        }

        while (true)
        {
            try
            {
                bilgisayar.Accept(gezici);

                var v = new SensorVerisi();

                foreach (IHardware hw in bilgisayar.Hardware)
                {
                    if (hw.HardwareType == HardwareType.Cpu)
                    {
                        CpuVerisiDoldur(hw, v);
                    }
                    else if (v.gpuAd == null &&
                             (hw.HardwareType == HardwareType.GpuNvidia ||
                              hw.HardwareType == HardwareType.GpuAmd ||
                              hw.HardwareType == HardwareType.GpuIntel))
                    {
                        GpuVerisiDoldur(hw, v);
                    }
                    else if (hw.HardwareType == HardwareType.Storage)
                    {
                        v.diskler.Add(DiskVerisiDoldur(hw));
                    }
                }

                // LibreHardwareMonitor CPU sicakligini veremediyse (surucu
                // yuklenemiyor) Afterburner paylasimli belleginden dene.
                if (v.cpuSicaklik == null)
                    v.cpuSicaklik = Afterburner.CpuSicakligi(Afterburner.Oku());

                Console.Out.WriteLine(JsonSerializer.Serialize(v));
                Console.Out.Flush();
            }
            catch (Exception)
            {
                // Beklenmeyen hata - uygulamayi cokertme, bir sn bekleyip devam et.
            }

            Thread.Sleep(1000);
        }
    }
}
