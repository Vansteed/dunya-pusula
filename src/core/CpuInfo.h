// CpuInfo.h - islemci kullanim/frekans/sicaklik + GPU bilgisi. Sadece
// WinAPI/COM, Qt yok (SysInfo.h ile ayni stil).
#pragma once

#include <optional>
#include <string>
#include <vector>

struct CpuBilgisi {
    std::string ad;
    int cekirdek = 0;
    int mantiksalCekirdek = 0;
    double kullanimYuzde = 0.0;
    int mhz = 0;
    int maxMhz = 0;
    std::optional<double> sicaklikC;
};

struct GpuBilgisi {
    std::string ad;
    std::string surucuVersiyon;
    std::string surucuTarihi;
    double bellekGB = 0.0;
    std::string openglVersiyon;
    std::string vulkanVersiyon;
    std::string shaderModel;
};

// Sistem genelinde tek bir grafik API seti (GPU basina degil).
struct GrafikApiBilgisi {
    std::string opengl;
    std::string vulkan;
    std::string shaderModel;
};

namespace CpuInfo {

// Her cagrida guncel deger dondurur. kullanimYuzde iki olcum arasindaki
// farktan hesaplanir - ilk cagride 0.0 doner.
CpuBilgisi cpuBilgisi();

// Birden fazla GPU olabilir - hepsi listelenir.
std::vector<GpuBilgisi> gpuBilgisi();

// GPU 3D motor kullanim yuzdesi (PDH \GPU Engine(*engtype_3D)\Utilization
// Percentage, tum ornekler toplanir). Sayac yoksa veya ilk cagrida 0.0 doner.
double gpuKullanimYuzdesi();

// OpenGL/Vulkan/Direct3D shader model destegini sorgular. Basarisiz olan
// alanlar bos string doner, uygulamayi cokertmez.
GrafikApiBilgisi grafikApileri();

}
