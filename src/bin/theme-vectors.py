#!/usr/bin/env python3
"""Rebuild the built-in themes' SVG artwork. Uses only Python's standard library."""
from pathlib import Path
import re
import math
import runpy

ROOT = Path(__file__).resolve().parents[1] / 'data/themes'
PALETTES = {
    'default': dict(top='#faf8f3', bottom='#dedbd4', edge='#aaa69c', light='#ffffff',
                    ink='#67665f', accent='#b68b38', accent_light='#f6e1ab', well='#f0eee8',
                    shade='#514a3d', radius=12),
    'gant': dict(top='#648aa5', bottom='#416780', edge='#294b64', light='#93b4cb',
                 ink='#d4e5ef', accent='#b8dcef', accent_light='#e4f4ff', well='#345970',
                 shade='#203e53', radius=3),
    'nocturne': dict(top='#202638', bottom='#171c2b', edge='#3a435c', light='#65718e',
                     ink='#d7ddef', accent='#85d9c4', accent_light='#b9eedf', well='#141a29',
                     shade='#1a242a', radius=16),
    'frequency': dict(top='#343b39', bottom='#171e1e', edge='#635b48', light='#969784',
                      ink='#ebcc9b', accent='#dba352', accent_light='#ffe0a2', well='#101919',
                      shade='#291d12', radius=13),
    'voltage': dict(top='#171a30', bottom='#101220', edge='#386669', light='#72e7df',
                    ink='#d0f2ef', accent='#dcff53', accent_light='#efffa9', well='#0b1020',
                    shade='#141322', radius=2),
    'undertow': dict(top='#15343c', bottom='#0b202b', edge='#38616a', light='#91c9bc',
                     ink='#b6e2d8', accent='#71d9b8', accent_light='#b4efd4', well='#081c26',
                     shade='#0c2528', radius=8),
    'folio': dict(top='#f6efdf', bottom='#ebe1ce', edge='#c9b99c', light='#fff9ed',
                  ink='#4f4337', accent='#b56549', accent_light='#d99876', well='#faf4e8',
                  shade='#fff7e8', radius=18),
}


def build(theme, p):
    directory = ROOT / theme / 'images'
    def svg(name, w, h, content, defs=''):
        # EFL's SVG loader expects premultiplied stroke colors. Use opaque,
        # preblended strokes for subtle highlights, avoiding loader warnings.
        def stroke(match):
            tag = match.group(0)
            opacity = re.search(r' stroke-opacity="([.0-9]+)"', tag)
            color = re.search(r' stroke="(#[0-9a-fA-F]{6})"', tag)
            if opacity and color:
                alpha = float(opacity[1])
                fg, bg = color[1], p['bottom']
                blended = '#' + ''.join(f'{round(int(fg[i:i+2],16)*alpha + int(bg[i:i+2],16)*(1-alpha)):02x}' for i in (1,3,5))
                tag = tag.replace(color[0], f' stroke="{blended}"').replace(opacity[0], '')
            return tag
        content = re.sub(r'<[^>]+>', stroke, content)
        (directory / (name + '.svg')).write_text(
            f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" '
            f'viewBox="0 0 {w} {h}" preserveAspectRatio="none">\n'
            f'<defs>{defs}</defs>\n{content}\n</svg>\n')

    def gradient(a, b):
        return (f'<linearGradient id="surface" x1="0" y1="0" x2="0" y2="1">'
                f'<stop stop-color="{a}"/><stop offset="1" stop-color="{b}"/></linearGradient>')

    def panel(name, w, h, radius=None, fill=None):
        r = p['radius'] if radius is None else radius
        svg(name, w, h,
            f'<rect x=".5" y=".5" width="{w-1}" height="{h-1}" rx="{r}" '
            f'fill="{fill or "url(#surface)"}" stroke="{p["edge"]}"/>'
            f'<path d="M{r+1} 1.5H{w-r-1}" fill="none" stroke="{p["light"]}" stroke-opacity=".6"/>',
            gradient(p['top'], p['bottom']))

    panel('body', 306, 120)
    panel('side', 120, 106, 3)
    panel('left_border', 16, 106, 5 if theme == 'default' else 1)
    panel('right_border', 16, 106, 5 if theme == 'default' else 1)
    for name in ['left_border', 'right_border']:
        path = directory / (name + '.svg')
        text = path.read_text().replace('</svg>',
            f'<path d="M7 47v12m3-12v12" stroke="{p["edge"]}" stroke-linecap="round"/></svg>')
        path.write_text(text)
    panel('playlist_background', 260, 140, 3)
    # A straight join to the list and rounded outer bottom corners.
    svg('playlist_bottom', 260, 17,
        f'<path d="M.5 .5H259.5V6.5Q259.5 16.5 249.5 16.5H10.5Q.5 16.5 .5 6.5Z" '
        f'fill="url(#surface)" stroke="{p["edge"]}"/>',
        gradient(p['top'], p['bottom']))
    panel('playlist_box', 241, 123, 4, p['well'])
    panel('display', 168, 28, 8 if theme == 'default' else 2, p['well'])
    # Text lives in Edje, so labels follow desktop scale and never stretch.
    panel('playlist_button', 73, 19, 5 if theme == 'default' else 2)
    svg('cover', 88, 87,
        f'<rect x=".5" y=".5" width="87" height="86" rx="4" fill="{p["well"]}" stroke="{p["edge"]}"/>'
        f'<circle cx="44" cy="43.5" r="26" fill="none" stroke="{p["edge"]}"/>'
        f'<circle cx="44" cy="43.5" r="8" fill="none" stroke="{p["edge"]}"/>'
        f'<circle cx="44" cy="43.5" r="2" fill="{p["ink"]}"/>')
    svg('cover_border', 88, 87, f'<rect x=".5" y=".5" width="87" height="86" rx="3" fill="none" stroke="{p["edge"]}"/>')
    for name in ['left_shadow', 'left_shadow2', 'right_shadow', 'playlist_shadow']:
        vertical = name != 'playlist_shadow'
        w, h = (16, 106) if vertical else (260, 10)
        svg(name, w, h, f'<rect width="{w}" height="{h}" fill="#000000" fill-opacity=".04"/>')
    svg('playlist_box_over', 241, 123, '<path d="M5 1h231" stroke="#ffffff" stroke-opacity=".12"/>')
    panel('playlist_scrollbar_button', 11, 6, 2)
    for name, path in [('playlist_scrollbar_up', 'M3 4 5.5 1.5 8 4'), ('playlist_scrollbar_down', 'M3 2 5.5 4.5 8 2')]:
        svg(name, 11, 6, f'<path d="{path}" fill="none" stroke="{p["ink"]}" stroke-width="1.2" stroke-linecap="round" stroke-linejoin="round"/>')
    svg('track', 120, 4,
        f'<rect x=".5" y=".5" width="119" height="3" rx="1.5" fill="{p["well"]}" stroke="{p["edge"]}"/>')
    for name, a, b in [('thumb', p['top'], p['bottom']), ('thumb_down', p['accent_light'], p['accent'])]:
        shape = (f'<circle cx="7" cy="7" r="5.5" fill="url(#surface)" stroke="{p["edge"]}"/>'
                 f'<circle cx="7" cy="7" r="2" fill="{p["accent"]}"/>' if theme == 'default' else
                 f'<rect x="1.5" y="1.5" width="11" height="11" rx="1" fill="url(#surface)" stroke="{p["edge"]}"/>'
                 f'<path d="M5 5h4v4H5z" fill="{p["accent"]}"/>')
        svg(name, 14, 14, shape, gradient(a, b))
    svg('thumb_glow', 14, 14, f'<circle cx="7" cy="7" r="6" fill="none" stroke="{p["accent"]}" stroke-opacity=".5"/>')
    svg('speaker', 12, 14,
        f'<path d="M1 5h3l3-3v10l-3-3H1zM9 4q4 3 0 6" fill="none" stroke="{p["ink"]}" stroke-width="1.2" stroke-linejoin="round" stroke-linecap="round"/>')
    # The transport keeps each theme's original circular / directional silhouette.
    if theme == 'default':
        transport = (f'<circle cx="48" cy="48" r="46.5" fill="url(#surface)" stroke="{p["edge"]}"/>'
                     f'<circle cx="48" cy="48" r="42" fill="none" stroke="{p["accent"]}" stroke-opacity=".65" stroke-width="2"/>'
                     f'<path d="M17 17 79 79M79 17 17 79" stroke="{p["edge"]}" stroke-opacity=".5"/>'
                     f'<circle cx="48" cy="48" r="19" fill="url(#surface)" stroke="{p["edge"]}"/>')
    else:
        transport = (f'<path d="M29 2H67V29H94V67H67V94H29V67H2V29H29Z" '
                     f'fill="url(#surface)" stroke="{p["edge"]}"/>'
                     f'<path d="M31 4H65V31H92M4 65V31H31V4" fill="none" stroke="{p["light"]}" stroke-opacity=".65"/>'
                     f'<rect x="30.5" y="30.5" width="35" height="35" rx="2" fill="{p["well"]}" stroke="{p["edge"]}"/>')
    svg('transport', 96, 96, transport, gradient(p['top'], p['bottom']))
    icons = {
        'play': 'M9 5 19 12 9 19Z', 'pause': 'M8 5h3v14H8ZM15 5h3v14h-3Z',
        'stop': 'M7 7h10v10H7Z',
        'prev': 'M6 6h2v12H6ZM18 6 9 12 18 18Z',
        'next': 'M16 6h2v12h-2ZM6 6 15 12 6 18Z',
        'open': 'M5 14 12 6 19 14ZM5 17h14v2H5Z',
    }
    for name, path in icons.items():
        for state, ink in [('', p['ink']), ('_over', p['accent']), ('_down', p['shade'])]:
            svg('nav_' + name + state, 24, 24,
                f'<path d="{path}" fill="{ink}" stroke-linejoin="round"/>')
    for name, path in [('button_close', 'M6 6 14 14M14 6 6 14'), ('button_minimize', 'M6 12h8')]:
        svg(name, 20, 20, f'<path d="{path}" fill="none" stroke="{p["ink"]}" stroke-width="1.4" stroke-linecap="round"/>')
    modes = {
        'repeat': 'M5 9V8a3 3 0 0 1 3-3h10m-3-2 3 2-3 2M19 11v1a3 3 0 0 1-3 3H6m3-2-3 2 3 2',
        'shuffle': 'M5 5h2c4 0 6 10 10 10h2m-3-3 3 3-3 3M5 15h2c1.5 0 2.8-1.6 4-4M13 7c1.2-1.4 2.5-2 4-2h2m-3-3 3 3-3 3',
    }
    for mode, path in modes.items():
        for active in (False, True):
            ink = p['shade'] if active else p['ink']
            svg(mode + ('_on' if active else '_off'), 24, 20,
                f'<rect x=".5" y=".5" width="23" height="19" rx="{5 if theme == "default" else 2}" '
                f'fill="url(#surface)" stroke="{p["accent"] if active else p["edge"]}"/>'
                f'<path d="{path}" fill="none" stroke="{ink}" stroke-width="1.25" '
                f'stroke-linecap="round" stroke-linejoin="round"/>',
                gradient(p['accent_light'], p['accent']) if active else gradient(p['top'], p['bottom']))


def nocturne_details():
    """The listening deck has its own silhouette and large mint play key."""
    directory = ROOT / 'nocturne/images'
    def svg(name, w, h, content):
        (directory / (name + '.svg')).write_text(
            f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" '
            f'viewBox="0 0 {w} {h}" preserveAspectRatio="none">\n{content}\n</svg>\n')
    svg('body', 306, 160,
        '<defs><linearGradient id="deck" x2=".8" y2="1"><stop stop-color="#262c42"/>'
        '<stop offset="1" stop-color="#171c2b"/></linearGradient></defs>'
        '<rect x=".5" y=".5" width="305" height="159" rx="16" fill="url(#deck)" stroke="#3a435c"/>'
        '<path d="M20 128H286" stroke="#30384f"/>'
        '<path d="M20 1h110" stroke="#afa0d5" stroke-width="2"/>')
    svg('display', 212, 28, '<path d="M0 27h212" stroke="#31394f"/>')
    svg('transport', 126, 32, '<rect width="126" height="32" rx="10" fill="#141a29"/>')
    svg('play_surface', 48, 48,
        '<defs><linearGradient id="mint" x2="1" y2="1"><stop stop-color="#b9eedf"/>'
        '<stop offset="1" stop-color="#72c8b3"/></linearGradient></defs>'
        '<circle cx="24" cy="24" r="23" fill="url(#mint)" stroke="#b9eedf"/>')
    svg('status', 6, 6, '<circle cx="3" cy="3" r="2.5" fill="#b6a1e2"/>')
    # Dark glyphs on the large mint key; other controls stay pale on ink.
    for icon in ['play', 'pause']:
        for suffix, color in [('', '#1a3436'), ('_over', '#36566a'), ('_down', '#101e25')]:
            path = directory / ('nav_' + icon + suffix + '.svg')
            text = path.read_text()
            text = re.sub(r'fill="#[0-9a-fA-F]{6}"', f'fill="{color}"', text)
            path.write_text(text)
    svg('thumb', 14, 14, '<circle cx="7" cy="7" r="4.5" fill="#a9dfd0"/>')
    svg('thumb_down', 14, 14, '<circle cx="7" cy="7" r="6" fill="#cbf8ec"/>')
    svg('cover', 88, 87,
        '<rect x=".5" y=".5" width="87" height="86" rx="12" fill="#1d2336" stroke="#3a435c"/>'
        '<circle cx="44" cy="43.5" r="30" fill="#292d47"/>'
        '<path d="M20 57Q45 14 69 36M22 64Q49 22 71 44" fill="none" stroke="#b6a1e2" stroke-width="2"/>'
        '<circle cx="44" cy="43.5" r="8" fill="#85d9c4"/>'
        '<circle cx="44" cy="43.5" r="2" fill="#202638"/>')
    svg('cover_border', 88, 87,
        '<rect x=".5" y=".5" width="87" height="86" rx="12" fill="none" stroke="#3a435c"/>')


def folio_details():
    """Paper, lacquer, and brass: the portrait record sleeve."""
    directory = ROOT / 'folio/images'
    def svg(name, w, h, content):
        (directory / (name + '.svg')).write_text(
            f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" '
            f'viewBox="0 0 {w} {h}" preserveAspectRatio="none">\n{content}\n</svg>\n')
    grain = ''.join(f'<path d="M18 {y}h270" stroke="#eee4d2" stroke-width=".35"/>' for y in range(34, 326, 7))
    svg('body', 306, 340,
        '<rect x=".5" y=".5" width="305" height="339" rx="18" fill="#f6efdf" stroke="#c9b99c"/>'
        + grain + '<path d="M24 28h258M24 305h94m73 0h91" stroke="#d5c6aa"/>'
        '<path d="M24 28h37" stroke="#b56549" stroke-width="2"/>')
    grooves = ''.join(f'<circle cx="92" cy="92" r="{r}" fill="none" stroke="{("#363332" if r % 4 else "#44403b")}" stroke-width=".6"/>' for r in range(44, 87, 2))
    svg('platter', 184, 184,
        '<circle cx="92" cy="92" r="91" fill="#272729" stroke="#716455"/>'
        + grooves + '<circle cx="92" cy="92" r="38" fill="#b56549"/>'
        '<path d="M58 107a38 38 0 0 0 69-30" fill="#e2ac88"/>'
        '<path d="M67 71h24m-29 5h16" stroke="#f7e3ca" stroke-width="1.5"/>'
        '<circle cx="92" cy="92" r="8" fill="#f6efdf"/>'
        '<circle cx="92" cy="92" r="2.5" fill="#716455"/>')
    svg('spindle', 16, 16, '<circle cx="8" cy="8" r="8" fill="#f6efdf"/><circle cx="8" cy="8" r="2.5" fill="#716455"/>')
    # Keep a fixed drawing extent so the arm can turn inside its SVG viewport.
    svg('tonearm', 180, 180,
        '<rect width="180" height="180" fill="none"/>'
        '<g transform="translate(40 10)">'
        '<path d="M65 10V49L17 117" fill="none" stroke="#655343" stroke-width="6" stroke-linecap="round" stroke-linejoin="round"/>'
        '<path d="M64 10V49L16 116" fill="none" stroke="#d6ba85" stroke-width="3" stroke-linecap="round"/>'
        '<circle cx="65" cy="10" r="8" fill="#d5bd8d" stroke="#8c7655"/>'
        '<circle cx="65" cy="10" r="3" fill="#6b5b47"/>'
        '<path d="m9 112 13 9-9 12-13-9Z" fill="#6a5041"/>'
        '<path d="m3 124 10 7" stroke="#b56549" stroke-width="2"/></g>')
    svg('play_surface', 48, 48, '<circle cx="24" cy="24" r="23" fill="#b56549" stroke="#92523d"/>')
    for icon in ['play', 'pause']:
        for suffix, color in [('', '#fff5e3'), ('_over', '#ffffff'), ('_down', '#eac6a7')]:
            path = directory / ('nav_' + icon + suffix + '.svg')
            path.write_text(re.sub(r'fill="#[0-9a-fA-F]{6}"', f'fill="{color}"', path.read_text()))
    svg('display', 258, 28, '<path d="M0 27h258" fill="none" stroke="#d5c6aa"/>')
    svg('transport', 266, 30, '<path d="M0 29h266" stroke="#e1d3bb"/>')
    svg('thumb', 14, 14, '<circle cx="7" cy="7" r="4" fill="#b56549"/><circle cx="7" cy="7" r="1.3" fill="#f6efdf"/>')
    svg('thumb_down', 14, 14, '<circle cx="7" cy="7" r="5.5" fill="#8f4935"/>')
    svg('cover', 88, 87,
        '<rect x=".5" y=".5" width="87" height="86" fill="#e4c9a9" stroke="#c9b99c"/>'
        '<path d="M0 70Q30 19 88 34V87H0Z" fill="#ba7356"/>'
        '<circle cx="59" cy="27" r="15" fill="#f9ebce"/>'
        '<path d="M0 76Q43 48 88 55V87H0Z" fill="#7b7f68"/>')
    svg('side', 120, 326,
        '<rect x=".5" y=".5" width="119" height="325" fill="#eee3ce" stroke="#c9b99c"/>'
        '<path d="M12 14h96M12 310h96M7 3v320" stroke="#d8c8a9"/>'
        '<path d="M16 238h88m-88 5h44m-44 5h58" stroke="#c9b99c"/>')
    for name in ['left_border', 'right_border']:
        svg(name, 16, 326,
            '<rect x=".5" y=".5" width="15" height="325" rx="4" fill="#e8dbc1" stroke="#c9b99c"/>'
            '<path d="M6 151v24m4-24v24" stroke="#af9470"/>')


def frequency_details():
    """Smoked glass, a luminous station scale, and a mechanical tuning needle."""
    directory = ROOT / 'frequency/images'
    def svg(name, w, h, content):
        (directory / (name + '.svg')).write_text(
            f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" '
            f'viewBox="0 0 {w} {h}" preserveAspectRatio="none">{content}</svg>\n')
    ticks = ''.join(f'<path d="M{x} 12v{22 if i%5==0 else 12 if i%2==0 else 7}" stroke="{("#e8bc78" if i%5==0 else "#8e7958")}"/>' for i,x in enumerate(range(8,329,8)))
    svg('body', 386, 222,
        '<defs><linearGradient id="case" x2="0" y2="1"><stop stop-color="#414946"/><stop offset=".12" stop-color="#2c3533"/><stop offset="1" stop-color="#151d1d"/></linearGradient></defs>'
        '<rect x=".5" y=".5" width="385" height="221" rx="13" fill="url(#case)" stroke="#756b53"/>'
        '<rect x="4" y="4" width="378" height="214" rx="10" fill="none" stroke="#46504a"/>'
        '<path d="M16 27h354M24 161h338" stroke="#555442"/>'
        '<path d="M24 30h99" stroke="#bd9254"/>'
        '<path d="M20 205h346" stroke="#38413c"/>'
        '<circle cx="14" cy="14" r="2" fill="#9f9474"/><circle cx="372" cy="208" r="2" fill="#9f9474"/>')
    (directory / 'volume_track.svg').write_text((directory / 'track.svg').read_text())
    svg('track', 336, 72,
        '<defs><linearGradient id="glass" x2=".4" y2="1"><stop stop-color="#070f12"/><stop offset=".65" stop-color="#1b2926"/><stop offset="1" stop-color="#0c1618"/></linearGradient></defs>'
        '<rect x=".5" y=".5" width="335" height="71" rx="5" fill="url(#glass)" stroke="#84704c"/>'
        '<path d="M4 4h328v21L4 48Z" fill="#b2beab" fill-opacity=".07"/>'
        + ticks + '<path d="M8 46h320M8 61h320" stroke="#65593f"/>'
        '<path d="M8 54h10m30 0h10m30 0h10m30 0h10m30 0h10m30 0h10m30 0h10m30 0h10m30 0h10" stroke="#ad8e5c"/>')
    for name,color in [('needle','#ffd08b'),('needle_down','#fff1cf'),('needle_glow','#e7ac59')]:
        svg(name,14,64,
            f'<path d="M7 2v59" stroke="{color}" stroke-width="1.5"/>'
            f'<path d="M3 0h8L7 6ZM3 64h8l-4-6Z" fill="{color}"/>'
            f'<path d="M5 8v48m4-48v48" fill="none" stroke="{color}" opacity=".12"/>')
    svg('display',267,28,'<path d="M0 27h267" stroke="#3b4540"/>')
    svg('transport',168,36,'<rect x=".5" y=".5" width="167" height="35" rx="4" fill="#192220" stroke="#665d47"/><path d="M34 4v28m68-28v28m34-28v28" stroke="#3e453b"/>')
    svg('play_surface',36,36,'<rect x=".5" y=".5" width="35" height="35" rx="3" fill="#d5a261" stroke="#ffe0a2"/>')
    for icon in ['play','pause']:
        for suffix,color in [('', '#291d12'),('_over','#000000'),('_down','#634528')]:
            p=directory/('nav_'+icon+suffix+'.svg')
            p.write_text(re.sub(r'fill="#[0-9a-fA-F]{6}"',f'fill="{color}"',p.read_text()))
    for name,color in [('thumb','#c9a56c'),('thumb_down','#ffe0a2')]:
        svg(name,14,14,f'<circle cx="7" cy="7" r="5.5" fill="#26332e" stroke="{color}"/><path d="M7 3v4" stroke="{color}" stroke-width="1.5"/>')
    svg('status',6,6,'<circle cx="3" cy="3" r="2.5" fill="#ffc77d"/>')
    svg('side',120,208,'<rect x=".5" y=".5" width="119" height="207" rx="3" fill="#202b28" stroke="#635b48"/><path d="M10 15h100M10 193h100" stroke="#736242"/>')
    for name in ['left_border','right_border']:
        svg(name,16,208,'<rect x=".5" y=".5" width="15" height="207" rx="2" fill="#2a3430" stroke="#635b48"/><path d="M6 94v18m4-18v18" stroke="#c29b60"/>')
    svg('cover',88,87,'<rect x=".5" y=".5" width="87" height="86" fill="#141e1d" stroke="#635b48"/><circle cx="44" cy="43" r="27" fill="none" stroke="#b28a52"/><circle cx="44" cy="43" r="20" fill="none" stroke="#635b48"/><path d="M44 21v44M22 43h44M29 28l30 30m0-30L29 58" stroke="#635b48"/><circle cx="44" cy="43" r="4" fill="#dba352"/>')


def voltage_details():
    """A clipped neon chassis with circuit traces and a luminous signal rail."""
    directory = ROOT / 'voltage/images'
    def svg(name, w, h, content):
        (directory / (name + '.svg')).write_text(
            f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" '
            f'viewBox="0 0 {w} {h}" preserveAspectRatio="none">{content}</svg>\n')
    grid = ''.join(f'<path d="M{x} 119 {173+(x-173)*.4:.1f} 65" stroke="#213b48"/>' for x in range(24,324,20))
    grid += ''.join(f'<path d="M24 {y}h298" stroke="#213b48"/>' for y in [70,78,89,103,119])
    svg('body',346,208,
        '<path d="M16 .5H329.5L345.5 16v175l-16 16.5H.5V16Z" fill="#101321" stroke="#4d9798"/>'
        '<path d="M17 5h310l13 13v170l-14 14H6V18Z" fill="#171a2d" stroke="#293b4b"/>'
        '<path d="M20 30h306v90H20Z" fill="#0a1020"/>' + grid +
        '<path d="M24 28h84l8-6h69M324 64v39l-8 9M24 122h136l7-7h66" fill="none" stroke="#5bd9d1"/>'
        '<path d="M7 39v50M328 160v25l-11 11h-39" fill="none" stroke="#f455a5" stroke-width="2"/>'
        '<path d="M23 171h300M23 202h75" stroke="#385968"/>'
        '<path d="m300 118 6-6m1 6 6-6m1 6 6-6M26 6h28" stroke="#dcff53"/>'
        '<rect x="289" y="7" width="11" height="2" fill="#f455a5"/>')
    svg('display',298,32,'<path d="M0 31h298" stroke="#324554"/>')
    svg('transport',169,40,'<path d="M6 .5H168.5V33l-6 6.5H.5V7Z" fill="#111727" stroke="#416872"/><path d="M1 7h30m105 32h27" stroke="#f455a5"/>')
    svg('play_surface',40,40,'<path d="M7 1h32v31l-7 7H1V7Z" fill="#dcff53" stroke="#f3ffb3"/><path d="M5 10v23h21" fill="none" stroke="#819a36"/>')
    svg('track',120,4,'<rect x="0" y="1" width="120" height="2" fill="#397d7a"/><path d="M0 0h120" stroke="#21474f"/>')
    for name,color in [('thumb','#dcff53'),('thumb_down','#ff70b8')]:
        svg(name,14,14,f'<path d="m7 1 5 6-5 6-5-6Z" fill="{color}"/><path d="M7 4v6" stroke="#141322"/>')
    svg('status',298,4,'<path d="M0 2h214m8 0h30m8 0h16m8 0h14" stroke="#f455a5" stroke-width="2"/>')
    for name in ['play','pause']:
        for suffix,color in [('', '#141322'),('_over','#141322'),('_down','#583454')]:
            p=directory/('nav_'+name+suffix+'.svg')
            p.write_text(re.sub(r'fill="#[0-9a-fA-F]{6}"',f'fill="{color}"',p.read_text()))
    svg('side',120,194,'<path d="M6 .5H119.5v193H.5V7Z" fill="#111a2b" stroke="#386669"/><path d="M10 15h100M10 179h100M5 40v40" stroke="#5ac5c2"/>')
    for name in ['left_border','right_border']:
        svg(name,16,194,'<path d="M4 .5H15.5v193H.5V5Z" fill="#152330" stroke="#386669"/><path d="m5 80 5 5-5 5m0 8 5 5-5 5" fill="none" stroke="#dcff53"/>')
    svg('cover',88,87,'<rect x=".5" y=".5" width="87" height="86" fill="#0b1020" stroke="#386669"/><path d="M7 21V7h14m46 0h14v14m0 45v14H67m-46 0H7V66" fill="none" stroke="#5bd9d1"/><path d="m44 19 23 25-23 24-23-24Z" fill="#1d3946" stroke="#dcff53"/><path d="M33 44h22m-11-11v22" stroke="#f455a5"/>')




def undertow_details():
    """A pressure-hull listening instrument with bathymetry and sonar echoes."""
    directory = ROOT / 'undertow/images'
    def svg(name, w, h, content):
        (directory / (name + '.svg')).write_text(
            f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" '
            f'viewBox="0 0 {w} {h}" preserveAspectRatio="none">{content}</svg>\n')
    ticks = ''
    for i in range(72):
        angle = math.radians(i*5)
        r1 = 94 if i%6 == 0 else 98
        ticks += (f'<path d="M{173+r1*math.sin(angle):.2f} {132-r1*math.cos(angle):.2f}'
                  f'L{173+102*math.sin(angle):.2f} {132-102*math.cos(angle):.2f}" '
                  f'stroke="{("#77b9ac" if i%6==0 else "#315662")}"/>')
    contours = ''
    for n in range(11):
        points = []
        for x in range(61,286,4):
            y = 124+n*8 + 14*math.sin((x-60)/36+n*.19) + 7*math.sin(x/17+n*.45)
            if math.hypot(x-173, y-132) < 86:
                points.append(f'{x},{y:.2f}')
        contours += f'<polyline points="{" ".join(points)}" fill="none" stroke="{("#2c686c" if n%3==0 else "#1c4855")}" stroke-width=".7"/>'
    svg('body',346,322,
        '<defs><linearGradient id="depth" x2=".5" y2="1"><stop stop-color="#193d45"/><stop offset=".5" stop-color="#102c38"/><stop offset="1" stop-color="#091d2a"/></linearGradient>'
        '<radialGradient id="scope"><stop stop-color="#133f43"/><stop offset="1" stop-color="#061822"/></radialGradient>'
        '</defs>'
        '<path d="M30 .5H316Q345.5 .5 345.5 31V306Q345.5 321.5 330 321.5H16Q.5 321.5 .5 306V31Q.5 .5 30 .5Z" fill="url(#depth)" stroke="#50727a"/>'
        '<path d="M30 5H316Q341 5 341 32V305Q341 317 329 317H17Q5 317 5 305V32Q5 5 30 5Z" fill="none" stroke="#254953"/>'
        '<path d="M24 28h73m152 0h73M24 273h298M24 292h298" stroke="#365660"/>'
        '<circle cx="173" cy="132" r="109" fill="#0c2530" stroke="#446b71"/>'
        '<circle cx="173" cy="132" r="105" fill="none" stroke="#172f3c" stroke-width="3"/>'
        '<circle cx="173" cy="132" r="90" fill="url(#scope)" stroke="#527d7c"/>'
        + ticks + contours +
        '<path d="M83 132H263M173 42V222M109 68l128 128M109 196 237 68" stroke="#20434f" stroke-width=".65"/>'
        '<circle cx="173" cy="132" r="30" fill="none" stroke="#335d64"/>'
        '<circle cx="173" cy="132" r="60" fill="none" stroke="#335d64"/>'
        '<circle cx="211" cy="83" r="2" fill="#83dcc2"/><circle cx="143" cy="175" r="1.5" fill="#83dcc2"/>'
        '<circle cx="221" cy="167" r="1" fill="#83dcc2"/>'
        '<path d="M173 21v8m0 206v8M62 132h8m206 0h8" stroke="#90c6b7"/>'
        '<circle cx="79" cy="132" r="17" fill="#102f3a" stroke="#38616a"/>'
        '<circle cx="267" cy="132" r="17" fill="#102f3a" stroke="#38616a"/>'
        '<circle cx="128" cy="219" r="16" fill="#102f3a" stroke="#38616a"/>'
        '<circle cx="222" cy="219" r="16" fill="#102f3a" stroke="#38616a"/>'
        '<path d="M17 62v139m312-139v139" stroke="#244753"/>'
        '<path d="M16 63h3m-3 15h3m-3 15h3m-3 15h3m-3 15h3m-3 15h3m-3 15h3m-3 15h3m-3 15h3m-3 15h3" stroke="#557e80"/>'
        '<circle cx="16" cy="307" r="2.5" fill="#71908b"/><circle cx="330" cy="307" r="2.5" fill="#71908b"/>')
    svg('echo',216,216,
        '<circle cx="108" cy="108" r="105" fill="none" stroke="#8fe6c6" stroke-width="1.2"/>'
        '<circle cx="108" cy="108" r="100" fill="none" stroke="#5d9c99" stroke-width=".5"/>'
        '<path d="M108 0v7m0 202v7M0 108h7m202 0h7" stroke="#a4e7ca"/>')
    svg('display',240,28,'<path d="M0 27h240" stroke="#244752"/>')
    svg('transport',236,120,'<rect width="236" height="120" fill="none"/>')
    svg('play_surface',56,56,
        '<defs><radialGradient id="glass"><stop stop-color="#366f6b"/><stop offset="1" stop-color="#194249"/></radialGradient></defs>'
        '<circle cx="28" cy="28" r="27" fill="url(#glass)" stroke="#99d9c1"/>'
        '<circle cx="28" cy="28" r="23" fill="none" stroke="#4b8580"/>'
        '<path d="M11 18a20 20 0 0 1 32-5" fill="none" stroke="#86b6a8"/>')
    svg('track',120,4,'<rect x=".5" y=".5" width="119" height="3" rx="1.5" fill="#071924" stroke="#365b65"/>')
    for name,color in [('thumb','#8de0c1'),('thumb_down','#e2f9d9')]:
        svg(name,14,14,f'<circle cx="7" cy="7" r="4.5" fill="#173e46" stroke="{color}"/><circle cx="7" cy="7" r="1.5" fill="{color}"/>')
    for icon in ['play','pause']:
        for suffix,color in [('', '#d2f3dd'),('_over','#ffffff'),('_down','#91c9bc')]:
            p=directory/('nav_'+icon+suffix+'.svg')
            p.write_text(re.sub(r'fill="#[0-9a-fA-F]{6}"',f'fill="{color}"',p.read_text()))
    svg('side',120,308,'<path d="M12 .5H119.5v307H12Q.5 307.5 .5 295V13Q.5 .5 12 .5Z" fill="#0d2935" stroke="#38616a"/><path d="M11 24h97m-97 260h97M7 40v225" stroke="#244b58"/>')
    for name in ['left_border','right_border']:
        svg(name,16,308,'<rect x=".5" y=".5" width="15" height="307" rx="7" fill="#173b45" stroke="#38616a"/><path d="M6 140v24m4-24v24" stroke="#83b7aa"/>')
    svg('cover',88,87,
        '<defs><radialGradient id="sea"><stop stop-color="#24575b"/><stop offset="1" stop-color="#081e2b"/></radialGradient></defs>'
        '<rect x=".5" y=".5" width="87" height="86" rx="5" fill="url(#sea)" stroke="#38616a"/>'
        '<path d="M23 39c0-25 42-25 42 0Z" fill="#559f98" stroke="#a1debf"/>'
        '<path d="M29 40q-10 15 0 29m9-29q9 15-2 34m12-34q-7 12 3 28m7-28q10 16 0 31" fill="none" stroke="#83ccb6"/>'
        '<path d="M28 34q13-19 31 0" fill="none" stroke="#b0dec8"/>'
        '<circle cx="17" cy="18" r="1" fill="#a1debf"/><circle cx="72" cy="58" r="1" fill="#a1debf"/>')


if __name__ == '__main__':
    for theme, palette in PALETTES.items():
        build(theme, palette)
        if theme == 'nocturne':
            nocturne_details()
        elif theme == 'folio':
            folio_details()
        elif theme == 'frequency':
            frequency_details()
        elif theme == 'voltage':
            voltage_details()
        elif theme == 'undertow':
            undertow_details()
        runpy.run_path(str(Path(__file__).with_name('theme-dialogs.py')))['build'](ROOT, theme, palette)
        directory = ROOT / theme / 'images'
        resources = ''.join(f'   vector: "{p.name}";\n' for p in sorted(directory.glob('*.svg')))
        (directory / 'images.edc').write_text(
            '/* Native vector resources remain scalable inside the compiled theme. */\n'
            'images {\n' + ''.join(f'   image: \"{p.name}\" COMP;\n' for p in sorted(directory.glob('shadow_*.png'))) + resources + '}\n')
        # edje_cc's depfile omits native vector resources in some EFL versions.
        dependencies = ''.join(f"  '{p.name}',\n" for p in sorted(directory.glob('*.svg')))
        (directory / 'meson.build').write_text(
            '# Generated by tools/theme-vectors.py.\n'
            'theme_vectors = files(\n' + dependencies + ''.join(f"  '{p.name}',\n" for p in sorted(directory.glob('shadow_*.png'))) + ')\n')
