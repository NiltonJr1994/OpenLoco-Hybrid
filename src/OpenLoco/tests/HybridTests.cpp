#include "../src/Hybrid/ParkManager.h"
#include <OpenLoco/Graphics/RenderTarget.h>
#include <OpenLoco/Graphics/SoftwareDrawingContext.h>
#include <gtest/gtest.h>

using namespace OpenLoco;
using namespace OpenLoco::Hybrid;

namespace
{
    class HybridTest : public ::testing::Test
    {
        Gfx::G1Element savedPalette;
        std::array<uint8_t, 768> palette{};
        SceneManager::Flags savedFlags{};
        CompanyId savedCompany{};

    protected:
        std::shared_ptr<Rct2::Definition> definition;
        void SetUp() override
        {
            Parks::reset();
            savedFlags = SceneManager::getSceneFlags();
            savedCompany = CompanyManager::getControllingId();
            SceneManager::setSceneFlags(SceneManager::Flags::none);
            CompanyManager::setControllingId(CompanyId(0));
            auto* element = Gfx::getG1Element(ImageIds::default_palette);
            savedPalette = *element;
            for (size_t i = 0; i < 256; ++i)
            {
                for (size_t c = 0; c < 3; ++c)
                {
                    palette[i * 3 + c] = static_cast<uint8_t>(i);
                }
            }
            element->offset = palette.data();
            element->width = 256;
            element->xOffset = 0;
            Rct2Assets::_registry = {};
            Rct2Assets::_registry.scanned = true;
            Rct2Assets::_registry.ready = true;
            for (size_t i = 0; i < 256; ++i)
            {
                Rct2Assets::_registry.palette[i].fill(static_cast<uint8_t>(i));
            }
            definition = std::make_shared<Rct2::Definition>();
            definition->id = "TEST";
            definition->name = "Test ride";
            definition->payload = { 1, 2, 3 };
            definition->sprites.push_back({ 2, 2, 0, 0, { 0, 50, 100, 150 } });
            Rct2Assets::_registry.rides.push_back(definition);
            Rct2Assets::_selectedRide = 0;
        }
        void TearDown() override
        {
            Parks::reset();
            Rct2Assets::_registry = {};
            *Gfx::getG1Element(ImageIds::default_palette) = savedPalette;
            SceneManager::setSceneFlags(savedFlags);
            CompanyManager::setControllingId(savedCompany);
        }
    };
}

TEST_F(HybridTest, NativeImageRangeRendersWithTransparencyAndLeavesLocoSlotUntouched)
{
    const auto oldSlot = *Gfx::getG1Element(1234);
    const auto id = Rct2Graphics::load(definition);
    EXPECT_GE(id, Rct2Graphics::kFirstImage);
    ASSERT_NE(Gfx::getG1Element(id), nullptr);
    EXPECT_EQ(Rct2Graphics::load(definition), id);
    std::array<uint8_t, 16> pixels;
    pixels.fill(7);
    Gfx::SoftwareDrawingContext context;
    context.pushRenderTarget({ pixels.data(), 0, 0, 4, 4, 0 });
    context.drawImage(ZoomLevel::full, { 1, 1 }, ImageId(id));
    context.popRenderTarget();
    EXPECT_EQ(pixels[5], 7); // transparent input must preserve background
    EXPECT_EQ(pixels[6], 50);
    EXPECT_EQ(pixels[9], 100);
    EXPECT_EQ(pixels[10], 150);
    EXPECT_EQ(Gfx::getG1Element(1234)->offset, oldSlot.offset);
    EXPECT_EQ(Gfx::getG1Element(1234)->width, oldSlot.width);
    Parks::reset();
    EXPECT_EQ(Gfx::getG1Element(id), nullptr);
}

TEST_F(HybridTest, InstancesBelongToSelectedParkRetainDefinitionAndRespectCapacity)
{
    Parks::Park first;
    first.id = 1;
    first.owner = CompanyId(0);
    first.position = { 320, 320 };
    Parks::Park second;
    second.id = 2;
    second.owner = CompanyId(0);
    second.position = { 640, 640 };
    Parks::_parks = { first, second };
    Parks::_selectedParkId = 2;
    ASSERT_TRUE(Parks::instantiateSelectedRide());
    EXPECT_TRUE(Parks::_parks[0].rides.empty());
    ASSERT_EQ(Parks::_parks[1].rides.size(), 1u);
    EXPECT_EQ(Parks::_parks[1].rides[0].definition, definition);
    EXPECT_EQ(Parks::_parks[1].rides[0].definition->payload, definition->payload);
    for (int i = 1; i < 9; ++i)
    {
        EXPECT_TRUE(Parks::instantiateSelectedRide());
    }
    EXPECT_FALSE(Parks::instantiateSelectedRide());
    EXPECT_EQ(Parks::_parks[1].rides.size(), 9u);
    EXPECT_TRUE(Parks::contains({ 544, 544 }));
    EXPECT_FALSE(Parks::contains({ 512, 544 }));
    Parks::_selectedParkId = 1;
    CompanyManager::setControllingId(CompanyId(1));
    EXPECT_FALSE(Parks::instantiateSelectedRide());
    CompanyManager::setControllingId(CompanyId(0));
    SceneManager::setSceneFlags(SceneManager::Flags::networked);
    EXPECT_FALSE(Parks::instantiateSelectedRide());
    Parks::reset();
    EXPECT_FALSE(Parks::contains({ 640, 640 }));
    EXPECT_EQ(Parks::selectedPark(), nullptr);
}
