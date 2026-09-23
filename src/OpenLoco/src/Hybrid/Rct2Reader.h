#pragma once

// A bounded reader for the legacy RCT2 DAT wire format. No RCT2 engine state,
// Locomotion object slots, or process launcher is involved.
#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace OpenLoco::Hybrid::Rct2
{
    constexpr size_t kMaxBytes = 32 * 1024 * 1024;
    struct Reader
    {
        std::span<const uint8_t> bytes;
        size_t pos{};
        void require(size_t n) const
        {
            if (pos > bytes.size() || n > bytes.size() - pos)
                throw std::runtime_error("Truncated RCT2 data");
        }
        uint8_t u8() { require(1); return bytes[pos++]; }
        uint16_t u16() { const auto a = u8(); return static_cast<uint16_t>(a | (u8() << 8)); }
        uint32_t u32() { const auto a = u16(); return a | (static_cast<uint32_t>(u16()) << 16); }
        void skip(size_t n) { require(n); pos += n; }
        std::string strings()
        {
            std::string selected;
            for (size_t count = 0; count < 64; ++count)
            {
                const auto language = u8();
                if (language == 255) return selected;
                std::string text;
                for (auto c = u8(); c != 0; c = u8())
                {
                    if (text.size() >= 4096) throw std::runtime_error("RCT2 string too long");
                    // Do not pass RCT2 formatting controls to the OpenLoco formatter.
                    text += c >= 32 && c < 127 ? static_cast<char>(c) : '?';
                }
                if (selected.empty() || language == 0) selected = text;
            }
            throw std::runtime_error("RCT2 string table has no terminator");
        }
    };

    inline std::vector<uint8_t> readFile(const std::filesystem::path& path, size_t limit = kMaxBytes)
    {
        std::ifstream f(path, std::ios::binary | std::ios::ate);
        if (!f || f.tellg() < 0 || static_cast<uint64_t>(f.tellg()) > limit)
            throw std::runtime_error("RCT2 file missing or too large");
        std::vector<uint8_t> data(static_cast<size_t>(f.tellg()));
        f.seekg(0);
        if (!f.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(data.size())))
            throw std::runtime_error("Cannot read RCT2 file");
        return data;
    }

    inline std::vector<uint8_t> decode(std::span<const uint8_t> encoded, uint8_t encoding)
    {
        if (encoded.size() > kMaxBytes || encoding > 3) throw std::runtime_error("Unsupported RCT2 chunk");
        std::vector<uint8_t> out;
        Reader r{ encoded };
        while (r.pos < encoded.size())
        {
            const auto b = r.u8();
            if (encoding == 0 || encoding == 3)
            {
                const auto shift = static_cast<unsigned>(((r.pos - 1) * 2 + 1) % 8);
                out.push_back(encoding == 0 ? b : static_cast<uint8_t>((b >> shift) | (b << (8 - shift))));
            }
            else
            {
                const size_t n = b >= 128 ? 257 - b : b + 1;
                if (n > kMaxBytes - out.size()) throw std::runtime_error("RCT2 expansion limit");
                if (b >= 128) out.insert(out.end(), n, r.u8());
                else
                {
                    r.require(n);
                    out.insert(out.end(), encoded.begin() + r.pos, encoded.begin() + r.pos + n);
                    r.skip(n);
                }
            }
        }
        if (encoding != 2) return out;
        auto intermediate = std::move(out);
        out.clear();
        Reader repeat{ intermediate };
        while (repeat.pos < intermediate.size())
        {
            const auto b = repeat.u8();
            if (b == 255)
            {
                if (out.size() == kMaxBytes) throw std::runtime_error("RCT2 expansion limit");
                out.push_back(repeat.u8());
            }
            else
            {
                const size_t distance = 32 - (b >> 3), n = (b & 7) + 1;
                if (distance > out.size() || n > distance || n > kMaxBytes - out.size())
                    throw std::runtime_error("Invalid RCT2 back reference");
                const auto start = out.size() - distance;
                for (size_t i = 0; i < n; ++i) out.push_back(out[start + i]);
            }
        }
        return out;
    }

    struct Sprite
    {
        int16_t width{}, height{}, x{}, y{};
        std::vector<uint8_t> pixels; // decoded palette indices; 0 is transparent
    };
    struct Definition
    {
        std::array<uint8_t, 16> identity{};
        uint8_t type{};
        std::string id, name, description, capacity;
        std::array<uint8_t, 3> rideTypes{ 255, 255, 255 };
        uint8_t minCars{}, maxCars{}, flatRideCars{};
        std::vector<uint8_t> payload;
        std::vector<Sprite> sprites; // entrance's 12 directional parts, or ride preview
    };

    inline std::vector<Sprite> images(Reader& r, size_t wanted)
    {
        const size_t count = r.u32(), size = r.u32();
        if (count == 0 || count > 65536 || size > kMaxBytes) throw std::runtime_error("Invalid RCT2 image table");
        r.require(count * 16 + size);
        const auto headers = r.pos, base = headers + count * 16;
        std::vector<Sprite> result;
        for (size_t i = 0; i < std::min(count, wanted); ++i)
        {
            Reader h{ r.bytes.subspan(headers + i * 16, 16) };
            const size_t offset = h.u32();
            Sprite s;
            s.width = static_cast<int16_t>(h.u16()); s.height = static_cast<int16_t>(h.u16());
            s.x = static_cast<int16_t>(h.u16()); s.y = static_cast<int16_t>(h.u16());
            const auto flags = h.u16();
            if (s.width <= 0 || s.height <= 0 || s.width > 512 || s.height > 512 || offset >= size || (flags & 8))
                throw std::runtime_error("Unsupported RCT2 sprite dimensions or flags");
            s.pixels.resize(static_cast<size_t>(s.width) * s.height);
            Reader pixels{ r.bytes.subspan(base + offset, size - offset) };
            if ((flags & 4) == 0)
            {
                pixels.require(s.pixels.size());
                std::copy_n(pixels.bytes.begin(), s.pixels.size(), s.pixels.begin());
            }
            else
            {
                pixels.require(static_cast<size_t>(s.height) * 2);
                for (int y = 0; y < s.height; ++y)
                {
                    Reader row{ pixels.bytes, static_cast<size_t>(y) * 2 };
                    row.pos = row.u16();
                    if (row.pos < static_cast<size_t>(s.height) * 2) throw std::runtime_error("Invalid RCT2 row offset");
                    bool ended = false;
                    for (int runs = 0; runs <= s.width; ++runs)
                    {
                        const auto tag = row.u8(), x = row.u8();
                        const size_t n = tag & 127;
                        if (x + n > static_cast<size_t>(s.width)) throw std::runtime_error("RCT2 sprite row overflow");
                        row.require(n);
                        std::copy_n(row.bytes.begin() + row.pos, n, s.pixels.begin() + y * s.width + x);
                        row.skip(n);
                        if (tag & 128) { ended = true; break; }
                    }
                    if (!ended) throw std::runtime_error("Unterminated RCT2 sprite row");
                }
            }
            result.push_back(std::move(s));
        }
        r.skip(count * 16 + size);
        return result;
    }

    inline Definition parse(std::span<const uint8_t> file)
    {
        Reader r{ file };
        r.require(21);
        Definition d;
        std::copy_n(file.begin(), 16, d.identity.begin());
        d.type = file[0] & 15;
        if (d.type != 0 && d.type != 8) throw std::runtime_error("Object class not supported by native slice");
        for (size_t i = 4; i < 12; ++i)
        {
            if (file[i] < 32 || file[i] > 126) throw std::runtime_error("Invalid RCT2 identifier");
            d.id += static_cast<char>(file[i]);
        }
        while (!d.id.empty() && d.id.back() == ' ') d.id.pop_back();
        if (d.id.empty()) throw std::runtime_error("Empty RCT2 identifier");
        r.skip(16);
        const auto encoding = r.u8();
        const auto length = r.u32();
        r.require(length);
        if (length != file.size() - r.pos) throw std::runtime_error("RCT2 chunk length mismatch");
        d.payload = decode(file.subspan(r.pos, length), encoding);
        Reader data{ d.payload };
        if (d.type == 8)
        {
            data.skip(8);
            d.name = data.strings();
            d.sprites = images(data, 12);
            if (d.sprites.size() != 12) throw std::runtime_error("Entrance requires 12 directional sprites");
        }
        else
        {
            // Legacy ride fixed record: 26 bytes + 4 * 101-byte car records + 20 bytes.
            data.skip(12);
            for (auto& t : d.rideTypes) t = data.u8();
            if (std::all_of(d.rideTypes.begin(), d.rideTypes.end(), [](auto t) { return t == 255; }))
                throw std::runtime_error("Ride has no ride type");
            d.minCars = data.u8(); d.maxCars = data.u8(); d.flatRideCars = data.u8();
            data.skip(0x1C2 - data.pos);
            d.name = data.strings(); d.description = data.strings(); d.capacity = data.strings();
            const auto colours = data.u8();
            data.skip((colours == 255 ? 32 : colours) * 3);
            for (int car = 0; car < 4; ++car)
            {
                size_t positions = data.u8();
                if (positions == 255) positions = data.u16();
                data.skip(positions);
            }
            d.sprites = images(data, 1);
        }
        if (d.name.empty()) d.name = d.id;
        return d;
    }

    using Palette = std::array<std::array<uint8_t, 3>, 256>;
    inline Palette readPalette(const std::filesystem::path& path)
    {
        const auto file = readFile(path, 128 * 1024 * 1024);
        Reader r{ file };
        const size_t count = r.u32(), size = r.u32();
        if (count <= 1532 || count > 65536) throw std::runtime_error("Invalid RCT2 g1 table");
        r.require(count * 16 + size);
        Reader h{ r.bytes, 8 + 1532 * 16 };
        const auto offset = h.u32();
        const auto width = h.u16(); h.skip(2);
        const auto first = h.u16(); h.skip(2);
        if ((h.u16() & 8) == 0 || first + width > 256 || width < 200 || offset > size || width * 3 > size - offset)
            throw std::runtime_error("Invalid RCT2 default palette");
        Palette palette{};
        Reader rgb{ r.bytes.subspan(8 + count * 16 + offset, width * 3) };
        for (size_t i = first; i < first + width; ++i)
            for (auto& channel : palette[i]) channel = rgb.u8();
        return palette;
    }
}
