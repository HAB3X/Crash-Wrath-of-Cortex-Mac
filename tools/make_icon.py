#!/usr/bin/env python3
"""Makes the app's icon from the game's own art: Crash's head as the game's memory card save icon has it (STUFF/CRASH.ICO on
the disc, the PS2's 3D icon format: a model with its texture), drawn large on a rounded square of blue, the way the Crash
Twinsanity Mac port's icon is made. Nothing of the game's goes into the repository: the icon is made in the build folder from
your own disc.

    tools/make_icon.py DISC [--out build/icon] [--yaw DEGREES] [--pitch DEGREES]

Writes icon_1024.png and, on macOS, AppIcon.icns (iconutil).
"""
import argparse
import math
import shutil
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

try:
    import numpy as np
    from PIL import Image, ImageDraw, ImageFilter
except ImportError:
    # No icon then (the app is the same without one): the build goes on
    print("make_icon.py: Pillow and NumPy are needed for the icon (python3 -m pip install pillow numpy); no icon made",
          file=sys.stderr)
    out = Path(sys.argv[sys.argv.index("--out") + 1]) if "--out" in sys.argv else Path("build/icon")
    out.mkdir(parents=True, exist_ok=True)
    (out / "AppIcon.icns").write_bytes(b"")
    sys.exit(0)

ROOT = Path(__file__).resolve().parents[1]
RUNTIME_TOOLS = [ROOT / "ps2-native-runtime/tools", ROOT.parent / "ps2-native-runtime/tools"]


def read_disc_file(disc, path):
    for tools in RUNTIME_TOOLS:
        if (tools / "disc.py").exists():
            sys.path.insert(0, str(tools))
            break
    from disc import Disc  # noqa: E402
    return Disc(disc).read_file(path)


def parse_icon(data):
    """The PS2 icon: its triangles' vertices (x, y, z), texture coordinates and colours, and its 128x128 texture (RGBA)"""
    magic, shapes, texture_type, _, count = struct.unpack_from("<IIIfI", data, 0)
    if magic != 0x10000:
        raise SystemExit("not a PS2 icon")
    at = 20
    positions, uvs, colours = [], [], []
    for _ in range(count):
        x, y, z, _w = struct.unpack_from("<4h", data, at)
        at += 8 * shapes + 8  # every shape's position (the first used), the normal
        u, v = struct.unpack_from("<2h", data, at)
        at += 4
        r, g, b, a = data[at:at + 4]
        at += 4
        positions.append((x / 4096, y / 4096, z / 4096))
        uvs.append((u / 4096, v / 4096))
        colours.append((r, g, b))
    # The animation: its header and each frame's keys
    _, _, _, _, frames = struct.unpack_from("<IIfII", data, at)
    at += 20
    for _ in range(frames):
        # A frame: its shape, its keys' count, then the keys (a time and a value each)
        _, keys = struct.unpack_from("<2I", data, at)
        at += 8 + keys * 8
    if texture_type & 8:
        raise SystemExit("a compressed icon texture: not handled")
    pixels = np.frombuffer(data, dtype="<u2", count=128 * 128, offset=at).reshape(128, 128)
    rgba = np.zeros((128, 128, 4), np.uint8)
    rgba[..., 0] = (pixels & 31) * 255 // 31
    rgba[..., 1] = ((pixels >> 5) & 31) * 255 // 31
    rgba[..., 2] = ((pixels >> 10) & 31) * 255 // 31
    rgba[..., 3] = 255
    return np.array(positions), np.array(uvs), np.array(colours, np.float32) / 128.0, rgba


def render(positions, uvs, colours, texture, size, yaw, pitch):
    """The model drawn: turned, fitted to the picture, its triangles textured and lit by their vertex colours, nearest first"""
    a, b = math.radians(yaw), math.radians(pitch)
    turn = np.array([[math.cos(a), 0, math.sin(a)], [0, 1, 0], [-math.sin(a), 0, math.cos(a)]])
    tilt = np.array([[1, 0, 0], [0, math.cos(b), -math.sin(b)], [0, math.sin(b), math.cos(b)]])
    with np.errstate(all="ignore"):
        p = positions @ (tilt @ turn).T
    # The icon's y points down; the picture's does too. Fitted with a margin
    lo, hi = p[:, :2].min(0), p[:, :2].max(0)
    scale = size * 0.92 / (hi - lo).max()
    centre = (lo + hi) / 2
    xy = (p[:, :2] - centre) * scale + size / 2
    depth = p[:, 2]
    image = np.zeros((size, size, 4), np.float32)
    zbuffer = np.full((size, size), np.inf, np.float32)
    th, tw = texture.shape[:2]
    for t in range(0, len(p) - 2, 3):
        v = xy[t:t + 3]
        x0, y0 = np.floor(v.min(0)).astype(int)
        x1, y1 = np.ceil(v.max(0)).astype(int)
        x0, y0, x1, y1 = max(x0, 0), max(y0, 0), min(x1, size - 1), min(y1, size - 1)
        if x1 < x0 or y1 < y0:
            continue
        gx, gy = np.meshgrid(np.arange(x0, x1 + 1) + 0.5, np.arange(y0, y1 + 1) + 0.5)
        (ax, ay), (bx, by), (cx, cy) = v
        area = (bx - ax) * (cy - ay) - (by - ay) * (cx - ax)
        if abs(area) < 1e-9:
            continue
        w0 = ((bx - gx) * (cy - gy) - (by - gy) * (cx - gx)) / area
        w1 = ((cx - gx) * (ay - gy) - (cy - gy) * (ax - gx)) / area
        w2 = 1 - w0 - w1
        inside = (w0 >= 0) & (w1 >= 0) & (w2 >= 0)
        if not inside.any():
            continue
        z = w0 * depth[t] + w1 * depth[t + 1] + w2 * depth[t + 2]
        region = zbuffer[y0:y1 + 1, x0:x1 + 1]
        closer = inside & (z < region)
        if not closer.any():
            continue
        uv = w0[..., None] * uvs[t] + w1[..., None] * uvs[t + 1] + w2[..., None] * uvs[t + 2]
        tx = np.clip((uv[..., 0] * tw).astype(int), 0, tw - 1)
        ty = np.clip((uv[..., 1] * th).astype(int), 0, th - 1)
        texel = texture[ty, tx, :3].astype(np.float32)
        light = w0[..., None] * colours[t] + w1[..., None] * colours[t + 1] + w2[..., None] * colours[t + 2]
        rgb = np.clip(texel * light, 0, 255)
        out = image[y0:y1 + 1, x0:x1 + 1]
        out[closer, :3] = rgb[closer]
        out[closer, 3] = 255
        region[closer] = z[closer]
    return Image.fromarray(image.astype(np.uint8), "RGBA")


def background(size):
    """A radial gradient of blue on a rounded square, as the Twinsanity port's icon has"""
    centre, edge = (70, 170, 255), (10, 52, 150)
    y, x = np.mgrid[0:size, 0:size]
    d = np.sqrt((x - size / 2) ** 2 + (y - size * 0.42) ** 2) / (size * 0.75)
    d = np.clip(d, 0, 1)[..., None]
    rgb = (np.array(centre) * (1 - d) + np.array(edge) * d).astype(np.uint8)
    square = Image.fromarray(np.dstack([rgb, np.full((size, size), 255, np.uint8)]), "RGBA")
    mask = Image.new("L", (size, size), 0)
    inset = size * 100 // 1024
    ImageDraw.Draw(mask).rounded_rectangle((inset, inset, size - inset - 1, size - inset - 1), radius=size * 185 // 1024,
                                           fill=255)
    square.putalpha(mask)
    return square, inset


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("disc")
    parser.add_argument("--out", default=str(ROOT / "build/icon"))
    parser.add_argument("--yaw", type=float, default=20)
    parser.add_argument("--pitch", type=float, default=0)
    args = parser.parse_args()
    positions, uvs, colours, texture = parse_icon(read_disc_file(args.disc, "STUFF/CRASH.ICO"))
    size = 1024
    # Drawn at twice the size and brought down, for smooth edges
    head = render(positions, uvs, colours, texture, size * 2, args.yaw, args.pitch).resize((size, size), Image.LANCZOS)
    icon, inset = background(size)
    inner = size - 2 * inset
    head = head.resize((inner * 88 // 100, inner * 88 // 100), Image.LANCZOS)
    # A soft shadow under the head
    shadow = Image.new("RGBA", head.size, (0, 0, 0, 0))
    shadow.putalpha(head.getchannel("A").point(lambda v: v * 90 // 255))
    shadow = shadow.filter(ImageFilter.GaussianBlur(size / 60))
    at = ((size - head.width) // 2, (size - head.height) // 2 + size // 60)
    layer = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    layer.paste(shadow, (at[0], at[1] + size // 50), shadow)
    layer.paste(head, at, head)
    mask = icon.getchannel("A")
    icon = Image.alpha_composite(icon, layer)
    icon.putalpha(Image.composite(icon.getchannel("A"), Image.new("L", (size, size), 0), mask))
    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    icon.save(out / "icon_1024.png")
    if shutil.which("iconutil"):
        with tempfile.TemporaryDirectory() as folder:
            iconset = Path(folder) / "AppIcon.iconset"
            iconset.mkdir()
            for n in (16, 32, 64, 128, 256, 512):
                icon.resize((n, n), Image.LANCZOS).save(iconset / f"icon_{n}x{n}.png")
                icon.resize((n * 2, n * 2), Image.LANCZOS).save(iconset / f"icon_{n}x{n}@2x.png")
            subprocess.run(["iconutil", "-c", "icns", str(iconset), "-o", str(out / "AppIcon.icns")], check=True)
    print(f"icon: {out / 'icon_1024.png'}")


if __name__ == "__main__":
    main()
