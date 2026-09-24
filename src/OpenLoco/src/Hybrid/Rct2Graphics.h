#pragma once
#include "Graphics/Gfx.h"
#include "Graphics/ImageIds.h"
#include "Hybrid/Rct2AssetRegistry.h"

namespace OpenLoco::Hybrid::Rct2Graphics
{
    // Dedicated range above all Locomotion object slots, below ImageId's 19-bit limit.
    constexpr uint32_t kFirstImage = 0x60000;
    constexpr size_t kMaxImages = 8192;
    struct Image
    {
        std::vector<uint8_t> pixels;
        Gfx::G1Element element;
    };
    struct Loaded
    {
        std::shared_ptr<const Rct2::Definition> definition;
        uint32_t base{};
    };
    inline std::vector<std::unique_ptr<Image>> _images;
    inline std::vector<Loaded> _loaded;
    inline Gfx::G1Element* get(uint32_t index)
    {
        if (index < kFirstImage || index - kFirstImage >= _images.size())
        {
            return nullptr;
        }
        return &_images[index - kFirstImage]->element;
    }
    inline void reset()
    {
        _loaded.clear();
        _images.clear();
    }
    // A small, native isometric garden/plaza tile, using Locomotion's palette.
    // Two tiles are shared by every park, never registered as Locomotion objects.
    inline uint32_t loadGround()
    {
        if (_images.size() + 2 > kMaxImages)
        {
            throw std::runtime_error("Native park sprite capacity reached");
        }
        const auto base = kFirstImage + static_cast<uint32_t>(_images.size());
        for (int path = 0; path < 2; ++path)
        {
            auto image = std::make_unique<Image>();
            image->pixels.resize(64 * 32);
            for (int y = 0; y < 32; ++y)
            {
                for (int x = 0; x < 64; ++x)
                {
                    const int diamond = std::abs(2 * x - 63) + 2 * std::abs(2 * y - 31);
                    if (diamond > 63)
                    {
                        continue;
                    }
                    const auto colour = path ? Colour::grey : Colour::mutedGrassGreen;
                    const auto shade = path ? 4 + ((x + y * 3) % 5 == 0) : 4 + ((x * 13 + y * 7) % 11 < 3);
                    image->pixels[y * 64 + x] = Colours::getShade(colour, shade);
                }
            }
            image->element.offset = image->pixels.data();
            image->element.width = 64;
            image->element.height = 32;
            image->element.xOffset = -32;
            image->element.yOffset = -16;
            image->element.flags = Gfx::G1ElementFlags::hasTransparency;
            _images.push_back(std::move(image));
        }
        return base;
    }
    inline uint32_t load(const std::shared_ptr<const Rct2::Definition>& definition)
    {
        for (const auto& loaded : _loaded)
        {
            if (loaded.definition == definition)
            {
                return loaded.base;
            }
        }
        if (!definition || definition->sprites.size() > kMaxImages - _images.size())
        {
            throw std::runtime_error("Native RCT2 sprite capacity reached");
        }
        const auto* locoPalette = Gfx::getG1Element(ImageIds::default_palette);
        if (!locoPalette || !locoPalette->offset)
        {
            throw std::runtime_error("Locomotion palette unavailable");
        }
        std::array<uint8_t, 256> remap{};
        const auto& palette = Rct2Assets::get().palette;
        for (size_t i = 1; i < 256; ++i)
        {
            int best = 1000000;
            for (int j = 10; j < 230; ++j)
            {
                const int offset = j - locoPalette->xOffset;
                if (offset < 0 || offset >= locoPalette->width)
                {
                    continue;
                }
                int distance = 0;
                for (int c = 0; c < 3; ++c)
                {
                    const int delta = palette[i][c] - locoPalette->offset[offset * 3 + c];
                    distance += delta * delta;
                }
                if (distance < best)
                {
                    best = distance;
                    remap[i] = static_cast<uint8_t>(j);
                }
            }
        }
        const auto base = kFirstImage + static_cast<uint32_t>(_images.size());
        for (const auto& sprite : definition->sprites)
        {
            auto image = std::make_unique<Image>();
            image->pixels.reserve(sprite.pixels.size());
            for (const auto pixel : sprite.pixels)
            {
                image->pixels.push_back(remap[pixel]);
            }
            image->element.offset = image->pixels.data();
            image->element.width = sprite.width;
            image->element.height = sprite.height;
            image->element.xOffset = sprite.x;
            image->element.yOffset = sprite.y;
            image->element.flags = Gfx::G1ElementFlags::hasTransparency;
            _images.push_back(std::move(image));
        }
        _loaded.push_back({ definition, base });
        return base;
    }
}
