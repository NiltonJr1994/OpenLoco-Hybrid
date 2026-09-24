#include "../src/Hybrid/ParkPersistence.h"
#include "../src/Hybrid/ParkWindows.h"
#include <OpenLoco/GameState.h>
#include <OpenLoco/Graphics/RenderTarget.h>
#include <OpenLoco/Graphics/SoftwareDrawingContext.h>
#include <OpenLoco/Localisation/Formatting.h>
#include <chrono>
#include <fstream>
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
    context.pushRenderTarget({}); // preserve the context base frame in debug builds
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

TEST_F(HybridTest, WindowTextRendersInsideAnOffsetClippedWindow)
{
    // Reproduces the blank window when it is away from the screen origin.
    auto* glyph = Gfx::getG1Element(ImageIds::characters_medium_normal_space + 224 + 'A' - 32);
    const auto savedGlyph = *glyph;
    auto* textPalette = Gfx::getG1Element(ImageIds::text_palette);
    const auto savedTextPalette = *textPalette;
    std::array<uint8_t, 4> textColours{ 10, 10, 10, 0 };
    textPalette->offset = textColours.data();
    std::array<uint8_t, 4> ink{ 1, 1, 1, 1 };
    glyph->offset = ink.data();
    glyph->width = 2;
    glyph->height = 2;
    glyph->xOffset = 0;
    glyph->yOffset = 0;
    glyph->flags = Gfx::G1ElementFlags::hasTransparency;
    std::array<uint8_t, 64 * 64> pixels;
    pixels.fill(99);
    Gfx::SoftwareDrawingContext context;
    context.pushRenderTarget({});
    context.pushRenderTarget({ pixels.data(), 400, 200, 64, 64, 0 });
    ASSERT_TRUE(context.pushClip(Ui::Rect(400, 200, 64, 64)));
    Gfx::TextRenderer renderer(context);
    Ui::Window window({ 400, 200 }, { 64, 64 });
    // Supply the text palette explicitly; unit tests do not load game graphics.
    const std::string text = "A";
    ParkWindows::drawText(window, renderer, 12, 27, text);
    context.popClip();
    context.popRenderTarget();
    *glyph = savedGlyph;
    *textPalette = savedTextPalette;
    EXPECT_NE(pixels[27 * 64 + 12], 99);
    EXPECT_EQ(pixels[0], 99);
}

namespace
{
    class HybridMapTest : public HybridTest
    {
    protected:
        World::Pos2 centre{ 640, 640 };
        RoadObject roadObject{};
        LandObject landObject{};
        TreeObject treeObject{};
        Parks::SiteObjects objects;
        std::array<Gfx::G1Element, 31> savedMaps;
        std::array<std::array<uint8_t, 256>, 31> maps{};
        void SetUp() override
        {
            HybridTest::SetUp();
            World::TileManager::allocateMapElements();
            World::TileManager::initialise();
            for (auto& t : getGameState().towns)
            {
                t.name = StringIds::null;
            }
            auto& town = getGameState().towns[0];
            town.name = 1;
            town.x = 640;
            town.y = 480;
            town.numBuildings = 10;
            for (auto& factor : getGameState().currencyMultiplicationFactor)
            {
                factor = 1024;
            }
            CompanyManager::get(CompanyId(0))->cash = currency48_t(100000);
            CompanyManager::get(CompanyId(0))->challengeFlags = CompanyFlags::none;
            roadObject.flags = RoadObjectFlags::isRoad;
            landObject.costFactor = 10;
            landObject.costIndex = 8;
            treeObject.clearCostFactor = 80;
            treeObject.costIndex = 8;
            objects.road = [&](size_t) { return &roadObject; };
            objects.land = [&](size_t) { return &landObject; };
            objects.tree = [&](size_t) { return &treeObject; };
            for (size_t c = 0; c < 31; ++c)
            {
                auto* g = Gfx::getG1Element(ImageIds::paletteMapBlack + c);
                savedMaps[c] = *g;
                for (size_t i = 0; i < 256; ++i)
                {
                    maps[c][i] = static_cast<uint8_t>(10 + (c * 7 + i) % 220);
                }
                g->offset = maps[c].data();
                g->width = 256;
                g->height = 1;
            }
            Colours::initColourMap();
            auto entrance = std::make_shared<Rct2::Definition>(*definition);
            entrance->identity[0] = 8;
            entrance->sprites.clear();
            for (int i = 0; i < 12; ++i)
            {
                entrance->sprites.push_back({ 96, 120, -48, -104, std::vector<uint8_t>(96 * 120, 100) });
            }
            Rct2Assets::_registry.entrances = { entrance };
            for (auto type : { 21, 33, 37, 52 })
            {
                auto d = std::make_shared<Rct2::Definition>(*definition);
                d->identity[5] = static_cast<uint8_t>(type);
                d->rideTypes = { static_cast<uint8_t>(type), 255, 255 };
                Rct2Assets::_registry.rides.push_back(d);
            }
        }
        void TearDown() override
        {
            for (size_t i = 0; i < 31; ++i)
            {
                *Gfx::getG1Element(ImageIds::paletteMapBlack + i) = savedMaps[i];
            }
            HybridTest::TearDown();
        }
        World::RoadElement& addRoad(uint8_t rotation)
        {
            auto pos = Parks::roadPosition(centre, rotation);
            auto* entry = World::TileManager::insertElement<World::RoadElement>(pos, 4, 15);
            auto& r = entry->get<World::RoadElement>();
            r.setRoadId(0);
            r.setOwner(CompanyId::neutral);
            r.setRoadObjectId(0);
            return r;
        }
    };
}

TEST_F(HybridMapTest, PlacementRequiresGroundRoadInChosenDirectionAndNeverShowsOnWater)
{
    EXPECT_FALSE(Parks::quoteSite(centre, 0, 0, objects).valid);
    auto& road = addRoad(0);
    EXPECT_TRUE(Parks::quoteSite(centre, 0, 0, objects).valid);
    for (uint8_t r = 1; r < 4; ++r)
    {
        EXPECT_FALSE(Parks::quoteSite(centre, r, 0, objects).valid);
    }
    road.setHasBridge(true);
    EXPECT_FALSE(Parks::quoteSite(centre, 0, 0, objects).valid);
    road.setHasBridge(false);
    road.setGhost(true);
    EXPECT_FALSE(Parks::quoteSite(centre, 0, 0, objects).valid);
    road.setGhost(false);
    Parks::movePreview(centre, objects);
    ASSERT_TRUE(Parks::_preview);
    EXPECT_TRUE(Parks::_parks.empty());
    EXPECT_FALSE(Parks::contains(centre));
    auto* surface = World::TileManager::get(centre).surface();
    surface->setWater(8);
    Parks::movePreview(centre, objects);
    EXPECT_FALSE(Parks::_preview);
    EXPECT_FALSE(Parks::createPark(centre, objects));
    EXPECT_TRUE(Parks::_parks.empty());
    surface->setWater(0);
    surface->setSlope(World::SurfaceSlope::doubleHeight | 1);
    EXPECT_FALSE(Parks::quoteSite(centre, 0, 0, objects).valid);
}

TEST_F(HybridMapTest, ConstructionDebitsQuotedCostOnceAndCreatesTheChosenModel)
{
    addRoad(0);
    const auto quote = Parks::quoteSite(centre, 0, 0, objects);
    ASSERT_TRUE(quote.valid);
    const auto cash = CompanyManager::get(CompanyId(0))->cash.asInt64();
    auto* park = Parks::createPark(centre, objects);
    ASSERT_NE(park, nullptr);
    EXPECT_EQ(park->rides.size(), 3u);
    EXPECT_EQ(park->rotation, 0);
    EXPECT_EQ(park->model, 0);
    EXPECT_EQ(CompanyManager::get(CompanyId(0))->cash.asInt64(), cash - quote.total());
    EXPECT_EQ(Parks::selectedPark(), park);
    EXPECT_FALSE(Parks::_preview);
    EXPECT_FALSE(Parks::createPark(centre, objects));
    EXPECT_EQ(Parks::_parks.size(), 1u);
    EXPECT_EQ(CompanyManager::get(CompanyId(0))->cash.asInt64(), cash - quote.total());
}

TEST_F(HybridMapTest, FailedFundingLeavesTerrainAndBalanceUntouched)
{
    addRoad(0);
    auto* s = World::TileManager::get(centre + World::Pos2(32, 0)).surface();
    s->setSlope(1);
    auto q = Parks::quoteSite(centre, 0, 0, objects);
    ASSERT_TRUE(q.valid);
    EXPECT_GT(q.landscaping, 0);
    auto* company = CompanyManager::get(CompanyId(0));
    company->cash = currency48_t(1);
    EXPECT_FALSE(Parks::createPark(centre, objects));
    EXPECT_EQ(s->slope(), 1);
    EXPECT_EQ(company->cash.asInt64(), 1);
    EXPECT_TRUE(Parks::_parks.empty());
}

TEST_F(HybridMapTest, ClearingAndLevellingAreQuotedWithoutChangingTheMap)
{
    addRoad(0);
    const auto pos = centre + World::Pos2(32, 0);
    auto* s = World::TileManager::get(pos).surface();
    s->setSlope(1);
    auto* e = World::TileManager::insertElement<World::TreeElement>(pos, 4, 15);
    e->get<World::TreeElement>().setTreeObjectId(0);
    const auto q = Parks::quoteSite(centre, 0, 0, objects);
    ASSERT_TRUE(q.valid);
    EXPECT_EQ(q.trees, 1u);
    EXPECT_EQ(q.clearance, 20);
    EXPECT_EQ(q.landscaping, 10);
    EXPECT_EQ(World::TileManager::get(pos).size(), 2u);
    EXPECT_EQ(World::TileManager::get(pos).surface()->slope(), 1);
}

TEST_F(HybridMapTest, MonthlyChargesUseInflationAndCompanyLedgerExactlyOnce)
{
    addRoad(0);
    auto* p = Parks::createPark(centre, objects);
    ASSERT_NE(p, nullptr);
    auto* company = CompanyManager::get(CompanyId(0));
    const auto cash = company->cash.asInt64();
    const auto expenses = company->expenditures[0][ExpenditureType::Miscellaneous];
    Parks::updateMonthly();
    EXPECT_EQ(company->cash.asInt64(), cash);
    p->lastChargedMonth = Parks::monthKey() - 1;
    const auto charge = Parks::monthlyTax(3) + Parks::monthlyUpkeep(3);
    Parks::updateMonthly();
    EXPECT_EQ(company->cash.asInt64(), cash - charge);
    EXPECT_EQ(company->expenditures[0][ExpenditureType::Miscellaneous], expenses - charge);
    Parks::updateMonthly();
    EXPECT_EQ(company->cash.asInt64(), cash - charge);
    getGameState().currencyMultiplicationFactor[8] *= 2;
    EXPECT_EQ(Parks::monthlyTax(3), 226);
    EXPECT_GT(Parks::monthlyTax(4), Parks::monthlyTax(3));
}

TEST_F(HybridMapTest, EveryModelHasRealDefinitionsSmallGatesAndChangingAnimationFrames)
{
    for (uint8_t m = 0; m < 3; ++m)
    {
        Parks::_model = m;
        const auto park = Parks::makeModel();
        ASSERT_EQ(park.rides.size(), 3u);
        for (int r = 0; r < 4; ++r)
        {
            auto* gate = Rct2Graphics::get(park.entranceImage + r);
            ASSERT_NE(gate, nullptr);
            EXPECT_LE(gate->width, 60);
            EXPECT_LE(gate->height, 44);
        }
        for (const auto& ride : park.rides)
        {
            EXPECT_FALSE(ride.definition->payload.empty());
            const auto& a = Rct2Graphics::_images[ride.image - Rct2Graphics::kFirstImage]->pixels;
            const auto& b = Rct2Graphics::_images[ride.image - Rct2Graphics::kFirstImage + 1]->pixels;
            EXPECT_NE(a, b);
        }
    }
    Parks::reset();
    EXPECT_TRUE(ParkVisuals::_models.empty());
    EXPECT_FALSE(Parks::_groundImage);
}

TEST_F(HybridMapTest, InteriorPlacementUsesIndependentCoordinatesAndChargesOnlyOnSuccess)
{
    Parks::Park park;
    park.owner = CompanyId(0);
    auto d = std::make_shared<Rct2::Definition>(*definition);
    d->type = 1;
    d->sceneryPrice = 20;
    d->sprites.resize(4, d->sprites[0]);
    const auto cash = CompanyManager::get(park.owner)->cash.asInt64();
    EXPECT_FALSE(ParkInterior::place(park, d, { 3, 6 }, 0)); // keep paths clear
    EXPECT_TRUE(ParkInterior::place(park, d, { 3, 4 }, 2));
    ASSERT_EQ(park.scenery.size(), 1u);
    EXPECT_EQ(park.scenery[0].tile, World::Pos2(3, 4));
    EXPECT_EQ(park.scenery[0].rotation, 2);
    EXPECT_EQ(CompanyManager::get(park.owner)->cash.asInt64(), cash - 20);
    EXPECT_FALSE(ParkInterior::place(park, d, { 3, 4 }, 0));
    EXPECT_EQ(CompanyManager::get(park.owner)->cash.asInt64(), cash - 20);
    CompanyManager::get(park.owner)->cash = currency48_t(0);
    CompanyManager::get(park.owner)->currentLoan = 0;
    EXPECT_FALSE(ParkInterior::place(park, d, { 4, 4 }, 0));
    EXPECT_EQ(park.scenery.size(), 1u);
    CompanyManager::setControllingId(CompanyId(1));
    EXPECT_FALSE(ParkInterior::place(park, d, { 4, 4 }, 0));
}

TEST(HybridInterior, PickingRoundTripsEveryInteriorTileAndRejectsOutside)
{
    for (coord_t x = 0; x < 12; ++x)
    {
        for (coord_t y = 0; y < 12; ++y)
        {
            const auto p = ParkInterior::project({ x, y });
            const auto tile = ParkInterior::pick(p.x, p.y);
            ASSERT_TRUE(tile);
            EXPECT_EQ(*tile, World::Pos2(x, y));
        }
    }
    EXPECT_FALSE(ParkInterior::pick(-1000, -1000));
    EXPECT_FALSE(ParkInterior::pick(384, 440));
}

TEST_F(HybridMapTest, SidecarRoundTripsObjectsFinanceAndRejectsMismatchedOrCorruptFiles)
{
    addRoad(0);
    auto* park = Parks::createPark(centre, objects);
    ASSERT_NE(park, nullptr);
    auto d = std::make_shared<Rct2::Definition>(*definition);
    d->type = 1;
    d->identity[0] = 1;
    d->sceneryPrice = 20;
    d->sprites.resize(4, d->sprites[0]);
    Rct2Assets::_registry.scenery.push_back(d);
    ASSERT_TRUE(ParkInterior::place(*park, d, { 2, 3 }, 3));
    park->lastTax = 42;
    park->lastOperatingCost = 21;
    const auto bytes = ParkPersistence::encode(Parks::_parks, 12345);
    const auto cash = CompanyManager::get(park->owner)->cash.asInt64();
    auto restored = ParkPersistence::decode(bytes, 12345);
    ASSERT_EQ(restored.size(), 1u);
    EXPECT_EQ(restored[0].position, centre);
    EXPECT_EQ(restored[0].owner, park->owner);
    EXPECT_EQ(restored[0].paidConstruction, park->paidConstruction);
    EXPECT_EQ(restored[0].lastChargedMonth, park->lastChargedMonth);
    EXPECT_EQ(restored[0].lastTax, 42);
    EXPECT_EQ(restored[0].lastOperatingCost, 21);
    EXPECT_EQ(restored[0].paidInterior, 20);
    ASSERT_EQ(restored[0].scenery.size(), 1u);
    EXPECT_EQ(restored[0].scenery[0].definition, d);
    EXPECT_EQ(restored[0].scenery[0].tile, World::Pos2(2, 3));
    EXPECT_EQ(restored[0].scenery[0].rotation, 3);
    EXPECT_EQ(CompanyManager::get(park->owner)->cash.asInt64(), cash);
    EXPECT_THROW(ParkPersistence::decode(bytes, 67890), std::runtime_error);
    auto corrupt = bytes;
    corrupt[35] ^= 1;
    EXPECT_THROW(ParkPersistence::decode(corrupt, 12345), std::runtime_error);
    for (size_t n : { 0U, 20U, 28U })
    {
        EXPECT_THROW(ParkPersistence::decode(std::span(bytes).first(n), 12345), std::runtime_error);
    }
    Rct2Assets::_registry.scenery.clear();
    EXPECT_THROW(ParkPersistence::decode(bytes, 12345), std::runtime_error);
    EXPECT_EQ(Parks::_parks.size(), 1u); // failed decode never mutates live parks
}

TEST_F(HybridMapTest, SidecarRebuildsSpritesAfterResetWithoutChargingCompany)
{
    addRoad(0);
    auto* park = Parks::createPark(centre, objects);
    ASSERT_NE(park, nullptr);
    const auto owner = park->owner;
    auto* company = CompanyManager::get(owner);
    company->name = 1;
    const auto cash = company->cash.asInt64();
    const auto data = ParkPersistence::encode(Parks::_parks, 91);
    auto saved = ParkPersistence::decode(data, 91);
    Parks::reset();
    ParkPersistence::restore(std::move(saved));
    ASSERT_EQ(Parks::_parks.size(), 1u);
    EXPECT_NE(Gfx::getG1Element(Parks::_parks[0].entranceImage), nullptr);
    EXPECT_NE(Gfx::getG1Element(Parks::_parks[0].rides[0].image), nullptr);
    EXPECT_EQ(company->cash.asInt64(), cash);
    Parks::updateMonthly();
    EXPECT_EQ(company->cash.asInt64(), cash);
    EXPECT_EQ(Parks::_nextParkId, 2);
}

TEST_F(HybridTest, SidecarFileIsBoundToBaseSaveAndCanBeReplaced)
{
    const auto folder = std::filesystem::temp_directory_path() / ("openloco-hybrid-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    ASSERT_TRUE(std::filesystem::create_directory(folder));
    const auto base = folder / "test.SV5";
    {
        std::ofstream stream(base);
        stream << "save one";
    }
    EXPECT_TRUE(ParkPersistence::read(base).empty());
    ParkPersistence::save(base);
    EXPECT_TRUE(std::filesystem::exists(ParkPersistence::sidecarPath(base)));
    EXPECT_TRUE(ParkPersistence::read(base).empty());
    {
        std::ofstream stream(base);
        stream << "save two";
    }
    EXPECT_THROW(ParkPersistence::read(base), std::runtime_error);
    ParkPersistence::save(base);
    EXPECT_TRUE(ParkPersistence::read(base).empty());
    std::filesystem::remove(base);
    std::filesystem::remove(ParkPersistence::sidecarPath(base));
    auto backup = ParkPersistence::sidecarPath(base);
    backup += ".bak";
    std::filesystem::remove(backup);
    std::filesystem::remove(folder);
}
