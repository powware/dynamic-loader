#ifndef __ICON_HELPERS_H__
#define __ICON_HELPERS_H__

#include <optional>

#include <QPixmap>

std::optional<QIcon> QIconFromHIcon(HICON icon) // TODO proper cleanup on error
{
    ICONINFO iconInfo;
    if (!GetIconInfo(icon, &iconInfo))
    {
        return std::nullopt;
    }

    BITMAP bm;
    if (!GetObject(iconInfo.hbmColor, sizeof(BITMAP), &bm))
    {
        return std::nullopt;
    }

    const auto width = bm.bmWidth;
    const auto height = bm.bmHeight;

    QImage image(width, height, QImage::Format_ARGB32);
    image.fill(Qt::transparent);

    HDC hdc = CreateCompatibleDC(nullptr);
    SelectObject(hdc, iconInfo.hbmColor);

    BITMAPINFOHEADER bi;
    memset(&bi, 0, sizeof(bi));
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = width;
    bi.biHeight = -height; // Negative indicates a top-down DIB
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;

    GetDIBits(hdc, iconInfo.hbmColor, 0, height, image.bits(), reinterpret_cast<BITMAPINFO *>(&bi), DIB_RGB_COLORS);

    DeleteObject(iconInfo.hbmColor);
    DeleteObject(iconInfo.hbmMask);
    DeleteDC(hdc);

    return QIcon(QPixmap::fromImage(image));
}

std::optional<QIcon> QIconFromProcessId(DWORD process_id)
{

    auto exe_path = pfw::GetExecutablePathFromProcessId(process_id);
    if (!exe_path)
    {
        return std::nullopt;
    }

    SHFILEINFO shFileInfo;
    if (!SHGetFileInfo(exe_path->c_str(), 0, &shFileInfo, sizeof(shFileInfo), SHGFI_ICON | SHGFI_LARGEICON))
    {
        return std::nullopt;
    }

    return QIconFromHIcon(shFileInfo.hIcon);
}

std::optional<QIcon> LoadIconFromImageres(WORD id)
{
    static auto module = LoadLibraryW(L"imageres.dll");
    HICON icon = LoadIconW(module, MAKEINTRESOURCEW(id));

    return QIconFromHIcon(icon);
}

#endif // __ICON_HELPERS_H__