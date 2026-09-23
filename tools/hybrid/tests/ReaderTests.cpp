#include "../../../src/OpenLoco/src/Hybrid/Rct2Reader.h"
#include <iostream>
#include <random>

using namespace OpenLoco::Hybrid::Rct2;
static void check(bool value)
{
    if (!value)
    {
        throw std::runtime_error("Test assertion failed");
    }
}
template<typename F>
static void rejects(F f)
{
    bool rejected = false;
    try
    {
        f();
    }
    catch (const std::exception&)
    {
        rejected = true;
    }
    check(rejected);
}
static void u16(std::vector<uint8_t>& b, uint16_t n)
{
    b.push_back(n & 255);
    b.push_back(n >> 8);
}
static void u32(std::vector<uint8_t>& b, uint32_t n)
{
    u16(b, n & 65535);
    u16(b, n >> 16);
}
static void name(std::vector<uint8_t>& b) { b.insert(b.end(), { 0, 'T', 'e', 's', 't', 0, 255 }); }
static std::vector<uint8_t> fixture(bool ride)
{
    std::vector<uint8_t> payload(ride ? 0x1C2 : 8);
    name(payload);
    if (ride)
    {
        payload[12] = 33;
        payload[13] = 255;
        payload[14] = 255;
        name(payload);
        name(payload);
        payload.insert(payload.end(), 5, 0); // no preset colours or passenger offsets
    }
    const uint32_t count = ride ? 1 : 12;
    u32(payload, count);
    u32(payload, count * 4);
    for (uint32_t i = 0; i < count; ++i)
    {
        u32(payload, i * 4);
        u16(payload, 2);
        u16(payload, 2);
        u16(payload, 0);
        u16(payload, 0);
        u16(payload, 1);
        u16(payload, 0);
    }
    for (uint32_t i = 0; i < count; ++i)
    {
        payload.insert(payload.end(), { 0, 10, 20, 30 });
    }
    std::vector<uint8_t> file{ static_cast<uint8_t>(ride ? 0 : 8), 0, 0, 0, 'T', 'E', 'S', 'T', ' ', ' ', ' ', ' ', 0, 0, 0, 0, 0 };
    u32(file, static_cast<uint32_t>(payload.size()));
    file.insert(file.end(), payload.begin(), payload.end());
    return file;
}
int main(int argc, char** argv)
{
    try
    {
        check(decode(std::array<uint8_t, 4>{ 2, 4, 5, 6 }, 1) == std::vector<uint8_t>({ 4, 5, 6 }));
        check(decode(std::array<uint8_t, 2>{ 254, 7 }, 1) == std::vector<uint8_t>({ 7, 7, 7 }));
        check(decode(std::array<uint8_t, 4>{ 2, 255, 42, 248 }, 2) == std::vector<uint8_t>({ 42, 42 }));
        check(decode(std::array<uint8_t, 4>{ 2, 8, 32, 128 }, 3) == std::vector<uint8_t>({ 1, 1, 1, 1 }));
        rejects([] { decode(std::array<uint8_t, 1>{ 255 }, 1); });
        rejects([] { decode(std::array<uint8_t, 2>{ 3, 5 }, 1); });
        rejects([] { decode(std::array<uint8_t, 2>{ 0, 0 }, 2); });
        rejects([] { decode(std::array<uint8_t, 1>{ 0 }, 4); });
        for (bool ride : { false, true })
        {
            auto f = fixture(ride);
            auto d = parse(f);
            check(d.id == "TEST" && d.name == "Test" && d.sprites.size() == (ride ? 1 : 12));
            check(d.sprites[0].pixels == std::vector<uint8_t>({ 0, 10, 20, 30 }));
            for (size_t n = 0; n < f.size(); ++n)
            {
                rejects([&] { parse(std::span(f).first(n)); });
            }
            f[17] = 255;
            rejects([&] { parse(f); });
        }
        // Exercise RLE sprite scanlines independently, including out-of-bounds runs.
        std::vector<uint8_t> b;
        u32(b, 1);
        u32(b, 6);
        u32(b, 0);
        u16(b, 2);
        u16(b, 1);
        u16(b, 0);
        u16(b, 0);
        u16(b, 5);
        u16(b, 0);
        b.insert(b.end(), { 2, 0, 130, 0, 12, 13 });
        Reader r{ b };
        check(images(r, 1)[0].pixels == std::vector<uint8_t>({ 12, 13 }));
        b[27] = 1;
        rejects([&] { Reader bad{ b }; images(bad, 1); });
        std::mt19937 rng(20260923);
        for (int i = 0; i < 2000; ++i)
        {
            auto f = fixture(i % 2 == 0);
            for (int j = 0; j < 3; ++j)
            {
                f[rng() % f.size()] = static_cast<uint8_t>(rng());
            }
            try
            {
                parse(f);
            }
            catch (const std::exception&)
            {
            }
        }
        std::cout << "Decoder, definitions, malformed files and sprite bounds: PASS\n";
        if (argc > 1)
        {
            const std::filesystem::path root = argv[1];
            readPalette(root / "Data" / "g1.dat");
            unsigned rides = 0, entrances = 0, rejected = 0;
            for (const auto& e : std::filesystem::directory_iterator(root / "ObjData"))
            {
                if (!e.is_regular_file())
                {
                    continue;
                }
                auto file = readFile(e.path());
                if (file.size() < 21 || ((file[0] & 15) != 0 && (file[0] & 15) != 8))
                {
                    continue;
                }
                try
                {
                    auto d = parse(file);
                    d.type == 0 ? ++rides : ++entrances;
                }
                catch (const std::exception&)
                {
                    ++rejected;
                }
            }
            std::cout << "Real installation: " << rides << " rides, " << entrances << " entrances, " << rejected << " rejected\n";
            check(rides > 0 && entrances > 0 && rejected == 0);
        }
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
