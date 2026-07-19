#include "UiHelper.h"
#include <QSvgRenderer>
#include <QPainter>
#include <QFileInfo>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QtConcurrent/QtConcurrent>
#include <windows.h>
#include <objbase.h>
#include <shlobj.h>
#include <thumbcache.h>

namespace ArcMeta {

QImage UiHelper::getImageForAnalysis(const QString& path, int size) {
    QFileInfo fi(path);
    if (fi.suffix().toLower() == "svg") {
        QSvgRenderer renderer(path);
        if (renderer.isValid()) {
            QImage img(size, size, QImage::Format_ARGB32);
            img.fill(Qt::transparent);
            QPainter painter(&img);
            renderer.render(&painter);
            return img;
        }
    }
    
    QImage img = getShellThumbnail(path, size);
    if (img.isNull()) img.load(path);
    return img;
}

QImage UiHelper::getShellThumbnail(const QString& path, int size) {
    QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString cacheDir = QDir(appData).filePath("thumbs/");
    QDir().mkpath(cacheDir);

    QFileInfo fi(path);
    QString hashKey = QString("%1_%2_%3_%4_v14").arg(path).arg(fi.size()).arg(fi.lastModified().toMSecsSinceEpoch()).arg(size);
    QString safeName = QString::number(qHash(hashKey), 16) + ".png";
    QString cachePath = cacheDir + safeName;

    if (QFile::exists(cachePath)) {
        QImage img;
        if (img.load(cachePath)) return img;
    }

#ifdef Q_OS_WIN
    PIDLIST_ABSOLUTE pidl = nullptr;
    HRESULT hr = SHParseDisplayName(path.toStdWString().c_str(), nullptr, &pidl, 0, nullptr);
    if (FAILED(hr)) return QImage();
    IShellItem* pItem = nullptr;
    hr = SHCreateItemFromIDList(pidl, IID_IShellItem, (void**)&pItem);
    ILFree(pidl);
    if (SUCCEEDED(hr)) {
        IShellItemImageFactory* pFactory = nullptr;
        hr = pItem->QueryInterface(IID_IShellItemImageFactory, (void**)&pFactory);
        if (SUCCEEDED(hr)) {
            SIZE nativeSize = { size, size };
            HBITMAP hBitmap = nullptr;
            hr = pFactory->GetImage(nativeSize, SIIGBF_RESIZETOFIT, &hBitmap);
            if (SUCCEEDED(hr) && hBitmap) {
                BITMAP bmpInfo;
                GetObject(hBitmap, sizeof(bmpInfo), &bmpInfo);
                int w = bmpInfo.bmWidth;
                int h = std::abs(bmpInfo.bmHeight);

                BITMAPINFOHEADER bi = {};
                bi.biSize        = sizeof(BITMAPINFOHEADER);
                bi.biWidth       = w;
                bi.biHeight      = -h;
                bi.biPlanes      = 1;
                bi.biBitCount    = 32;
                bi.biCompression = BI_RGB;

                QByteArray pixels(w * h * 4, 0);
                HDC hdc = GetDC(nullptr);
                GetDIBits(hdc, hBitmap, 0, h, pixels.data(),
                          reinterpret_cast<BITMAPINFO*>(&bi), DIB_RGB_COLORS);
                ReleaseDC(nullptr, hdc);

                uint8_t* p = reinterpret_cast<uint8_t*>(pixels.data());
                for (int i = 0; i < w * h; ++i) {
                    std::swap(p[i * 4 + 0], p[i * 4 + 2]);
                }

                QImage img(p, w, h, w * 4, QImage::Format_RGBA8888);
                img = img.copy();
                
                (void)QtConcurrent::run([img, cachePath]() {
                    img.save(cachePath, "PNG");
                });

                DeleteObject(hBitmap);
                pFactory->Release();
                pItem->Release();
                return img;
            }
            pFactory->Release();
        }
        pItem->Release();
    }
#else
    Q_UNUSED(path); Q_UNUSED(size);
#endif
    return QImage();
}

} // namespace ArcMeta
