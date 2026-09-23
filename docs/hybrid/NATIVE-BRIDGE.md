# Native RCT2 bridge — v0.5.0 alpha

The v0.4 bridge counted DAT headers and launched a bundled OpenRCT2 process. It did
not instantiate RCT2 definitions inside OpenLoco. Its CI also patched the source
tree before building. The launcher, runtime download, template selector and patch
scripts have been removed. The fork now compiles its checked-in native hooks.

## Implemented slice

- RCT2 assets are read only from `RCT2` beside OpenLoco.exe. The reader does not
  register DATs with Locomotion's ObjectManager or change either ObjData directory.
- A bounded DAT decoder supports raw, RLE, repeat and rotate chunks. Ride records,
  language tables, colour presets, passenger offset tables, entrance records and
  the required images are parsed. Other classes are counted as unsupported.
  Malformed supported files are counted as rejected. Readiness requires decoded
  rides, entrances and a valid palette, not merely file presence. Checksums are
  retained as part of identity, but legacy DAT checksums are not verified.
- Park entities retain shared immutable definitions, including the 16-byte DAT
  identity, decoded data and preview. Add object creates a separate instance
  referencing the selected real definition, with a nine-instance limit.
- A three-part entrance from the installed DAT is painted by OpenLoco's map
  renderer, with four viewport orientations. The real ride preview is used as
  a regional proxy at each instance's position. These are not reconstructed rides.
- RCT2 pixels are decoded into bounded bitmaps, matched to the Locomotion palette
  and stored in a separate image range, starting at 0x60000. Locomotion image slots
  and object definitions are unchanged. Transparent pixels remain transparent.
- The native window uses OpenLoco frame, caption and button widgets. Enter park
  switches it to the native object browser. Previous park selects existing parks.
- Placement requires a flat, clear, dry 7x7 area near a town and costs 5,000.
  Shared construction clearance rejects occupied park tiles. Multiplayer/editor
  creation is disabled. Rescan is disabled while parks exist to preserve references.
- New-map and save-load paths reset parks and their sprites. The fake monthly
  revenue tied to launching the external process has been removed.

## Explicit limitations

Parks and instances are session-only, not saved into S5. No TD6 reconstruction,
SC6/SV6 import, ride operation, guests, pathfinding, animation or operating economy
is implemented. Instances are free test objects. Some terrain editing commands
can bypass construction clearance; full terrain reservation is not implemented.
Palette conversion is an approximation. Only the entrance's 12 images and the
ride's first preview are decoded, not every ride animation frame. The browser
uses an English/ASCII fallback. The format reader is independent C++ code, not a
linked or embedded OpenRCT2 engine.

## Validation

`tools/hybrid/tests` builds a standalone Windows x64 reader test. It covers every
chunk encoding, fixed ride/entrance records, sprite decoding, truncation,
out-of-range runs/backreferences and deterministic malformed-file mutations.
An optional argument points it at a locally owned RCT2 installation for a real
asset sweep; these proprietary assets are never uploaded to CI.

The Windows workflow runs these tests, builds OpenLoco, checks the PE architecture
and native markers, rejects launcher strings and external runtime files, then
packages a ZIP and SHA-256. Binary markers alone are not gameplay validation.

Wire-format references examined: OpenRCT2 v0.5.3, commit
f503f57bdb74b31507f83909db587a5db5794ef0, object/EntranceObject.cpp,
object/RideObject.cpp, object/ImageTable.cpp, sawyer_coding/SawyerChunkReader.cpp
and SpriteIds.h. No OpenRCT2 source files are compiled into this fork.
