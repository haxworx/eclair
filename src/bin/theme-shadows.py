#!/usr/bin/env python3
"""Bake the fixed player silhouettes at 2x (requires Pillow and rsvg-convert).

The generated PNGs are checked in so building Eclair needs neither tool.
"""
from io import BytesIO
from pathlib import Path
import re
import subprocess

from PIL import Image, ImageChops, ImageFilter

ROOT = Path(__file__).resolve().parents[2] / 'data/themes'

# Blur radius in logical pixels, downward offset, and peak opacity.
PROFILES = {
    'default': (6, 7, 128),
    'gant': (4, 5, 134),
    'nocturne': (6, 6, 155),
    'folio': (7, 5, 112),
    'frequency': (5, 6, 144),
    'voltage': (2, 2, 142),
    'undertow': (7, 7, 148),
}


def alpha(directory, name, width, height):
    return Image.open(BytesIO(subprocess.check_output([
        'rsvg-convert', '-w', str(width * 2), '-h', str(height * 2),
        str(directory / f'{name}.svg')]))).convert('RGBA').getchannel('A')


def build(theme, blur, offset, opacity):
    directory = ROOT / theme / 'images'
    width, body_height = map(int, re.search(
        r'width="(\d+)" height="(\d+)"', (directory / 'body.svg').read_text()
    ).groups())
    window_width, window_height = map(int, re.search(
        r'min: (\d+) (\d+);', (directory.parent / f'{theme}.edc').read_text()
    ).groups())

    # The full surface starts 14 px down and ends 28 px above the window bottom.
    surface = alpha(directory, 'body', width, window_height - 14 - 28)
    mask = Image.new('L', (window_width * 2, window_height * 2))
    origin = (window_width - width, 14 * 2)
    mask.paste(surface, origin)

    # The playback panel sits over the taller surface and can fill its corners.
    front = Image.new('L', mask.size)
    front.paste(alpha(directory, 'body', width, body_height), origin)
    mask = ImageChops.lighter(mask, front)

    cast = Image.new('L', mask.size)
    cast.paste(mask, (0, offset * 2))
    cast = cast.filter(ImageFilter.GaussianBlur(blur * 2))
    cast = cast.point(lambda value: value * opacity // 255)

    # A close, undisplaced shadow follows the SVG's rounded or chamfered edge
    # even where the softer cast shadow extends into the corner.
    contact = mask.filter(ImageFilter.GaussianBlur(2))
    contact = contact.point(lambda value: value * opacity // (3 * 255))

    image = Image.new('RGBA', mask.size)
    image.putalpha(ImageChops.screen(cast, contact))
    image.save(directory / 'shadow_body.png', optimize=True)


if __name__ == '__main__':
    for theme, profile in PROFILES.items():
        build(theme, *profile)
