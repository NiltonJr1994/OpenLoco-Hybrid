#pragma once
#include "Graphics/DrawingContext.h"
#include "Graphics/RenderTarget.h"
#include "Graphics/TextRenderer.h"
#include "Hybrid/ParkManager.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/Formatting.h"
#include "Localisation/StringManager.h"
#include "Objects/InterfaceSkinObject.h"
#include "Ui/Widgets/ButtonWidget.h"
#include "Ui/Widgets/CaptionWidget.h"
#include "Ui/Widgets/FrameWidget.h"
#include "Ui/Widgets/ImageButtonWidget.h"
#include "Ui/Widgets/PanelWidget.h"
#include "Ui/Widgets/ScrollViewWidget.h"
#include <cmath>

namespace OpenLoco::Hybrid::ParkInterior
{
    using namespace Ui;
    constexpr int kSide = 12;
    inline uint16_t _parkId{};
    inline size_t _object{};
    inline uint8_t _rotation{};
    inline bool _remove{};
    inline std::optional<World::Pos2> _hover;
    inline std::string _status;
    inline std::vector<std::shared_ptr<const Rct2::Definition>> _catalogue;
    constexpr WidgetId kClose{ "interior_close" }, kPrevious{ "interior_previous" }, kNext{ "interior_next" }, kRotate{ "interior_rotate" }, kRemove{ "interior_remove" };
    constexpr Size kSize{ 800, 540 };
    constexpr StringId kTitle = 2481, kPrevText = 2482, kNextText = 2483, kRotateText = 2484, kRemoveText = 2485;
    inline constexpr auto kWidgets = makeWidgets(
        Widgets::Frame({ 0, 0 }, kSize, WindowColour::primary),
        Widgets::Caption({ 1, 1 }, { 798, 13 }, Widgets::Caption::Style::whiteText, WindowColour::primary, kTitle),
        Widgets::ImageButton(kClose, { 785, 2 }, { 13, 13 }, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
        Widgets::Panel({ 0, 15 }, { 800, 525 }, WindowColour::secondary),
        Widgets::Button(kPrevious, { 10, 42 }, { 125, 24 }, WindowColour::secondary, kPrevText),
        Widgets::Button(kNext, { 140, 42 }, { 125, 24 }, WindowColour::secondary, kNextText),
        Widgets::Button(kRotate, { 270, 42 }, { 125, 24 }, WindowColour::secondary, kRotateText),
        Widgets::Button(kRemove, { 400, 42 }, { 125, 24 }, WindowColour::secondary, kRemoveText),
        Widgets::ScrollView({ 196, 92 }, { 596, 420 }, WindowColour::secondary, Scrollbars::horizontal | Scrollbars::vertical),
        Widgets::ScrollView({ 8, 92 }, { 180, 420 }, WindowColour::secondary, Scrollbars::vertical));

    inline Ui::Point project(World::Pos2 tile)
    {
        return { static_cast<int16_t>(384 + (tile.y - tile.x) * 32), static_cast<int16_t>(40 + (tile.x + tile.y) * 16) };
    }
    inline std::optional<World::Pos2> pick(int x, int y)
    {
        const auto a = (x - 384) / 32.0, b = (y - 40) / 16.0;
        World::Pos2 tile{ static_cast<coord_t>(std::floor((b - a) / 2 + 0.5)), static_cast<coord_t>(std::floor((b + a) / 2 + 0.5)) };
        if (tile.x < 0 || tile.y < 0 || tile.x >= kSide || tile.y >= kSide)
        {
            return {};
        }
        return tile;
    }
    inline bool path(World::Pos2 tile) { return tile.y == 6 || (tile.x == 6 && tile.y >= 3 && tile.y <= 9); }
    inline bool supported(const Rct2::Definition& d)
    {
        // Only static single-layer scenery is buildable until specialised renderers exist.
        constexpr uint32_t unsupported = (1U << 4) | (1U << 7) | (1U << 9) | (1U << 11) | (1U << 12) | (1U << 13) | (1U << 14) | (1U << 15) | (1U << 22);
        return d.type == 1 && !(d.sceneryFlags & unsupported) && d.sprites.size() == 4 && d.sceneryPrice > 0;
    }
    inline currency32_t cost(const Rct2::Definition& d) { return Economy::getInflationAdjustedCost(d.sceneryPrice, 8, 10); }
    inline bool place(Parks::Park& park, std::shared_ptr<const Rct2::Definition> definition, World::Pos2 tile, uint8_t rotation)
    {
        if (SceneManager::isNetworked() || SceneManager::isEditorMode() || SceneManager::isTitleMode() || park.owner != CompanyManager::getControllingId())
        {
            _status = "This park is not editable by your company.";
            return false;
        }
        if (!definition || !supported(*definition) || rotation > 3 || tile.x < 0 || tile.y < 0 || tile.x >= kSide || tile.y >= kSide || path(tile))
        {
            _status = "Choose a grass tile; paths and the entrance must remain clear.";
            return false;
        }
        if (park.scenery.size() >= 128 || std::any_of(park.scenery.begin(), park.scenery.end(), [&](const auto& s) { return s.tile == tile; }))
        {
            _status = "This tile is occupied, or the park has reached 128 scenery objects.";
            return false;
        }
        try
        {
            const auto image = Rct2Graphics::load(definition);
            park.scenery.reserve(park.scenery.size() + 1);
            const auto price = cost(*definition);
            if (!CompanyManager::ensureCompanyFunding(park.owner, price))
            {
                _status = "Not enough company funds. Nothing was built or charged.";
                return false;
            }
            park.scenery.push_back({ std::move(definition), tile, rotation, image });
            CompanyManager::applyPaymentToCompany(park.owner, price, ExpenditureType::Construction);
            park.paidInterior += price;
            Scenario::getOptions().madeAnyChanges = 1;
            _status = "RCT2 object built. Construction charged to the company.";
            return true;
        }
        catch (const std::exception& e)
        {
            _status = e.what();
            return false;
        }
    }
    inline void label(Gfx::DrawingContext& ctx, int y, const std::string& text)
    {
        char buffer[512]{};
        std::copy_n(text.data(), std::min(text.size(), sizeof(buffer) - 8), buffer);
        Gfx::TextRenderer tr(ctx);
        Gfx::TextRenderer::clipString(tr.getCurrentFont(), 770, buffer);
        tr.drawString({ 10, static_cast<int16_t>(y) }, AdvancedColour::FD(), buffer);
    }
    inline void mouseUp(Window& self, WidgetIndex_t, WidgetId id)
    {
        if (id == kClose)
        {
            WindowManager::close(&self);
            return;
        }
        if (id == kRotate)
        {
            _rotation = (_rotation + 1) & 3;
        }
        if (id == kRemove)
        {
            _remove = !_remove;
        }
        if (!_catalogue.empty() && (id == kPrevious || id == kNext))
        {
            _object = (_object + _catalogue.size() + (id == kNext ? 1 : -1)) % _catalogue.size();
            _remove = false;
        }
        self.invalidate();
    }
    inline void click(Window& self, int16_t x, int16_t y, uint8_t index)
    {
        if (index == 1)
        {
            const auto selected = static_cast<size_t>(std::max<int16_t>(0, y) / 14);
            if (selected < _catalogue.size())
            {
                _object = selected;
                _remove = false;
            }
            self.invalidate();
            return;
        }
        auto* park = Parks::getPark(_parkId);
        const auto tile = pick(x, y);
        if (!park || !tile)
        {
            return;
        }
        if (_remove)
        {
            if (park->owner != CompanyManager::getControllingId() || SceneManager::isNetworked())
            {
                return;
            }
            const auto oldSize = park->scenery.size();
            std::erase_if(park->scenery, [&](const auto& s) { return s.tile == *tile; });
            if (oldSize != park->scenery.size())
            {
                Scenario::getOptions().madeAnyChanges = 1;
            }
            _status = "Scenery removed; no refund.";
        }
        else if (!_catalogue.empty())
        {
            place(*park, _catalogue[_object], *tile, _rotation);
        }
        self.invalidate();
    }
    inline void hover(Window& self, int16_t x, int16_t y, uint8_t index)
    {
        if (index != 0)
        {
            return;
        }
        _hover = pick(x, y);
        self.invalidate();
    }
    inline void size(Window&, uint32_t index, int32_t& w, int32_t& h)
    {
        w = index == 0 ? 780 : 164;
        h = index == 0 ? 416 : static_cast<int32_t>(_catalogue.size() * 14);
    }
    inline void drawScroll(Window&, Gfx::DrawingContext& ctx, uint32_t index)
    {
        if (index == 1)
        {
            ctx.clearSingle(Colours::getShade(Colour::grey, 6));
            Gfx::TextRenderer tr(ctx);
            for (size_t i = 0; i < _catalogue.size(); ++i)
            {
                const auto y = static_cast<int16_t>(i * 14);
                const auto& rt = ctx.currentRenderTarget();
                if (y + 14 < rt.y || y >= rt.y + rt.height)
                {
                    continue;
                }
                if (i == _object)
                {
                    ctx.fillRect(0, y, 164, y + 13, Colours::getShade(Colour::grey, 4), Gfx::RectFlags::none);
                }
                char name[256]{};
                const auto& text = _catalogue[i]->name;
                std::copy_n(text.data(), std::min(text.size(), sizeof(name) - 8), name);
                Gfx::TextRenderer::clipString(tr.getCurrentFont(), 158, name);
                tr.drawString({ 2, y }, AdvancedColour::FD(), name);
            }
            return;
        }
        ctx.clearSingle(Colours::getShade(Colour::mutedGrassGreen, 3));
        const auto* park = Parks::getPark(_parkId);
        if (!park)
        {
            return;
        }
        try
        {
            for (int sum = 0; sum <= 22; ++sum)
            {
                for (int x = 0; x < kSide; ++x)
                {
                    const int y = sum - x;
                    if (y < 0 || y >= kSide)
                    {
                        continue;
                    }
                    const World::Pos2 tile{ static_cast<coord_t>(x), static_cast<coord_t>(y) };
                    ctx.drawImage(ZoomLevel::full, project(tile), ImageId(Parks::groundImage() + (path(tile) ? 1 : 0)));
                }
            }
            // Real three-part RCT2 entrance at its original sprite scale.
            const auto entrance = Rct2Graphics::load(park->entrance);
            for (int part = 1; part >= 0; --part)
            {
                ctx.drawImage(ZoomLevel::full, project({ 11, static_cast<coord_t>(part == 1 ? 5 : 6) }), ImageId(entrance + part));
            }
            ctx.drawImage(ZoomLevel::full, project({ 11, 7 }), ImageId(entrance + 2));
            for (int sum = 0; sum <= 22; ++sum)
            {
                for (const auto& object : park->scenery)
                {
                    if (object.tile.x + object.tile.y == sum)
                    {
                        auto anchor = project(object.tile);
                        if ((object.definition->sceneryFlags & 3) == 3)
                        {
                            anchor.y -= 12;
                        }
                        ctx.drawImage(ZoomLevel::full, anchor, ImageId(object.image + object.rotation));
                    }
                }
            }
            if (_hover)
            {
                const auto p = project(*_hover);
                const auto colour = Colours::getShade(Colour::yellow, 7);
                ctx.drawLine(p + Point{ -31, 0 }, p + Point{ 0, -15 }, colour);
                ctx.drawLine(p + Point{ 0, -15 }, p + Point{ 31, 0 }, colour);
                ctx.drawLine(p + Point{ 31, 0 }, p + Point{ 0, 15 }, colour);
                ctx.drawLine(p + Point{ 0, 15 }, p + Point{ -31, 0 }, colour);
            }
        }
        catch (const std::exception& e)
        {
            _status = e.what();
        }
    }
    inline void draw(Window& self, Gfx::DrawingContext& ctx)
    {
        self.draw(ctx);
        label(ctx, 24, "Park #" + std::to_string(_parkId) + " - Interior construction | Real RCT2 scenery | 12 x 12 tiles");
        if (!_catalogue.empty())
        {
            const auto& d = *_catalogue[_object];
            char amount[128]{};
            FormatArguments args;
            args.push(currency48_t(cost(d)));
            StringManager::formatString(amount, sizeof(amount), StringIds::currency48, args);
            label(ctx, 74, _remove ? "Removal mode: click scenery to remove it. No refund." : d.name + " | Price: " + amount + " | Rotation " + std::to_string(_rotation + 1) + "/4 | Click grass to build");
        }
        label(ctx, 520, _status);
    }
    inline void update(Window& self)
    {
        if (!Parks::getPark(_parkId))
        {
            WindowManager::close(&self);
        }
    }
    inline constexpr WindowEventList kEvents = {
        .onMouseUp = mouseUp,
        .onUpdate = update,
        .getScrollSize = size,
        .scrollMouseDown = click,
        .scrollMouseOver = hover,
        .draw = draw,
        .drawScroll = drawScroll,
    };
    inline void open(uint16_t parkId)
    {
        _parkId = parkId;
        _catalogue.clear();
        for (const auto& d : Rct2Assets::get().scenery)
        {
            if (supported(*d))
            {
                _catalogue.push_back(d);
            }
        }
        std::sort(_catalogue.begin(), _catalogue.end(), [](const auto& a, const auto& b) { return a->name < b->name; });
        _object = 0;
        _rotation = 0;
        _remove = false;
        _hover.reset();
        _status = _catalogue.empty() ? "No supported static RCT2 scenery was loaded." : "Interior editor: scenery is functional; visitors and ride operation are not implemented yet.";
        StringManager::swapString(kTitle, "Hybrid - Park interior");
        StringManager::swapString(kPrevText, "< Scenery");
        StringManager::swapString(kNextText, "Scenery >");
        StringManager::swapString(kRotateText, "Rotate object");
        StringManager::swapString(kRemoveText, "Build / Remove");
        auto* w = WindowManager::bringToFront(WindowType::hybridParks, 1);
        if (!w)
        {
            w = WindowManager::createWindowCentred(WindowType::hybridParks, kSize, WindowFlags::none, kEvents);
            w->number = 1;
            w->setWidgets(kWidgets);
            w->initScrollWidgets();
            if (const auto* skin = ObjectManager::get<InterfaceSkinObject>())
            {
                w->setColour(WindowColour::primary, skin->windowTitlebarColour);
                w->setColour(WindowColour::secondary, skin->windowColour);
            }
        }
        w->invalidate();
    }
}
