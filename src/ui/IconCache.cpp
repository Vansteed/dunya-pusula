#include "IconCache.h"

#include <windows.h>
#include <shellapi.h>

#include <QImage>
#include <QPixmap>

QHash<QString, QIcon> IconCache::s_onbellek;

namespace {

// HICON'u QImage'e cevirir. Herhangi bir adim basarisiz olursa bos QImage doner.
QImage hiconToQImage(HICON hicon)
{
    if (!hicon) return QImage();

    ICONINFO ii;
    if (!GetIconInfo(hicon, &ii)) return QImage();

    BITMAP bm;
    if (GetObject(ii.hbmColor, sizeof(bm), &bm) == 0) {
        if (ii.hbmColor) DeleteObject(ii.hbmColor);
        if (ii.hbmMask) DeleteObject(ii.hbmMask);
        return QImage();
    }

    const int width = bm.bmWidth;
    const int height = bm.bmHeight;

    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = width;
    bi.bmiHeader.biHeight = -height; // ust-asagi
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    QImage img(width, height, QImage::Format_ARGB32);
    HDC hdc = CreateCompatibleDC(nullptr);
    if (!hdc) {
        DeleteObject(ii.hbmColor);
        if (ii.hbmMask) DeleteObject(ii.hbmMask);
        return QImage();
    }

    const int alinan = GetDIBits(hdc, ii.hbmColor, 0, height,
                                  img.bits(), &bi, DIB_RGB_COLORS);

    DeleteDC(hdc);
    DeleteObject(ii.hbmColor);
    if (ii.hbmMask) DeleteObject(ii.hbmMask);

    if (alinan == 0) return QImage();
    return img;
}

}

QIcon IconCache::simgeGetir(const std::string &tamYol)
{
    if (tamYol.empty()) return QIcon();

    const QString anahtar = QString::fromStdString(tamYol).toLower();
    auto it = s_onbellek.find(anahtar);
    if (it != s_onbellek.end()) return it.value();

    std::wstring wyol(tamYol.begin(), tamYol.end());
    SHFILEINFOW shfi = {};
    const DWORD_PTR sonuc = SHGetFileInfoW(wyol.c_str(), 0, &shfi, sizeof(shfi),
                                            SHGFI_ICON | SHGFI_SMALLICON);
    if (sonuc == 0 || !shfi.hIcon) {
        s_onbellek.insert(anahtar, QIcon());
        return QIcon();
    }

    const QImage img = hiconToQImage(shfi.hIcon);
    DestroyIcon(shfi.hIcon);

    QIcon simge;
    if (!img.isNull()) simge = QIcon(QPixmap::fromImage(img));

    s_onbellek.insert(anahtar, simge);
    return simge;
}
