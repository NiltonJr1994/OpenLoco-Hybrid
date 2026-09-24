#include "Hybrid/ParkPersistence.h"
#include "Hybrid/ParkInterior.h"
#include "Localisation/StringManager.h"
#include <OpenLoco/Diagnostics/Logging.h>
#include <fstream>
#include <set>

namespace OpenLoco::Hybrid::ParkPersistence
{
    namespace
    {
        std::string _errorText; // Survives park resets and the transition back to the title screen.
        constexpr uint64_t kOffset = 14695981039346656037ULL;
        uint64_t hash(std::span<const uint8_t> bytes, uint64_t value = kOffset)
        {
            for (auto byte : bytes)
            {
                value = (value ^ byte) * 1099511628211ULL;
            }
            return value;
        }
        struct Writer
        {
            std::vector<uint8_t> bytes;
            void number(uint64_t value, size_t count = 4)
            {
                for (size_t i = 0; i < count; ++i)
                {
                    bytes.push_back(static_cast<uint8_t>(value >> (i * 8)));
                }
            }
            void identity(const std::shared_ptr<const Rct2::Definition>& d)
            {
                if (!d)
                {
                    throw std::runtime_error("Park has an unresolved RCT2 object");
                }
                bytes.insert(bytes.end(), d->identity.begin(), d->identity.end());
            }
        };
        uint64_t wide(Rct2::Reader& r)
        {
            const uint64_t low = r.u32();
            return low | (uint64_t(r.u32()) << 32);
        }
        std::shared_ptr<const Rct2::Definition> definition(Rct2::Reader& r, uint8_t type)
        {
            std::array<uint8_t, 16> identity{};
            for (auto& b : identity)
            {
                b = r.u8();
            }
            const auto& registry = Rct2Assets::get();
            const auto& list = type == 0 ? registry.rides : (type == 1 ? registry.scenery : registry.entrances);
            for (const auto& d : list)
            {
                if (d->identity == identity)
                {
                    return d;
                }
            }
            throw std::runtime_error("Saved park needs an RCT2 object that is missing or has changed. Restore its original DAT file.");
        }
    }
    std::filesystem::path sidecarPath(const std::filesystem::path& save)
    {
        auto p = save;
        p += ".olh";
        return p;
    }
    uint64_t fingerprint(const std::filesystem::path& save)
    {
        std::ifstream stream(save, std::ios::binary);
        if (!stream)
        {
            throw std::runtime_error("Cannot read save to bind Hybrid data");
        }
        uint64_t result = kOffset;
        std::array<uint8_t, 65536> bytes{};
        while (stream)
        {
            stream.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
            result = hash(std::span(bytes.data(), static_cast<size_t>(stream.gcount())), result);
        }
        if (!stream.eof())
        {
            throw std::runtime_error("Cannot finish reading save");
        }
        return result;
    }
    std::vector<uint8_t> encode(std::span<const Parks::Park> parks, uint64_t saveFingerprint)
    {
        if (parks.size() > 64)
        {
            throw std::runtime_error("Too many Hybrid parks");
        }
        Writer w;
        w.number(0x31484C4F); // OLH1; fixed-width little endian, never native structs/pointers.
        w.number(1);
        w.number(saveFingerprint, 8);
        w.number(parks.size());
        for (const auto& p : parks)
        {
            w.number(p.id);
            w.number(p.position.x);
            w.number(p.position.y);
            w.number(enumValue(p.owner));
            w.number(p.closestTownId);
            w.number(p.rotation);
            w.number(p.model);
            w.number(p.height);
            w.number(p.open);
            w.number(p.paidConstruction);
            w.number(p.lastTax);
            w.number(p.lastOperatingCost);
            w.number(p.lastChargedMonth);
            w.number(p.paidInterior);
            w.identity(p.entrance);
            if (p.rides.size() > 9 || p.scenery.size() > 128)
            {
                throw std::runtime_error("Invalid Hybrid object count");
            }
            w.number(p.rides.size());
            for (const auto& ride : p.rides)
            {
                w.identity(ride.definition);
                w.number(ride.site.x);
                w.number(ride.site.y);
            }
            w.number(p.scenery.size());
            for (const auto& object : p.scenery)
            {
                w.identity(object.definition);
                w.number(object.tile.x);
                w.number(object.tile.y);
                w.number(object.rotation);
            }
        }
        w.number(hash(w.bytes), 8);
        return w.bytes;
    }
    std::vector<Parks::Park> decode(std::span<const uint8_t> bytes, uint64_t saveFingerprint)
    {
        if (bytes.size() < 28 || bytes.size() > 1024 * 1024)
        {
            throw std::runtime_error("Invalid Hybrid sidecar size");
        }
        Rct2::Reader checksum{ bytes.last(8) };
        if (wide(checksum) != hash(bytes.first(bytes.size() - 8)))
        {
            throw std::runtime_error("Hybrid sidecar checksum failed");
        }
        Rct2::Reader r{ bytes.first(bytes.size() - 8) };
        if (r.u32() != 0x31484C4F || r.u32() != 1)
        {
            throw std::runtime_error("Unsupported Hybrid save version");
        }
        if (wide(r) != saveFingerprint)
        {
            throw std::runtime_error("Hybrid sidecar belongs to a different save. Keep the SV5 and its .olh file together.");
        }
        const auto count = r.u32();
        if (count > 64)
        {
            throw std::runtime_error("Too many parks in Hybrid sidecar");
        }
        const auto bounded = [&](uint32_t max) { const auto n = r.u32(); if (n > max){ throw std::runtime_error("Invalid Hybrid save field");
} return n; };
        std::vector<Parks::Park> parks;
        std::set<uint16_t> ids;
        for (uint32_t i = 0; i < count; ++i)
        {
            Parks::Park p;
            p.id = static_cast<uint16_t>(bounded(65534));
            if (!p.id || !ids.insert(p.id).second)
            {
                throw std::runtime_error("Duplicate or zero park ID");
            }
            p.position.x = static_cast<coord_t>(bounded(12255));
            p.position.y = static_cast<coord_t>(bounded(12255));
            if (p.position.x % 32 || p.position.y % 32 || p.position.x < 96 || p.position.y < 96 || p.position.x > 12160 || p.position.y > 12160)
            {
                throw std::runtime_error("Invalid regional park footprint");
            }
            for (const auto& other : parks)
            {
                if (Parks::footprintsOverlap(p.position, other.position))
                {
                    throw std::runtime_error("Overlapping saved parks");
                }
            }
            p.owner = CompanyId(bounded(14));
            p.closestTownId = static_cast<uint16_t>(bounded(65535));
            p.rotation = static_cast<uint8_t>(bounded(3));
            p.model = static_cast<uint8_t>(bounded(2));
            p.height = static_cast<coord_t>(bounded(1020));
            p.open = bounded(1) != 0;
            p.paidConstruction = bounded(INT32_MAX);
            p.lastTax = bounded(INT32_MAX);
            p.lastOperatingCost = bounded(INT32_MAX);
            p.lastChargedMonth = bounded(1000000);
            p.paidInterior = bounded(INT32_MAX);
            p.entrance = definition(r, 8);
            const auto rides = bounded(9);
            for (uint32_t j = 0; j < rides; ++j)
            {
                Parks::Park::RideInstance ride;
                ride.definition = definition(r, 0);
                const auto x = static_cast<int32_t>(r.u32()), y = static_cast<int32_t>(r.u32());
                if (x < -3 || x > 3 || y < -3 || y > 3)
                {
                    throw std::runtime_error("Invalid regional ride position");
                }
                ride.site = { static_cast<coord_t>(x), static_cast<coord_t>(y) };
                p.rides.push_back(std::move(ride));
            }
            const auto objects = bounded(128);
            for (uint32_t j = 0; j < objects; ++j)
            {
                Parks::Park::SceneryInstance object;
                object.definition = definition(r, 1);
                object.tile.x = static_cast<coord_t>(bounded(11));
                object.tile.y = static_cast<coord_t>(bounded(11));
                object.rotation = static_cast<uint8_t>(bounded(3));
                if (!ParkInterior::supported(*object.definition) || ParkInterior::path(object.tile) || std::any_of(p.scenery.begin(), p.scenery.end(), [&](const auto& s) { return s.tile == object.tile; }))
                {
                    throw std::runtime_error("Invalid interior scenery placement");
                }
                p.scenery.push_back(std::move(object));
            }
            parks.push_back(std::move(p));
        }
        if (r.pos != r.bytes.size())
        {
            throw std::runtime_error("Unexpected Hybrid save data");
        }
        return parks;
    }
    void save(const std::filesystem::path& path)
    {
        const auto data = encode(Parks::_parks, fingerprint(path));
        const auto target = sidecarPath(path);
        auto temp = target;
        temp += ".tmp";
        auto backup = target;
        backup += ".bak";
        {
            std::ofstream stream(temp, std::ios::binary | std::ios::trunc);
            stream.write(reinterpret_cast<const char*>(data.data()), data.size());
            stream.close();
            if (!stream)
            {
                throw std::runtime_error("Cannot write Hybrid sidecar. The base save exists, but park data was not saved.");
            }
        }
        const bool old = std::filesystem::exists(target);
        if (old)
        {
            std::filesystem::remove(backup);
            std::filesystem::rename(target, backup);
        }
        try
        {
            std::filesystem::rename(temp, target);
        }
        catch (...)
        {
            if (old)
            {
                std::filesystem::rename(backup, target);
            }
            throw;
        }
    }
    std::vector<Parks::Park> read(const std::filesystem::path& path)
    {
        const auto sidecar = sidecarPath(path);
        if (!std::filesystem::exists(sidecar))
        {
            return {};
        }
        return decode(Rct2::readFile(sidecar, 1024 * 1024), fingerprint(path));
    }
    void restore(std::vector<Parks::Park> parks)
    {
        uint16_t next = 1;
        for (auto& p : parks)
        {
            const auto* company = CompanyManager::get(p.owner);
            if (!company || company->empty())
            {
                throw std::runtime_error("Saved Hybrid park owner does not exist in this scenario");
            }
            p.groundImage = Parks::groundImage();
            p.entranceImage = ParkVisuals::entrance(p.entrance);
            for (auto& r : p.rides)
            {
                r = Parks::makeRide(r.definition, r.site);
            }
            for (auto& s : p.scenery)
            {
                s.image = Rct2Graphics::load(s.definition);
            }
            next = std::max<uint16_t>(next, p.id + 1);
        }
        Parks::_parks = std::move(parks);
        Parks::_nextParkId = next;
        Parks::_selectedParkId = Parks::_parks.empty() ? 0 : Parks::_parks.front().id;
        Parks::_lastStatus = "Hybrid parks restored from the save companion file.";
    }
    void reportError(const std::exception& error)
    {
        Diagnostics::Logging::error("Hybrid save/load failed: {}", error.what());
        Parks::_lastStatus = std::string(error.what()).substr(0, 200);
        for (auto& c : Parks::_lastStatus)
        {
            if (static_cast<unsigned char>(c) < 32 || static_cast<unsigned char>(c) > 126)
            {
                c = '?';
            }
        }
        _errorText = Parks::_lastStatus;
        StringManager::swapString(2486, _errorText.c_str());
        Ui::Windows::Error::open(StringIds::error_file_contains_invalid_data, 2486);
    }
}
