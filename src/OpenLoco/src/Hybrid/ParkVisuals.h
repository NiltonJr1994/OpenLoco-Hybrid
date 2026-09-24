#pragma once
#include "Hybrid/Rct2Graphics.h"
#include <cmath>
#include <map>

namespace OpenLoco::Hybrid::ParkVisuals
{
    // Animated regional models, not an RCT2 simulation. Each model is backed by
    // the actual DAT definition held by the park's RideInstance.
    enum class Kind : uint8_t
    {
        wheel,
        carousel,
        coaster,
        slide,
        object
    };
    constexpr uint32_t kFrames = 8;
    inline std::map<Kind, uint32_t> _models;
    inline std::map<const Rct2::Definition*, uint32_t> _entrances;
    inline std::map<const Rct2::Definition*, uint32_t> _thumbnails;
    inline void reset()
    {
        _models.clear();
        _entrances.clear();
        _thumbnails.clear();
    }

    inline uint32_t store(std::vector<uint8_t> pixels, int16_t width, int16_t height, int16_t x, int16_t y)
    {
        if (Rct2Graphics::_images.size() >= Rct2Graphics::kMaxImages)
        {
            throw std::runtime_error("Native park sprite capacity reached");
        }
        auto image = std::make_unique<Rct2Graphics::Image>();
        image->pixels = std::move(pixels);
        image->element.offset = image->pixels.data();
        image->element.width = width;
        image->element.height = height;
        image->element.xOffset = x;
        image->element.yOffset = y;
        image->element.flags = Gfx::G1ElementFlags::hasTransparency;
        auto id = Rct2Graphics::kFirstImage + static_cast<uint32_t>(Rct2Graphics::_images.size());
        Rct2Graphics::_images.push_back(std::move(image));
        return id;
    }

    inline Kind kind(const Rct2::Definition& def)
    {
        // Legacy RCT2 ride type values, independent of Locomotion object IDs.
        for (auto type : def.rideTypes)
        {
            if (type == 37)
            {
                return Kind::wheel;
            }
            if (type == 33)
            {
                return Kind::carousel;
            }
            if (type == 52)
            {
                return Kind::coaster;
            }
            if (type == 21)
            {
                return Kind::slide;
            }
        }
        return Kind::object;
    }

    struct Canvas
    {
        std::vector<uint8_t> pixels = std::vector<uint8_t>(96 * 96);
        uint8_t rotation{};
        Ui::Point project(double x, double y, double z) const
        {
            for (int i = 0; i < rotation; ++i)
            {
                const auto old = x;
                x = -y;
                y = old;
            }
            return { static_cast<int16_t>(48 + std::lround(y - x)), static_cast<int16_t>(78 + std::lround((x + y) / 2 - z)) };
        }
        void dot(Ui::Point p, uint8_t colour, int radius = 0)
        {
            for (int y = p.y - radius; y <= p.y + radius; ++y)
            {
                for (int x = p.x - radius; x <= p.x + radius; ++x)
                {
                    if (x >= 0 && x < 96 && y >= 0 && y < 96)
                    {
                        pixels[y * 96 + x] = colour;
                    }
                }
            }
        }
        void line(Ui::Point a, Ui::Point b, uint8_t colour)
        {
            const int steps = std::max(std::abs(b.x - a.x), std::abs(b.y - a.y));
            for (int i = 0; i <= steps; ++i)
            {
                dot({ static_cast<int16_t>(a.x + (b.x - a.x) * i / std::max(1, steps)), static_cast<int16_t>(a.y + (b.y - a.y) * i / std::max(1, steps)) }, colour);
            }
        }
    };

    inline uint32_t animated(Kind type)
    {
        if (auto it = _models.find(type); it != _models.end())
        {
            return it->second;
        }
        const auto base = Rct2Graphics::kFirstImage + static_cast<uint32_t>(Rct2Graphics::_images.size());
        constexpr double pi = 3.141592653589793;
        const auto white = Colours::getShade(Colour::white, 7);
        const auto red = Colours::getShade(Colour::red, 6);
        const auto blue = Colours::getShade(Colour::blue, 6);
        const auto yellow = Colours::getShade(Colour::yellow, 6);
        const auto dark = Colours::getShade(Colour::grey, 3);
        for (uint8_t rotation = 0; rotation < 4; ++rotation)
        {
            for (uint32_t frame = 0; frame < kFrames; ++frame)
            {
                Canvas c;
                c.rotation = rotation;
                const auto phase = frame * 2 * pi / kFrames;
                if (type == Kind::wheel)
                {
                    auto hub = c.project(0, 0, 28);
                    c.line(c.project(-14, -5, 0), hub, white);
                    c.line(c.project(14, 5, 0), hub, white);
                    for (int n = 0; n < 48; ++n)
                    {
                        const auto a = n * 2 * pi / 48;
                        const auto b = (n + 1) * 2 * pi / 48;
                        c.line(c.project(21 * std::cos(a), 0, 28 + 21 * std::sin(a)), c.project(21 * std::cos(b), 0, 28 + 21 * std::sin(b)), red);
                    }
                    for (int n = 0; n < 8; ++n)
                    {
                        const auto a = n * 2 * pi / 8 + phase / 8;
                        const auto gondola = c.project(21 * std::cos(a), 0, 28 + 21 * std::sin(a));
                        c.line(hub, gondola, white);
                        c.dot(gondola, n % 2 ? blue : yellow, 2);
                    }
                }
                else if (type == Kind::carousel)
                {
                    for (int n = 0; n < 32; ++n)
                    {
                        const auto a = n * 2 * pi / 32;
                        c.line(c.project(0, 0, 29), c.project(19 * std::cos(a), 19 * std::sin(a), 17), n % 4 < 2 ? red : white);
                        c.dot(c.project(20 * std::cos(a), 20 * std::sin(a), 1), yellow);
                    }
                    for (int n = 0; n < 6; ++n)
                    {
                        const auto a = n * 2 * pi / 6 + phase;
                        c.line(c.project(15 * std::cos(a), 15 * std::sin(a), 2), c.project(15 * std::cos(a), 15 * std::sin(a), 16), dark);
                        c.dot(c.project(15 * std::cos(a), 15 * std::sin(a), 7 + 2 * std::sin(a * 2)), yellow, 2);
                    }
                }
                else if (type == Kind::coaster)
                {
                    const auto point = [&](double a) { return c.project(24 * std::cos(a), 19 * std::sin(a), 15 + 10 * std::sin(a * 2)); };
                    for (int n = 0; n < 48; ++n)
                    {
                        const auto a = n * 2 * pi / 48;
                        if (n % 4 == 0)
                        {
                            c.line(c.project(24 * std::cos(a), 19 * std::sin(a), 0), point(a), dark);
                        }
                        c.line(point(a), point((n + 1) * 2 * pi / 48), red);
                    }
                    for (int n = 0; n < 3; ++n)
                    {
                        c.dot(point(phase - n * 0.16), n == 0 ? yellow : blue, 2);
                    }
                }
                else
                {
                    c.line(c.project(0, 0, 0), c.project(0, 0, 42), white);
                    for (int n = 0; n < 96; ++n)
                    {
                        const auto a = n * 4 * pi / 96;
                        c.line(c.project(12 * std::cos(a), 12 * std::sin(a), 40 - n * 0.38), c.project(12 * std::cos(a + 0.14), 12 * std::sin(a + 0.14), 40 - (n + 1) * 0.38), yellow);
                    }
                    c.dot(c.project(12 * std::cos(phase * 2), 12 * std::sin(phase * 2), 40 - frame * 4.6), red, 2);
                }
                store(std::move(c.pixels), 96, 96, -48, -78);
            }
        }
        _models.emplace(type, base);
        return base;
    }

    inline uint32_t thumbnail(const std::shared_ptr<const Rct2::Definition>& def)
    {
        if (auto it = _thumbnails.find(def.get()); it != _thumbnails.end())
        {
            return it->second;
        }
        auto* source = Rct2Graphics::get(Rct2Graphics::load(def));
        const int scale = std::max({ 1, (source->width + 39) / 40, (source->height + 35) / 36 });
        const int w = std::max(1, source->width / scale), h = std::max(1, source->height / scale);
        std::vector<uint8_t> pixels(w * h);
        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                pixels[y * w + x] = source->offset[(y * scale) * source->width + x * scale];
            }
        }
        const auto id = store(std::move(pixels), w, h, -w / 2, -h);
        _thumbnails.emplace(def.get(), id);
        return id;
    }

    inline uint32_t entrance(const std::shared_ptr<const Rct2::Definition>& def)
    {
        if (auto it = _entrances.find(def.get()); it != _entrances.end())
        {
            return it->second;
        }
        const auto source = Rct2Graphics::load(def);
        const auto base = Rct2Graphics::kFirstImage + static_cast<uint32_t>(Rct2Graphics::_images.size());
        for (int rotation = 0; rotation < 4; ++rotation)
        {
            // Composite all three real entrance pieces before reducing their scale.
            std::vector<uint8_t> canvas(512 * 512);
            int minX = 512, minY = 512, maxX = 0, maxY = 0;
            for (int part = 0; part < 3; ++part)
            {
                const auto* sprite = Rct2Graphics::get(source + rotation * 3 + part);
                if (!sprite)
                {
                    throw std::runtime_error("Incomplete RCT2 entrance graphics");
                }
                int dx = 0, dy = part == 0 ? 0 : (part == 1 ? -32 : 32);
                for (int r = 0; r < rotation; ++r)
                {
                    auto old = dx;
                    dx = -dy;
                    dy = old;
                }
                const int ox = 256 + dy - dx + sprite->xOffset, oy = 256 + (dx + dy) / 2 + sprite->yOffset;
                for (int y = 0; y < sprite->height; ++y)
                {
                    for (int x = 0; x < sprite->width; ++x)
                    {
                        auto pixel = sprite->offset[y * sprite->width + x];
                        const auto px = ox + x, py = oy + y;
                        if (pixel && px >= 0 && px < 512 && py >= 0 && py < 512)
                        {
                            canvas[py * 512 + px] = pixel;
                            minX = std::min(minX, px);
                            minY = std::min(minY, py);
                            maxX = std::max(maxX, px);
                            maxY = std::max(maxY, py);
                        }
                    }
                }
            }
            if (minX > maxX)
            {
                throw std::runtime_error("Empty RCT2 entrance graphics");
            }
            const int scale = std::max({ 3, (maxX - minX + 60) / 60, (maxY - minY + 44) / 44 });
            const int w = (maxX - minX) / scale + 1, h = (maxY - minY) / scale + 1;
            std::vector<uint8_t> pixels(w * h);
            for (int y = 0; y < h; ++y)
            {
                for (int x = 0; x < w; ++x)
                {
                    pixels[y * w + x] = canvas[(minY + y * scale) * 512 + minX + x * scale];
                }
            }
            store(std::move(pixels), w, h, -w / 2, -h + 4);
        }
        _entrances.emplace(def.get(), base);
        return base;
    }
}
