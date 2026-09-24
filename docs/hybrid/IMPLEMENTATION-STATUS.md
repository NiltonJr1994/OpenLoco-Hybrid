# Hybrid implementation contract

One executable must provide regional transportation and detailed park simulation with real RCT2 objects. An animated proxy is not an operational ride. Preserve Locomotion DAT/mod and save compatibility.

## Video review: 2026-09-24 13-25-07

The 121.77-second recording confirms construction succeeds. Around 45 seconds the built park appears, but bright outlined paving makes it look like a checkerboard. Enter park keeps the regional information panel. Placement messages remain visible and regional controls still expose object browsing.

## Current implementation

- Subdued textured regional grass and reduced paving.
- A native 12x12 interior scenery editor with an original-scale entrance, real static scenery, rotation, placement/removal, overlap/path checks and company construction costs. Coordinates are independent of the regional map.
- Legacy small-scenery decoding: flags, height, price, frame offsets and four directional sprites. Special animated/glass/overlay objects are excluded from construction until their renderers exist.
- Versioned .SV5.olh companion: exact DAT identities, positions, ownership, models, interior scenery and financial counters. Bounded little-endian records, integrity checksum and base-save fingerprint. Decode validates before importing the map. Loading does not charge construction again.
- The base save format remains unchanged. Save and sidecar are two files: interruption between writes may produce a mismatched pair, which must be rejected. Keep both files together. Missing sidecar means an ordinary save with no Hybrid parks.

Format references: [OpenRCT2 SmallSceneryObject](https://github.com/OpenRCT2/OpenRCT2/blob/develop/src/openrct2/object/SmallSceneryObject.cpp) and [SmallSceneryEntry](https://github.com/OpenRCT2/OpenRCT2/blob/develop/src/openrct2/object/SmallSceneryEntry.h).

## Required next stages, not completed

1. Editable paths/queues, terrain, ride footprints, real ride renderers and object dependencies. Catalogue thumbnails must not stand in for operational rides.
2. TD6 parsing, dependency checks, preview, construction and pricing. SC6/SV6 require separate scenario/park import.
3. Visitors and service transactions, ticket/ride/shop prices, satisfaction, staff and operating costs. Credit the owning company for actual service events, without unconditional revenue.
4. Connect native PASS transportation to park destinations, stations, frequency, capacity and transfers. Preserve generic vehicle/mod compatibility. Optional existing cargo supplies.
5. Competitor park investment and operation through company AI, respecting the same costs, demand and ownership rules.
6. Reputation, closure, native news and bounded company-performance effects. Evolve persistence with migrations and tests.

Each delivered Windows build must pass CI and package validation. Automated tests are not interactive gameplay verification.
