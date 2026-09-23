#pragma once
#include "Hybrid/Rct2Reader.h"
#include <OpenLoco/Platform/Platform.h>
#include <cctype>

namespace OpenLoco::Hybrid::Rct2Assets
{
    struct Registry
    {
        std::filesystem::path root;
        Rct2::Palette palette{};
        std::vector<std::shared_ptr<const Rct2::Definition>> rides, entrances;
        uint32_t unsupported{}, rejected{};
        bool scanned{}, ready{};
        std::string status{ "RCT2 has not been scanned." };
    };
    inline Registry _registry;
    inline size_t _selectedRide{};
    inline void scan()
    {
        Registry next;
        next.scanned = true;
        next.root = Platform::getCurrentExecutablePath().parent_path() / "RCT2";
        try
        {
            next.palette = Rct2::readPalette(next.root / "Data" / "g1.dat");
            std::vector<std::filesystem::path> paths;
            for (const auto& e : std::filesystem::directory_iterator(next.root / "ObjData"))
            {
                auto ext = e.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                if (e.is_regular_file() && ext == ".dat") paths.push_back(e.path());
            }
            std::sort(paths.begin(), paths.end());
            size_t decodedBytes = 0;
            for (const auto& path : paths)
            {
                try
                {
                    const auto file = Rct2::readFile(path);
                    if (file.size() < 21) throw std::runtime_error("Short DAT");
                    if ((file[0] & 15) != 0 && (file[0] & 15) != 8) { ++next.unsupported; continue; }
                    auto d = std::make_shared<Rct2::Definition>(Rct2::parse(file));
                    size_t cost = d->payload.size();
                    for (const auto& s : d->sprites) cost += s.pixels.size();
                    if (cost > 256 * 1024 * 1024 - decodedBytes) throw std::runtime_error("Registry memory limit");
                    auto& list = d->type == 0 ? next.rides : next.entrances;
                    if (std::any_of(list.begin(), list.end(), [&](const auto& e) { return e->identity == d->identity; })) continue;
                    decodedBytes += cost;
                    list.push_back(std::move(d));
                }
                catch (const std::exception&) { ++next.rejected; }
            }
            next.ready = !next.rides.empty() && !next.entrances.empty();
            next.status = next.ready ? "Native RCT2 registry ready." : "No supported ride and entrance pair could be decoded.";
        }
        catch (const std::exception& e) { next.status = std::string("RCT2: ") + e.what(); }
        _registry = std::move(next);
        _selectedRide = 0;
    }
    inline const Registry& get() { if (!_registry.scanned) scan(); return _registry; }
    inline bool ready() { return get().ready; }
    inline std::shared_ptr<const Rct2::Definition> selectedRide()
    {
        const auto& r = get();
        return r.rides.empty() ? nullptr : r.rides[_selectedRide % r.rides.size()];
    }
    inline void selectRide(int direction)
    {
        const auto count = get().rides.size();
        if (count != 0) _selectedRide = (_selectedRide + count + direction) % count;
    }
}
