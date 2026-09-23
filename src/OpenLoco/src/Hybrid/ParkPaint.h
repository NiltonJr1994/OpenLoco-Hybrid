#pragma once
#include "Hybrid/ParkManager.h"
#include "Paint/Paint.h"
#include "Ui/ViewportInteraction.h"

namespace OpenLoco::Hybrid
{
    inline void paintPark(Paint::PaintSession& session, const World::Pos2& loc, coord_t height)
    {
        const auto paint = [&](const Parks::Park& park) {
            const int dx = (loc.x - park.position.x) / 32;
            const int dy = (loc.y - park.position.y) / 32;
            if (std::abs(dx) > 3 || std::abs(dy) > 3)
            {
                return;
            }
            session.setItemType(Ui::ViewportInteraction::InteractionItem::noInteraction);
            // The cross and perimeter paths make all 49 reserved tiles visible,
            // including an empty park before its first object is added.
            const bool path = dx == 0 || dy == 0 || std::abs(dx) == 3 || std::abs(dy) == 3;
            session.addToPlotListAsParent(ImageId(park.groundImage + (path ? 1 : 0)), { 16, 16, static_cast<coord_t>(height + 1) }, { 0, 0, static_cast<coord_t>(height + 1) }, { 32, 32, 1 });
            if (dx == 3 && dy >= -1 && dy <= 1)
            {
                // Three real entrance parts; orientation follows the viewport.
                const uint32_t part = dy == 0 ? 0 : (dy < 0 ? 1 : 2);
                session.addToPlotListAsParent(ImageId(park.entranceImage + session.getRotation() * 3 + part), { 16, 16, height }, { 2, 2, height }, { 28, 28, 80 });
            }
            for (size_t i = 0; i < park.rides.size(); ++i)
            {
                if (dx != static_cast<int>(i % 3) * 2 - 2 || dy != static_cast<int>(i / 3) * 2 - 2)
                {
                    continue;
                }
                // The real DAT build-menu image is a regional visual proxy,
                // not a simulated or reconstructed track layout.
                session.addToPlotListAsParent(ImageId(park.rides[i].image), { 16, 16, height }, { 2, 2, height }, { 28, 28, 48 });
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
