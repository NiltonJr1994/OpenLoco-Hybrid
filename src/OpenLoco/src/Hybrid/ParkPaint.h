#pragma once
#include "Hybrid/ParkManager.h"
#include "Paint/Paint.h"
#include "Ui/ViewportInteraction.h"

namespace OpenLoco::Hybrid
{
    inline void paintPark(Paint::PaintSession& session, const World::Pos2& loc, coord_t height)
    {
        for (const auto& park : Parks::_parks)
        {
            const int dx = (loc.x - park.position.x) / 32;
            const int dy = (loc.y - park.position.y) / 32;
            if (std::abs(dx) > 3 || std::abs(dy) > 3)
            {
                continue;
            }
            session.setItemType(Ui::ViewportInteraction::InteractionItem::noInteraction);
            if (dy == 3 && dx >= -1 && dx <= 1)
            {
                // Three real entrance parts; orientation follows the viewport.
                const uint32_t part = dx == 0 ? 0 : (dx < 0 ? 1 : 2);
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
        }
    }
}
