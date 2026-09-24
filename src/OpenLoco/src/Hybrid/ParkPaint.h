#pragma once
#include "Hybrid/ParkManager.h"
#include "Paint/Paint.h"
#include "Ui/ViewportInteraction.h"
namespace OpenLoco::Hybrid
{
    inline void paintPark(Paint::PaintSession& session, const World::Pos2& loc, [[maybe_unused]] coord_t surfaceHeight)
    {
        const auto paint = [&](const Parks::Park& park) {
            const auto delta = loc - park.position;
            const auto local = Parks::rotate(delta, (4 - park.rotation) & 3);
            const int dx = local.x / 32, dy = local.y / 32;
            if (std::abs(dx) > 3 || std::abs(dy) > 3)
            {
                return;
            }
            const auto height = park.height;
            session.setItemType(Ui::ViewportInteraction::InteractionItem::noInteraction);
            const bool path = dx == 0 || dy == 0 || std::abs(dx) == 3 || std::abs(dy) == 3;
            session.addToPlotListAsParent(ImageId(park.groundImage + (path ? 1 : 0)), { 16, 16, static_cast<coord_t>(height + 1) }, { 0, 0, static_cast<coord_t>(height + 1) }, { 32, 32, 1 });
            const auto view = (session.getRotation() + park.rotation) & 3;
            if (dx == 3 && dy == 0)
            {
                session.addToPlotListAsParent(ImageId(park.entranceImage + view), { 16, 16, height }, { 2, 2, height }, { 28, 28, 48 });
            }
            for (const auto& ride : park.rides)
            {
                if (dx != ride.site.x || dy != ride.site.y)
                {
                    continue;
                }
                const auto frame = ride.kind == ParkVisuals::Kind::object ? 0 : view * ParkVisuals::kFrames + (Parks::_animationTicks / 4) % ParkVisuals::kFrames;
                session.addToPlotListAsParent(ImageId(ride.image + frame), { 16, 16, height }, { 2, 2, height }, { 28, 28, 56 });
            }
        };
        for (const auto& park : Parks::_parks)
        {
            paint(park);
        }
        if (Parks::_preview)
        {
            paint(*Parks::_preview);
        }
    }
}
