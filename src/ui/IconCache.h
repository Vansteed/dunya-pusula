// IconCache.h - exe tam yolundan kucuk simge (HICON -> QIcon), onbellekli.
#pragma once
#include <QIcon>
#include <QHash>
#include <string>

class IconCache {
public:
    // tamYol bos ya da bulunamazsa varsayilan (bos) QIcon doner - patlamaz.
    // Ayni yol icin ikinci cagri ONBELLEKTEN doner (SHGetFileInfo pahali).
    static QIcon simgeGetir(const std::string &tamYol);
private:
    static QHash<QString, QIcon> s_onbellek;
};
