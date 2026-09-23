"""Guard the native boundary; binary checks are packaging checks, not gameplay tests."""
from pathlib import Path
import struct
import sys

root = Path(__file__).resolve().parents[2]
hybrid = root / 'src/OpenLoco/src/Hybrid'
for path in hybrid.glob('*.h'):
    text = path.read_text()
    for forbidden in ('CreateProcess', 'ShellExecute', 'launchDetailedPark', 'rct2-runtime', 'std::system', 'spawn('):
        assert forbidden not in text, (path, forbidden)
workflow = (root / '.github/workflows/hybrid-v03.yml').read_text()
assert 'apply_v03' not in workflow
assert 'releases/download' not in workflow
assert not (hybrid / 'Rct2Bridge.h').exists()
assert 'Hybrid::paintPark' in (root / 'src/OpenLoco/src/Paint/PaintTile.cpp').read_text()
assert 'Hybrid::Parks::reset()' in (root / 'src/OpenLoco/src/S5/S5.cpp').read_text()
assert 'Hybrid::Parks::contains(pos)' in (root / 'src/OpenLoco/src/Map/TileClearance.cpp').read_text()
assert 'Rct2Graphics::get(id)' in (root / 'src/OpenLoco/src/Graphics/Gfx.cpp').read_text()
if len(sys.argv) > 1:
    binary = Path(sys.argv[1]).read_bytes()
    pe = struct.unpack_from('<I', binary, 0x3c)[0]
    assert binary[pe:pe+4] == b'PE\0\0'
    assert struct.unpack_from('<H', binary, pe+4)[0] == 0x8664, 'Not Windows x64'
    for marker in (b'Native RCT2 assets', b'Add object', b'v0.5.1-alpha'):
        assert marker in binary, marker
    for marker in ('rct2-runtime', 'openrct2.exe', '--rct2-data-path'):
        assert marker.encode() not in binary and marker.encode('utf-16le') not in binary, marker
print('Native boundary and packaging checks: PASS')
