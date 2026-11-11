#pragma once

// Self-contained fixtures: never read the user's desktop or application icons.
inline bool WriteIconFixture(const std::wstring& path, int padding, int number) {
    Gdiplus::Bitmap bitmap(256, 256, PixelFormat32bppARGB);
    Gdiplus::Graphics graphics(&bitmap);
    graphics.Clear(Gdiplus::Color(0, 0, 0, 0));
    graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    float x = (float)padding, side = 256.f - padding * 2, diameter = side * .35f;
    Gdiplus::GraphicsPath shape;
    shape.AddArc(x, x, diameter, diameter, 180.f, 90.f);
    shape.AddArc(x + side - diameter, x, diameter, diameter, 270.f, 90.f);
    shape.AddArc(x + side - diameter, x + side - diameter, diameter, diameter, 0.f, 90.f);
    shape.AddArc(x, x + side - diameter, diameter, diameter, 90.f, 90.f);
    shape.CloseFigure();
    Gdiplus::Color colors[] = {
        Gdiplus::Color(255, 39, 100, 195), Gdiplus::Color(255, 160, 65, 110),
        Gdiplus::Color(255, 35, 135, 100)};
    Gdiplus::SolidBrush fill(colors[number % 3]), ink(Gdiplus::Color(255, 255, 255, 255));
    graphics.FillPath(&fill, &shape);
    Gdiplus::Font font(L"Segoe UI", side * .48f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
    Gdiplus::StringFormat format;
    format.SetAlignment(Gdiplus::StringAlignmentCenter);
    format.SetLineAlignment(Gdiplus::StringAlignmentCenter);
    wchar_t letter[] = {(wchar_t)(L'A' + number), 0};
    graphics.DrawString(letter, 1, &font, Gdiplus::RectF(x, x, side, side), &format, &ink);
    graphics.Flush();
    CLSID png{0x557cf406, 0x1a04, 0x11d3, {0x9a, 0x73, 0x00, 0x00, 0xf8, 0x1e, 0xf3, 0x2e}};
    ComPtr<IStream> stream;
    if (FAILED(CreateStreamOnHGlobal(nullptr, TRUE, &stream)) ||
        bitmap.Save(stream.Get(), &png) != Gdiplus::Ok) return false;
    STATSTG stat{};
    HGLOBAL memory = nullptr;
    if (FAILED(stream->Stat(&stat, STATFLAG_NONAME)) ||
        FAILED(GetHGlobalFromStream(stream.Get(), &memory))) return false;
    auto bytes = (const char*)GlobalLock(memory);
    if (!bytes) return false;
    std::ofstream output(path, std::ios::binary);
    const unsigned short header[] = {0, 1, 1};
    output.write((const char*)header, sizeof(header));
    const unsigned char dimensions[] = {0, 0, 0, 0};
    const unsigned short formatFields[] = {1, 32};
    const DWORD offsets[] = {stat.cbSize.LowPart, 22};
    output.write((const char*)dimensions, sizeof(dimensions));
    output.write((const char*)formatFields, sizeof(formatFields));
    output.write((const char*)offsets, sizeof(offsets));
    output.write(bytes, stat.cbSize.LowPart);
    GlobalUnlock(memory);
    return output.good();
}

inline Group MakeIconFixtures(const std::wstring& root) {
    Group group;
    group.path = root + L"\\icon-fixture.softfolder";
    int margins[] = {12, 32, 48};
    for (int i = 0; i < 3; ++i) {
        auto path = root + L"\\Example " + (wchar_t)(L'A' + i) + L".ico";
        if (!WriteIconFixture(path, margins[i], i)) return {};
        group.items.push_back({path, i});
    }
    return group;
}
