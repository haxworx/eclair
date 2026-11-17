#!/usr/bin/env python3
"""Bake Flat-style shadows around theme silhouettes (requires Pillow and rsvg-convert).

Generated PNGs are checked in; building Eclair needs neither tool. Unlike
Flat's rectangular compositor clips, these shadows continue beneath rounded
corners. Nine-patch borders preserve their softness when panels resize.
"""
from io import BytesIO
from pathlib import Path
import subprocess
import re
from PIL import Image, ImageChops, ImageDraw, ImageFilter

ROOT = Path(__file__).resolve().parents[1] / 'data/themes'


def alpha(directory, name):
    source = directory / (name + '.svg')
    w, h = map(int, re.search(r'width="(\d+)" height="(\d+)"', source.read_text()).groups())
    return Image.open(BytesIO(subprocess.check_output([
        'rsvg-convert', '-w', str(w * 2), '-h', str(h * 2),
        str(source)]))).convert('RGBA').getchannel('A')


def build(root, theme):
    directory = root / theme / 'images'
    silhouette = alpha(directory, 'body')
    # Side grips protrude beyond the body. Bake their actual SVG outlines into
    # the same mask so the outer edge, rather than the body seam, casts a shadow.
    grips = []
    for side in ('left', 'right'):
        parts = (root / theme / 'parts' / f'{side}_side_front.edc').read_text()
        offsets = re.findall(r'offset:\s*(-?\d+)\s+(-?\d+);', parts)
        x, y = map(int, offsets[0])
        right, bottom = map(int, offsets[1])
        grip = alpha(directory, f'{side}_border').resize(
            ((right - x + 1) * 2, (bottom - y + 1) * 2), Image.Resampling.LANCZOS)
        grips.append((x, y, grip))
    # Built-in grips extend six logical pixels left and nine right.
    complete = Image.new('L', (silhouette.width + 30, silhouette.height))
    complete.paste(silhouette, (12, 0))
    for x, y, grip in grips:
        layer = Image.new('L', complete.size)
        layer.paste(grip, (x * 2 + 12, y * 2))
        complete = ImageChops.lighter(complete, layer)
    silhouette = complete
    for name in ('body', 'artwork', 'drawer'):
        if name == 'body':
            mask = silhouette
        elif name == 'drawer':
            background = alpha(directory, 'playlist_background')
            footer = alpha(directory, 'playlist_bottom')
            mask = Image.new('L', (background.width, background.height + footer.height))
            mask.paste(background, (0, 0))
            mask.paste(footer, (0, background.height))
        else:
            # Canonical artwork panel; only the centre stretches at runtime.
            mask = Image.new('L', (400, 280))
            ImageDraw.Draw(mask).rounded_rectangle((0, 0, 399, 279),
                                                  radius=16,
                                                  fill=255)
        shadow = Image.new('L', (mask.width + 84, mask.height + 84))
        # Flat's 21/14/28 margins, 7px downward displacement and soft black mask.
        shadow.paste(mask, (42, 42))
        shadow = shadow.filter(ImageFilter.GaussianBlur(12)).point(lambda a: a // 2)
        image = Image.new('RGBA', shadow.size)
        image.putalpha(shadow)
        image.save(directory / f'shadow_{name}.png', optimize=True)


if __name__ == '__main__':
    for theme in ('default', 'gant', 'nocturne', 'folio', 'frequency', 'voltage', 'undertow'):
        build(ROOT, theme)
