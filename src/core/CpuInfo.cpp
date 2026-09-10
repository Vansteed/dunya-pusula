#include "CpuInfo.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <powrprof.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <comdef.h>
#include <Wbemidl.h>
#include <gl/GL.h>
#include <d3d11.h>

#include <algorithm>
#include <cwctype>
#include <string>
#include <vector>

// PROCESSOR_POWER_INFORMATION - CallNtPowerInformation(ProcessorInformation, ...)
// icin gereken yapi; Windows SDK'de herkese acik bir baslikta tanimli degil
// (MSDN ornekleri de ayni sekilde elle tanimlar).
typedef struct _PROCESSOR_POWER_INFORMATION {
    ULONG Number;
    ULONG MaxMhz;
    ULONG CurrentMhz;
    ULONG MhzLimit;
    ULONG MaxIdleState;
    ULONG CurrentIdleState;
} PROCESSOR_POWER_INFORMATION, *PPROCESSOR_POWER_INFORMATION;

namespace {

// Anlik CPU frekansi. CallNtPowerInformation modern Windows'ta genelde
// nominal (temel) frekansi doner, turbo/idle durumunu yansitmaz. Gorev
// Yoneticisi de bu yuzden "% Processor Performance" sayacini kullanir:
// anlik MHz = temel MHz * yuzde / 100. Sayac yoksa 0 doner.
double islemciPerformansYuzdesi()
{
    static PDH_HQUERY sorgu = nullptr;
    static PDH_HCOUNTER sayac = nullptr;
    static bool kuruldu = false;
    static bool ilkToplamaYapildi = false;

    if (!kuruldu) {
        kuruldu = true;
        if (PdhOpenQueryW(nullptr, 0, &sorgu) != ERROR_SUCCESS) {
            sorgu = nullptr;
        } else if (PdhAddEnglishCounterW(
                       sorgu, L"\\Processor Information(_Total)\\% Processor Performance",
                       0, &sayac) != ERROR_SUCCESS) {
            PdhCloseQuery(sorgu);
            sorgu = nullptr;
        }
    }
    if (!sorgu) return 0.0;

    if (PdhCollectQueryData(sorgu) != ERROR_SUCCESS) return 0.0;
    if (!ilkToplamaYapildi) {
        // Ilk toplama sadece taban olusturur, deger henuz anlamsiz.
        ilkToplamaYapildi = true;
        return 0.0;
    }

    PDH_FMT_COUNTERVALUE deger{};
    if (PdhGetFormattedCounterValue(sayac, PDH_FMT_DOUBLE, nullptr, &deger) != ERROR_SUCCESS)
        return 0.0;
    return deger.doubleValue;
}

// GPU 3D motor kullanimi - wildcard sayac ile TUM GPU Engine ornekleri
// (islem+motor basina bir tane) toplanir. Sayac yoksa/hata olursa 0.0 doner.
double gpuKullanimYuzdesiHesapla()
{
    static PDH_HQUERY sorgu = nullptr;
    static PDH_HCOUNTER sayac = nullptr;
    static bool kuruldu = false;
    static bool ilkToplamaYapildi = false;

    if (!kuruldu) {
        kuruldu = true;
        if (PdhOpenQueryW(nullptr, 0, &sorgu) != ERROR_SUCCESS) {
            sorgu = nullptr;
        } else if (PdhAddEnglishCounterW(
                       sorgu, L"\\GPU Engine(*engtype_3D)\\Utilization Percentage",
                       0, &sayac) != ERROR_SUCCESS) {
            PdhCloseQuery(sorgu);
            sorgu = nullptr;
        }
    }
    if (!sorgu) return 0.0;

    if (PdhCollectQueryData(sorgu) != ERROR_SUCCESS) return 0.0;
    if (!ilkToplamaYapildi) {
        // Ilk toplama sadece taban olusturur, deger henuz anlamsiz.
        ilkToplamaYapildi = true;
        return 0.0;
    }

    DWORD boyut = 0;
    DWORD ornekSayisi = 0;
    PDH_STATUS durum = PdhGetFormattedCounterArrayW(sayac, PDH_FMT_DOUBLE, &boyut, &ornekSayisi, nullptr);
    if (durum != PDH_MORE_DATA || boyut == 0) return 0.0;

    std::vector<BYTE> arabellek(boyut);
    auto *ornekler = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W *>(arabellek.data());
    if (PdhGetFormattedCounterArrayW(sayac, PDH_FMT_DOUBLE, &boyut, &ornekSayisi, ornekler) != ERROR_SUCCESS)
        return 0.0;

    double toplam = 0.0;
    for (DWORD i = 0; i < ornekSayisi; ++i) {
        if (ornekler[i].FmtValue.CStatus == ERROR_SUCCESS) toplam += ornekler[i].FmtValue.doubleValue;
    }
    return std::min(toplam, 100.0);
}

// GetSystemTimes olcumu arasi fark ile kullanim yuzdesi - ilk cagride 0.0
// donmesi icin oncekiToplam == 0 kontrolu yeterli.
struct ZamanOlcum {
    ULARGE_INTEGER idle{};
    ULARGE_INTEGER kernel{};
    ULARGE_INTEGER user{};
};

double kullanimYuzdesiHesapla()
{
    static ZamanOlcum onceki{};
    static bool ilkOlcumVarMi = false;

    FILETIME bosFt{}, kernelFt{}, userFt{};
    if (!GetSystemTimes(&bosFt, &kernelFt, &userFt)) return 0.0;

    ZamanOlcum simdi;
    simdi.idle.LowPart = bosFt.dwLowDateTime;
    simdi.idle.HighPart = bosFt.dwHighDateTime;
    simdi.kernel.LowPart = kernelFt.dwLowDateTime;
    simdi.kernel.HighPart = kernelFt.dwHighDateTime;
    simdi.user.LowPart = userFt.dwLowDateTime;
    simdi.user.HighPart = userFt.dwHighDateTime;

    double yuzde = 0.0;
    if (ilkOlcumVarMi) {
        const ULONGLONG idleFark = simdi.idle.QuadPart - onceki.idle.QuadPart;
        const ULONGLONG toplamFark = (simdi.kernel.QuadPart - onceki.kernel.QuadPart) +
                                      (simdi.user.QuadPart - onceki.user.QuadPart);
        if (toplamFark > 0) {
            yuzde = (1.0 - static_cast<double>(idleFark) / static_cast<double>(toplamFark)) * 100.0;
            if (yuzde < 0.0) yuzde = 0.0;
            if (yuzde > 100.0) yuzde = 100.0;
        }
    }
    onceki = simdi;
    ilkOlcumVarMi = true;
    return yuzde;
}

std::string registryOku(HKEY kok, const wchar_t *yol, const wchar_t *deger)
{
    HKEY h;
    if (RegOpenKeyExW(kok, yol, 0, KEY_READ, &h) != ERROR_SUCCESS) return {};
    wchar_t arabellek[512] = {0};
    DWORD boyut = sizeof(arabellek);
    DWORD tur = 0;
    std::string sonuc;
    if (RegQueryValueExW(h, deger, nullptr, &tur, reinterpret_cast<LPBYTE>(arabellek), &boyut) == ERROR_SUCCESS
        && tur == REG_SZ) {
        _bstr_t bs(arabellek);
        sonuc = static_cast<const char *>(bs);
    }
    RegCloseKey(h);
    return sonuc;
}

std::optional<DWORD> registryOkuDword(HKEY kok, const wchar_t *yol, const wchar_t *deger)
{
    HKEY h;
    if (RegOpenKeyExW(kok, yol, 0, KEY_READ, &h) != ERROR_SUCCESS) return std::nullopt;
    DWORD veri = 0;
    DWORD boyut = sizeof(veri);
    DWORD tur = 0;
    std::optional<DWORD> sonuc;
    if (RegQueryValueExW(h, deger, nullptr, &tur, reinterpret_cast<LPBYTE>(&veri), &boyut) == ERROR_SUCCESS
        && tur == REG_DWORD) {
        sonuc = veri;
    }
    RegCloseKey(h);
    return sonuc;
}

std::optional<ULONGLONG> registryOkuQword(HKEY kok, const wchar_t *yol, const wchar_t *deger)
{
    HKEY h;
    if (RegOpenKeyExW(kok, yol, 0, KEY_READ, &h) != ERROR_SUCCESS) return std::nullopt;
    ULONGLONG veri = 0;
    DWORD boyut = sizeof(veri);
    DWORD tur = 0;
    std::optional<ULONGLONG> sonuc;
    if (RegQueryValueExW(h, deger, nullptr, &tur, reinterpret_cast<LPBYTE>(&veri), &boyut) == ERROR_SUCCESS
        && tur == REG_QWORD) {
        sonuc = veri;
    }
    RegCloseKey(h);
    return sonuc;
}

int fizikselCekirdekSayisi(int mantiksalYedek)
{
    DWORD boyut = 0;
    GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &boyut);
    if (boyut == 0) return mantiksalYedek;

    std::vector<BYTE> arabellek(boyut);
    auto *veri = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(arabellek.data());
    if (!GetLogicalProcessorInformationEx(RelationProcessorCore, veri, &boyut)) return mantiksalYedek;

    int sayi = 0;
    DWORD ofset = 0;
    while (ofset < boyut) {
        auto *giris = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(arabellek.data() + ofset);
        if (giris->Relationship == RelationProcessorCore) ++sayi;
        ofset += giris->Size;
    }
    return sayi > 0 ? sayi : mantiksalYedek;
}

// Sicaklik - root\WMI / MSAcpi_ThermalZoneTemperature. Cogu masaustunde
// veri gelmez (donanim ACPI thermal zone bildirmez) - bu NORMAL, hata degil.
std::optional<double> sicaklikOku()
{
    std::optional<double> sonuc;
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    const bool comBaslatildi = SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE;
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) return sonuc;

    try {
        hr = CoInitializeSecurity(nullptr, -1, nullptr, nullptr,
                                   RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE,
                                   nullptr, EOAC_NONE, nullptr);
        // Zaten baslatilmis olabilir (RPC_E_TOO_LATE) - yok say.

        IWbemLocator *konumlayici = nullptr;
        hr = CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER,
                               IID_IWbemLocator, reinterpret_cast<LPVOID *>(&konumlayici));
        if (SUCCEEDED(hr) && konumlayici) {
            IWbemServices *servis = nullptr;
            hr = konumlayici->ConnectServer(_bstr_t(L"ROOT\\WMI"), nullptr, nullptr, nullptr,
                                             0, nullptr, nullptr, &servis);
            if (SUCCEEDED(hr) && servis) {
                CoSetProxyBlanket(servis, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr,
                                   RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);

                IEnumWbemClassObject *enumerator = nullptr;
                hr = servis->ExecQuery(_bstr_t(L"WQL"),
                                        _bstr_t(L"SELECT CurrentTemperature FROM MSAcpi_ThermalZoneTemperature"),
                                        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                                        nullptr, &enumerator);
                if (SUCCEEDED(hr) && enumerator) {
                    IWbemClassObject *nesne = nullptr;
                    ULONG donen = 0;
                    if (enumerator->Next(WBEM_INFINITE, 1, &nesne, &donen) == WBEM_S_NO_ERROR && donen > 0) {
                        VARIANT deger;
                        VariantInit(&deger);
                        if (SUCCEEDED(nesne->Get(L"CurrentTemperature", 0, &deger, nullptr, nullptr))
                            && deger.vt == VT_I4) {
                            sonuc = static_cast<double>(deger.lVal) / 10.0 - 273.15;
                        }
                        VariantClear(&deger);
                        nesne->Release();
                    }
                    enumerator->Release();
                }
                servis->Release();
            }
            konumlayici->Release();
        }
    } catch (const std::exception &) {
        sonuc = std::nullopt;
    } catch (...) {
        sonuc = std::nullopt;
    }

    if (comBaslatildi) CoUninitialize();
    return sonuc;
}

// Gecici gizli pencere + WGL baglami ile OpenGL surum dizesini okur.
// Baglam/pencere her kosulda temizlenir.
std::string openglVersiyonuOku()
{
    std::string sonuc;
    try {
        WNDCLASSW wc{};
        wc.lpfnWndProc = DefWindowProcW;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.lpszClassName = L"RamTemizleyiciGeciciGL";
        RegisterClassW(&wc);

        HWND pencere = CreateWindowW(wc.lpszClassName, L"", WS_POPUP, 0, 0, 1, 1,
                                      nullptr, nullptr, wc.hInstance, nullptr);
        if (pencere) {
            HDC dc = GetDC(pencere);
            if (dc) {
                PIXELFORMATDESCRIPTOR pfd{};
                pfd.nSize = sizeof(pfd);
                pfd.nVersion = 1;
                pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
                pfd.iPixelType = PFD_TYPE_RGBA;
                pfd.cColorBits = 32;

                const int format = ChoosePixelFormat(dc, &pfd);
                if (format != 0 && SetPixelFormat(dc, format, &pfd)) {
                    HGLRC baglam = wglCreateContext(dc);
                    if (baglam && wglMakeCurrent(dc, baglam)) {
                        if (const GLubyte *surum = glGetString(GL_VERSION)) {
                            sonuc = reinterpret_cast<const char *>(surum);
                        }
                        wglMakeCurrent(nullptr, nullptr);
                    }
                    if (baglam) wglDeleteContext(baglam);
                }
                ReleaseDC(pencere, dc);
            }
            DestroyWindow(pencere);
        }
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
    } catch (const std::exception &) {
        sonuc.clear();
    }
    return sonuc;
}

// vulkan-1.dll dinamik yuklenir - Vulkan SDK basligi kullanilmaz.
std::string vulkanVersiyonuOku()
{
    std::string sonuc;
    try {
        HMODULE kutuphane = LoadLibraryW(L"vulkan-1.dll");
        if (!kutuphane) return sonuc;

        using VkEnumerateInstanceVersionFn = int (__stdcall *)(uint32_t *);
        auto fn = reinterpret_cast<VkEnumerateInstanceVersionFn>(
            GetProcAddress(kutuphane, "vkEnumerateInstanceVersion"));
        if (fn) {
            uint32_t surum = 0;
            if (fn(&surum) == 0 /* VK_SUCCESS */) {
                const uint32_t major = (surum >> 22) & 0x7F;
                const uint32_t minor = (surum >> 12) & 0x3FF;
                const uint32_t patch = surum & 0xFFF;
                sonuc = std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
            }
        }
        FreeLibrary(kutuphane);
    } catch (const std::exception &) {
        sonuc.clear();
    }
    return sonuc;
}

// D3D11CreateDevice ile en yuksek desteklenen feature level'a gore shader
// model karsiligini dondurur.
std::string shaderModelOku()
{
    std::string sonuc;
    try {
        static const D3D_FEATURE_LEVEL istenenler[] = {
            D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0,
            D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0,
        };
        D3D_FEATURE_LEVEL alinan{};
        ID3D11Device *aygit = nullptr;
        ID3D11DeviceContext *baglam = nullptr;
        HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
                                        istenenler, static_cast<UINT>(std::size(istenenler)),
                                        D3D11_SDK_VERSION, &aygit, &alinan, &baglam);
        if (SUCCEEDED(hr) && aygit) {
            switch (alinan) {
            case D3D_FEATURE_LEVEL_12_1:
            case D3D_FEATURE_LEVEL_12_0:
                sonuc = "6.0+";
                break;
            case D3D_FEATURE_LEVEL_11_1:
                sonuc = "5.1";
                break;
            case D3D_FEATURE_LEVEL_11_0:
                sonuc = "5.0";
                break;
            default:
                sonuc = "4.0";
                break;
            }
        }
        if (baglam) baglam->Release();
        if (aygit) aygit->Release();
    } catch (const std::exception &) {
        sonuc.clear();
    }
    return sonuc;
}

}

namespace CpuInfo {

CpuBilgisi cpuBilgisi()
{
    CpuBilgisi c;

    SYSTEM_INFO si;
    GetSystemInfo(&si);
    c.mantiksalCekirdek = static_cast<int>(si.dwNumberOfProcessors);
    c.cekirdek = fizikselCekirdekSayisi(c.mantiksalCekirdek);

    c.ad = registryOku(HKEY_LOCAL_MACHINE,
                        L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", L"ProcessorNameString");
    if (const auto temelMhz = registryOkuDword(HKEY_LOCAL_MACHINE,
            L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", L"~MHz")) {
        c.mhz = static_cast<int>(*temelMhz);
        c.maxMhz = c.mhz;
    }

    // Anlik/maks frekans - CallNtPowerInformation basarisiz olursa registry
    // ~MHz degeri (yukarida atandi) kalir.
    std::vector<PROCESSOR_POWER_INFORMATION> guc(std::max(1, c.mantiksalCekirdek));
    int temelMhzDegeri = c.mhz;
    if (CallNtPowerInformation(ProcessorInformation, nullptr, 0, guc.data(),
                                static_cast<ULONG>(guc.size() * sizeof(PROCESSOR_POWER_INFORMATION))) == 0) {
        c.mhz = static_cast<int>(guc[0].CurrentMhz);
        c.maxMhz = static_cast<int>(guc[0].MaxMhz);
        if (guc[0].MaxMhz > 0) temelMhzDegeri = static_cast<int>(guc[0].MaxMhz);
    }

    // PDH sayaci varsa anlik frekansi ondan al (turbo'yu de gosterir).
    const double perfYuzde = islemciPerformansYuzdesi();
    if (perfYuzde > 0.0 && temelMhzDegeri > 0) {
        c.mhz = static_cast<int>(temelMhzDegeri * perfYuzde / 100.0);
        c.maxMhz = std::max(c.maxMhz, c.mhz);
    }

    c.kullanimYuzde = kullanimYuzdesiHesapla();
    c.sicaklikC = sicaklikOku();

    return c;
}

std::vector<GpuBilgisi> gpuBilgisi()
{
    std::vector<GpuBilgisi> sonuc;
    const wchar_t *sinifYolu = L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}";

    HKEY sinifAnahtar;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, sinifYolu, 0, KEY_READ, &sinifAnahtar) != ERROR_SUCCESS) {
        return sonuc;
    }

    for (DWORD i = 0;; ++i) {
        wchar_t altAd[16] = {0};
        DWORD altAdBoyut = 16;
        if (RegEnumKeyExW(sinifAnahtar, i, altAd, &altAdBoyut, nullptr, nullptr, nullptr, nullptr)
            != ERROR_SUCCESS) {
            break;
        }
        // Alt anahtarlar "0000", "0001" ... - sayisal olmayanlari ("Properties" vb.) atla.
        bool sayisalMi = altAdBoyut > 0;
        for (DWORD k = 0; k < altAdBoyut; ++k) {
            if (!iswdigit(altAd[k])) { sayisalMi = false; break; }
        }
        if (!sayisalMi) continue;

        std::wstring alanYol = std::wstring(sinifYolu) + L"\\" + altAd;
        const std::string ad = registryOku(HKEY_LOCAL_MACHINE, alanYol.c_str(), L"DriverDesc");
        if (ad.empty()) continue;

        GpuBilgisi g;
        g.ad = ad;
        g.surucuVersiyon = registryOku(HKEY_LOCAL_MACHINE, alanYol.c_str(), L"DriverVersion");
        g.surucuTarihi = registryOku(HKEY_LOCAL_MACHINE, alanYol.c_str(), L"DriverDate");

        if (const auto bayt = registryOkuQword(HKEY_LOCAL_MACHINE, alanYol.c_str(),
                                                L"HardwareInformation.qwMemorySize")) {
            g.bellekGB = static_cast<double>(*bayt) / (1024.0 * 1024.0 * 1024.0);
        } else if (const auto bayt32 = registryOkuDword(HKEY_LOCAL_MACHINE, alanYol.c_str(),
                                                         L"HardwareInformation.MemorySize")) {
            g.bellekGB = static_cast<double>(*bayt32) / (1024.0 * 1024.0 * 1024.0);
        }

        sonuc.push_back(g);
    }

    RegCloseKey(sinifAnahtar);
    return sonuc;
}

GrafikApiBilgisi grafikApileri()
{
    GrafikApiBilgisi g;
    g.opengl = openglVersiyonuOku();
    g.vulkan = vulkanVersiyonuOku();
    g.shaderModel = shaderModelOku();
    return g;
}

double gpuKullanimYuzdesi()
{
    return gpuKullanimYuzdesiHesapla();
}

}
