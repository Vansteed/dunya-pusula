// Theme.h - koyu tema renk sabitleri ve QSS uretimi.
// Renkler RamTemizleyici.ps1 XAML kaynagindan (satir ~179-450) BIREBIR alindi.
#pragma once

#include <QString>

namespace Theme {

// Kart zemin / kenar
inline constexpr auto KartZemin = "#141418";
inline constexpr auto Kenar = "#25252B";
// Pencere ic zemin (RootCard, XAML satir 344)
inline constexpr auto PencereZemin = "#101114";
// Yazi renkleri
inline constexpr auto AnaYazi = "#F4F4F5";
inline constexpr auto IkinciYazi = "#A1A1AA";
// Vurgu (secili sekme, buton, ilerleme cubugu)
inline constexpr auto Vurgu = "#4C5F7E";

QString qssKoyuTema();

} // namespace Theme
