#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
generate_city_models.py
========================

Procedurally generates low-poly PLACEHOLDER city building models for a
Hearts of Iron IV mod, following the exact same data pipeline vanilla HOI4
(and TGC-Hearts-of-Iron-IV) uses for its map clutter cities:

    map/cities.bmp   (indexed bitmap)  --color_index-->  city_group
    map/cities.txt                      city_group { density, building{ distance -> mesh } }
    gfx/models/buildings/*.mesh         actual PDX binary mesh geometry
    gfx/models/buildings/*.gfx          pdxmesh definitions (texture + shader)
    gfx/models/buildings/*.asset        entity wrappers (what cities.txt points at)

This was reverse-engineered directly from the files shipped in
TGC-Hearts-of-Iron-IV (map/cities.txt + gfx/models/buildings/*.mesh), so the
naming/wiring conventions below mirror vanilla exactly:

  * REGION  = a palette color in map/cities.bmp (`color_index` in cities.txt).
              Vanilla ships four: Western (15), Asian (0), French (1) and
              "uncivilized" (2). This script generates N regions the same
              way -- you paint the matching color onto map/cities.bmp
              wherever you want that regional building style to appear.
  * DENSITY = the `distance` field inside a city_group's `building` blocks.
              distance = 1 is the outer edge of an urban blob (sparsest),
              distance = 4 is deep in the blob's core (densest). Vanilla
              swaps in a visually "bigger" mesh at each successive distance
              tier -- that's the "different models for different levels of
              urban density" mechanic.

Everything this script produces is a fully valid (if crude) asset: real PDX
binary .mesh files, real uncompressed .dds textures, real .gfx/.asset text
files, and a ready-to-merge map/cities.txt fragment. Geometry is
deliberately simple axis-aligned boxes (+ an optional pitched roof / a
smaller "setback" block on top) -- these are meant to be stand-ins you
replace with real hand-modelled low-poly buildings later, not final art.

Ships with 6 example regions (western, east_asian, mediterranean, informal,
south_america, eastern_europe) -- add/rename more in REGIONS below. Every
mesh's diffuse is an obvious "dev texture": plain yellow, a 1px black
outline, and a black text label (region abbreviation + tier + variant, e.g.
"EEU-T4-04") stamped on it via a tiny built-in bitmap font, so it shows up
identically on every face and you can identify any building in-engine at a
glance.

Nothing here touches your live mod. Everything is written under OUTPUT_DIR;
copy the pieces you want into your mod folder yourself (see the generated
README.md and INTEGRATION section below).

Usage:
    python3 generate_city_models.py

No third-party dependencies -- pure standard library.
"""

import math
import os
import random
import struct

# ---------------------------------------------------------------------------
# CONFIG -- edit this section to add/rename regions or retune density tiers
# ---------------------------------------------------------------------------

OUTPUT_DIR = "output"
VARIANTS_PER_TIER = 4          # vanilla always ships 4 variants per tier (01-04)
UNIT_SCALE = 1.0                # 1 game "map unit" per world unit; tweak to taste

# Each region = one map/cities.bmp palette color. `color_index` is only a
# suggestion (matches vanilla's own western/asian/french/unciv indices where
# convenient) -- repaint map/cities.bmp with whatever indices your mod's
# palette actually uses and update these to match.
#
# `abbr` is a short code baked into that region's placeholder diffuse
# textures (see PLACEHOLDER TEXTURES below) so you can tell meshes apart
# in-engine at a glance -- keep it <= 3 chars and stick to the characters
# covered by FONT_5X7 (A-Z, 0-9, '-') or extend the font first.
REGIONS = {
    "western": {
        "color_index": 15,
        "abbr": "WST",
        "roof_style": "pitched",
        "footprint_scale": 1.00,
        "height_scale": 1.00,
    },
    "east_asian": {
        "color_index": 0,
        "abbr": "EAS",
        "roof_style": "pitched",
        "footprint_scale": 0.92,
        "height_scale": 1.10,
    },
    "mediterranean": {
        "color_index": 1,
        "abbr": "MED",
        "roof_style": "flat",
        "footprint_scale": 1.05,
        "height_scale": 0.95,
    },
    "informal": {
        "color_index": 2,
        "abbr": "INF",
        "roof_style": "flat",
        "footprint_scale": 0.85,
        "height_scale": 0.70,
    },
    "south_america": {
        "color_index": 3,
        "abbr": "SAM",
        "roof_style": "flat",        # concrete flat-roof self-built infill is the dominant silhouette
        "footprint_scale": 0.95,
        "height_scale": 0.90,
    },
    "eastern_europe": {
        "color_index": 4,
        "abbr": "EEU",               # ex-Soviet panel-block ("khrushchyovka"/"panelka") style
        "roof_style": "flat",
        "footprint_scale": 1.15,
        "height_scale": 1.20,
    },
}

# Density tiers = cities.txt `distance` values, sorted growing (1 = urban
# edge / lowest density, 4 = urban core / highest density), exactly like
# vanilla's building{ distance = N mesh = {...} } blocks.
DENSITY_TIERS = {
    1: {"label": "outskirts", "footprint": (4.0, 4.0), "height": (3.0, 5.0), "setback": False},
    2: {"label": "suburban", "footprint": (5.0, 5.5), "height": (6.0, 9.0), "setback": False},
    3: {"label": "urban", "footprint": (6.5, 7.0), "height": (10.0, 16.0), "setback": False},
    4: {"label": "downtown", "footprint": (7.0, 8.0), "height": (18.0, 30.0), "setback": True},
}

PITCHED_ROOF_HEIGHT_FRACTION = 0.35  # roof apex height, as a fraction of wall height


# ---------------------------------------------------------------------------
# small vector helpers (plain tuples, no numpy dependency)
# ---------------------------------------------------------------------------

def v_add(a, b):
    return (a[0] + b[0], a[1] + b[1], a[2] + b[2])


def v_sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def v_scale(a, s):
    return (a[0] * s, a[1] * s, a[2] * s)


def v_cross(a, b):
    return (
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0],
    )


def v_len(a):
    return math.sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2])


def v_norm(a):
    l = v_len(a)
    if l < 1e-9:
        return (0.0, 0.0, 0.0)
    return (a[0] / l, a[1] / l, a[2] / l)


def face_normal(p0, p1, p2):
    return v_norm(v_cross(v_sub(p1, p0), v_sub(p2, p1)))


def tangent_for_normal(n):
    up = (0.0, 1.0, 0.0)
    if abs(n[1]) > 0.99:
        t = v_norm((1.0, 0.0, 0.0))
    else:
        t = v_norm(v_cross(up, n))
    return (t[0], t[1], t[2], 1.0)


# ---------------------------------------------------------------------------
# low-poly mesh builder (flat-shaded: every triangle gets its own verts)
# ---------------------------------------------------------------------------

class MeshBuilder:
    def __init__(self):
        self.positions = []   # (x,y,z)
        self.normals = []     # (x,y,z)
        self.uvs = []         # (u,v)
        self.tris = []        # (i0,i1,i2)

    def _add_vertex(self, p, n, uv):
        idx = len(self.positions)
        self.positions.append(p)
        self.normals.append(n)
        self.uvs.append(uv)
        return idx

    def add_quad(self, p0, p1, p2, p3, uv_repeat=(1.0, 1.0)):
        """Adds a planar quad (p0..p3 wound so the surface normal, computed
        via the right-hand rule, points outward) as two triangles."""
        n = face_normal(p0, p1, p2)
        ur, vr = uv_repeat
        i0 = self._add_vertex(p0, n, (0.0, 0.0))
        i1 = self._add_vertex(p1, n, (ur, 0.0))
        i2 = self._add_vertex(p2, n, (ur, vr))
        i3 = self._add_vertex(p3, n, (0.0, vr))
        self.tris.append((i0, i1, i2))
        self.tris.append((i0, i2, i3))

    def add_tri(self, p0, p1, p2):
        n = face_normal(p0, p1, p2)
        i0 = self._add_vertex(p0, n, (0.0, 0.0))
        i1 = self._add_vertex(p1, n, (1.0, 0.0))
        i2 = self._add_vertex(p2, n, (0.5, 1.0))
        self.tris.append((i0, i1, i2))

    def extend(self, other, offset=(0.0, 0.0, 0.0)):
        base = len(self.positions)
        for p in other.positions:
            self.positions.append(v_add(p, offset))
        self.normals.extend(other.normals)
        self.uvs.extend(other.uvs)
        for (a, b, c) in other.tris:
            self.tris.append((a + base, b + base, c + base))

    def aabb(self):
        xs = [p[0] for p in self.positions]
        ys = [p[1] for p in self.positions]
        zs = [p[2] for p in self.positions]
        return (min(xs), min(ys), min(zs)), (max(xs), max(ys), max(zs))


# ---------------------------------------------------------------------------
# building geometry (box, optionally with a pitched roof or a smaller
# "setback" box stacked on top for a skyscraper silhouette)
# ---------------------------------------------------------------------------

def add_box_walls_and_flat_roof(mb, hw, hd, wall_h, y0=0.0, roof=True):
    """4 side walls + (optionally) a flat roof cap. No floor (never seen)."""
    y1 = y0 + wall_h
    # front (-z)
    mb.add_quad((-hw, y0, -hd), (-hw, y1, -hd), (hw, y1, -hd), (hw, y0, -hd))
    # back (+z)
    mb.add_quad((-hw, y0, hd), (hw, y0, hd), (hw, y1, hd), (-hw, y1, hd))
    # left (-x)
    mb.add_quad((-hw, y0, -hd), (-hw, y0, hd), (-hw, y1, hd), (-hw, y1, -hd))
    # right (+x)
    mb.add_quad((hw, y0, -hd), (hw, y1, -hd), (hw, y1, hd), (hw, y0, hd))
    if roof:
        # top (+y)
        mb.add_quad((-hw, y1, -hd), (-hw, y1, hd), (hw, y1, hd), (hw, y1, -hd))
    return y1


def add_pitched_roof(mb, hw, hd, y1, roof_h):
    apex = (0.0, y1 + roof_h, 0.0)
    tl, tr = (-hw, y1, -hd), (hw, y1, -hd)   # front top edge
    bl, br = (-hw, y1, hd), (hw, y1, hd)     # back top edge
    mb.add_tri(tr, tl, apex)   # front slope
    mb.add_tri(bl, br, apex)   # back slope
    mb.add_tri(tl, bl, apex)   # left slope
    mb.add_tri(br, tr, apex)   # right slope


def build_building_mesh(width, depth, height, roof_style, setback, rng):
    """Builds one low-poly building. `rng` is a seeded random.Random used for
    small per-variant jitter so the 4 variants in a tier don't look identical
    (matching vanilla's approach of shipping 4 hand-varied meshes/tier)."""
    hw, hd = width / 2.0, depth / 2.0
    mb = MeshBuilder()

    if roof_style == "pitched":
        wall_h = height * (1.0 - PITCHED_ROOF_HEIGHT_FRACTION)
        y1 = add_box_walls_and_flat_roof(mb, hw, hd, wall_h, roof=False)
        add_pitched_roof(mb, hw, hd, y1, height - wall_h)
    else:
        main_h = height
        if setback:
            # downtown towers: a slimmer block stacked on the main block,
            # like a rooftop massing setback on a skyscraper.
            main_h = height * rng.uniform(0.55, 0.7)
        y1 = add_box_walls_and_flat_roof(mb, hw, hd, main_h, roof=not setback)
        if setback:
            top_w = width * rng.uniform(0.45, 0.65)
            top_d = depth * rng.uniform(0.45, 0.65)
            top_h = height - main_h
            add_box_walls_and_flat_roof(mb, top_w / 2.0, top_d / 2.0, top_h, y0=y1, roof=True)

    return mb


# ---------------------------------------------------------------------------
# PDX binary mesh writer
#
# Reverse-engineered directly from real .mesh files shipped in
# TGC-Hearts-of-Iron-IV/gfx/models/buildings/ (origo_mesh.mesh, TEST_building*.mesh).
# This is the same "Clausewitz asset" binary container format used by CK3 /
# Stellaris / HOI4's Maya & Blender (io_pdx_mesh) exporters:
#
#   file   := "@@b@" prop(pdxasset) node
#   node   := ("[" * depth) name "\0" (prop | node)*
#   prop   := "!" len(1 byte) name(len bytes) type(1 byte) count(int32) value*
#   type 'f' -> float32 * count
#   type 'i' -> int32   * count
#   type 's' -> count * ( int32 strlen, strlen bytes incl. trailing NUL )
# ---------------------------------------------------------------------------

class PDXWriter:
    def __init__(self):
        self.buf = bytearray()

    def raw(self, b):
        self.buf += b

    def node_open(self, name, depth):
        self.buf += b"[" * depth
        self.buf += name.encode("ascii") + b"\x00"

    def _prop_header(self, name, type_char):
        name_b = name.encode("ascii")
        self.buf += b"!" + bytes([len(name_b)]) + name_b + type_char.encode("ascii")

    def prop_floats(self, name, values):
        self._prop_header(name, "f")
        self.buf += struct.pack("<i", len(values))
        for v in values:
            self.buf += struct.pack("<f", float(v))

    def prop_ints(self, name, values):
        self._prop_header(name, "i")
        self.buf += struct.pack("<i", len(values))
        for v in values:
            self.buf += struct.pack("<i", int(v))

    def prop_strings(self, name, values):
        self._prop_header(name, "s")
        self.buf += struct.pack("<i", len(values))
        for s in values:
            sb = s.encode("ascii") + b"\x00"
            self.buf += struct.pack("<i", len(sb))
            self.buf += sb

    def write(self, path):
        with open(path, "wb") as f:
            f.write(self.buf)


def write_mesh_file(path, mb, shape_name, shader, diffuse_tex, normal_tex):
    w = PDXWriter()
    w.raw(b"@@b@")
    w.prop_ints("pdxasset", [1, 0])

    w.node_open("object", 1)
    w.node_open(shape_name, 2)
    w.node_open("mesh", 3)

    flat_p = [c for p in mb.positions for c in p]
    flat_n = [c for n in mb.normals for c in n]
    flat_ta = [c for n in mb.normals for c in tangent_for_normal(n)]
    flat_uv = [c for uv in mb.uvs for c in uv]
    flat_tri = [i for tri in mb.tris for i in tri]

    w.prop_floats("p", flat_p)
    w.prop_floats("n", flat_n)
    w.prop_floats("ta", flat_ta)
    w.prop_floats("u0", flat_uv)
    w.prop_ints("tri", flat_tri)

    (minv, maxv) = mb.aabb()
    w.node_open("aabb", 4)
    w.prop_floats("min", list(minv))
    w.prop_floats("max", list(maxv))

    w.node_open("material", 4)
    w.prop_strings("shader", [shader])
    w.prop_strings("diff", [diffuse_tex])
    w.prop_strings("n", [normal_tex])

    w.write(path)


# ---------------------------------------------------------------------------
# minimal uncompressed DDS writer (32bpp BGRA, no mips) -- placeholder textures
# ---------------------------------------------------------------------------

def _dds_header(width, height):
    flags = 0x1 | 0x2 | 0x4 | 0x8 | 0x1000       # CAPS|HEIGHT|WIDTH|PITCH|PIXELFORMAT
    pitch = width * 4
    pf_flags = 0x1 | 0x40                         # ALPHAPIXELS | RGB
    caps = 0x1000                                 # TEXTURE
    # 4s magic, then 124-byte DDS_HEADER:
    #   7I  = dwSize, dwFlags, dwHeight, dwWidth, dwPitchOrLinearSize, dwDepth, dwMipMapCount
    #   11I = dwReserved1[11]
    #   4I  = pixelformat: dwSize, dwFlags, dwFourCC, dwRGBBitCount
    #   4I  = pixelformat: dwRBitMask, dwGBitMask, dwBBitMask, dwABitMask
    #   5I  = dwCaps, dwCaps2, dwCaps3, dwCaps4, dwReserved2
    header = struct.pack(
        "<4s 7I 11I 4I 4I 5I",
        b"DDS ",
        124, flags, height, width, pitch, 0, 1,
        *([0] * 11),
        32, pf_flags, 0, 32,
        0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000,
        caps, 0, 0, 0, 0,
    )
    return header


def write_flat_dds(path, width, height, bgra):
    header = _dds_header(width, height)
    pixel = bytes(bgra)
    with open(path, "wb") as f:
        f.write(header)
        f.write(pixel * (width * height))


# ---------------------------------------------------------------------------
# tiny built-in 5x7 bitmap font -- just enough characters for the default
# "{REGION_ABBR}-T{tier}-{variant}" labels (A,D,E,F,I,M,N,S,T,U,W, 0-9, '-',
# space). Add more glyphs here (5 rows of 5 bits, MSB-first, '1'=ink) if you
# use different label text.
# ---------------------------------------------------------------------------

FONT_5X7 = {
    "0": ["01110", "10001", "10011", "10101", "11001", "10001", "01110"],
    "1": ["00100", "01100", "00100", "00100", "00100", "00100", "01110"],
    "2": ["01110", "10001", "00001", "00010", "00100", "01000", "11111"],
    "3": ["11111", "00010", "00100", "00010", "00001", "10001", "01110"],
    "4": ["00010", "00110", "01010", "10010", "11111", "00010", "00010"],
    "5": ["11111", "10000", "11110", "00001", "00001", "10001", "01110"],
    "6": ["00110", "01000", "10000", "11110", "10001", "10001", "01110"],
    "7": ["11111", "00001", "00010", "00100", "01000", "01000", "01000"],
    "8": ["01110", "10001", "10001", "01110", "10001", "10001", "01110"],
    "9": ["01110", "10001", "10001", "01111", "00001", "00010", "01100"],
    "A": ["01110", "10001", "10001", "11111", "10001", "10001", "10001"],
    "D": ["11110", "10001", "10001", "10001", "10001", "10001", "11110"],
    "E": ["11111", "10000", "10000", "11110", "10000", "10000", "11111"],
    "F": ["11111", "10000", "10000", "11110", "10000", "10000", "10000"],
    "I": ["01110", "00100", "00100", "00100", "00100", "00100", "01110"],
    "M": ["10001", "11011", "10101", "10101", "10001", "10001", "10001"],
    "N": ["10001", "11001", "10101", "10101", "10011", "10001", "10001"],
    "S": ["01111", "10000", "10000", "01110", "00001", "00001", "11110"],
    "T": ["11111", "00100", "00100", "00100", "00100", "00100", "00100"],
    "U": ["10001", "10001", "10001", "10001", "10001", "10001", "01110"],
    "W": ["10001", "10001", "10001", "10101", "10101", "11011", "10001"],
    "-": ["00000", "00000", "00000", "11111", "00000", "00000", "00000"],
    " ": ["00000", "00000", "00000", "00000", "00000", "00000", "00000"],
}
FONT_W, FONT_H = 5, 7


def _text_pixel_positions(text, scale):
    """Yields (x, y) pixel coords (top-left origin) of every 'on' pixel for
    `text` rendered at integer `scale`, plus the total (width, height)."""
    glyph_w, glyph_h = FONT_W * scale, FONT_H * scale
    gap = scale
    total_w = len(text) * glyph_w + max(0, len(text) - 1) * gap
    coords = []
    cursor_x = 0
    for ch in text:
        glyph = FONT_5X7.get(ch.upper(), FONT_5X7[" "])
        for row, bits in enumerate(glyph):
            for col, bit in enumerate(bits):
                if bit == "1":
                    for sy in range(scale):
                        for sx in range(scale):
                            coords.append((cursor_x + col * scale + sx, row * scale + sy))
        cursor_x += glyph_w + gap
    return coords, total_w, glyph_h


def write_label_dds(path, width, height, text, scale=2):
    """Writes a 'dev texture' style placeholder: plain yellow fill, a 1px
    black outline around the whole texture, and `text` stamped in black,
    centered. Every mesh face UVs to the full 0..1 square (see
    MeshBuilder.add_quad), so this appears identically on every face."""
    YELLOW = (0, 255, 255, 255)   # BGRA
    BLACK = (0, 0, 0, 255)        # BGRA

    pixels = [YELLOW] * (width * height)

    def set_px(x, y, color):
        if 0 <= x < width and 0 <= y < height:
            pixels[y * width + x] = color

    # 1px black outline around the whole texture
    for x in range(width):
        set_px(x, 0, BLACK)
        set_px(x, height - 1, BLACK)
    for y in range(height):
        set_px(0, y, BLACK)
        set_px(width - 1, y, BLACK)

    # centered text label
    coords, text_w, text_h = _text_pixel_positions(text, scale)
    ox = (width - text_w) // 2
    oy = (height - text_h) // 2
    for (x, y) in coords:
        set_px(ox + x, oy + y, BLACK)

    header = _dds_header(width, height)
    with open(path, "wb") as f:
        f.write(header)
        for p in pixels:
            f.write(bytes(p))


# ---------------------------------------------------------------------------
# generation driver
# ---------------------------------------------------------------------------

def region_mesh_name(region, tier, variant):
    return "{}_buildings_{}_{:02d}".format(region, tier, variant)


def generate_region_shared_textures(region_dir, region_name):
    """Normal/specular stay flat, neutral, and shared across every mesh in
    the region -- only the diffuse becomes a unique labeled texture per
    mesh (see generate_region)."""
    write_flat_dds(os.path.join(region_dir, "{}_normal.dds".format(region_name)), 4, 4, (255, 128, 128, 255))  # flat "up" normal (BGRA)
    write_flat_dds(os.path.join(region_dir, "{}_specular.dds".format(region_name)), 4, 4, (20, 20, 20, 255))   # low, uniform shininess


def generate_region(region_name, region_cfg, out_root):
    region_dir = os.path.join(out_root, "gfx", "models", "buildings", region_name)
    os.makedirs(region_dir, exist_ok=True)
    generate_region_shared_textures(region_dir, region_name)

    normal_tex = "{}_normal.dds".format(region_name)

    gfx_lines = ["objectTypes = {"]
    asset_lines = []
    mesh_names_by_tier = {}

    for tier, tier_cfg in sorted(DENSITY_TIERS.items()):
        mesh_names_by_tier[tier] = []
        for variant in range(1, VARIANTS_PER_TIER + 1):
            name = region_mesh_name(region_name, tier, variant)
            rng = random.Random("{}-{}-{}".format(region_name, tier, variant))

            fw, fd = tier_cfg["footprint"]
            fw *= region_cfg["footprint_scale"] * rng.uniform(0.9, 1.1)
            fd *= region_cfg["footprint_scale"] * rng.uniform(0.9, 1.1)
            h_lo, h_hi = tier_cfg["height"]
            height = rng.uniform(h_lo, h_hi) * region_cfg["height_scale"]

            mb = build_building_mesh(
                width=fw * UNIT_SCALE,
                depth=fd * UNIT_SCALE,
                height=height * UNIT_SCALE,
                roof_style=region_cfg["roof_style"],
                setback=tier_cfg["setback"],
                rng=rng,
            )

            # unique per-mesh "dev texture": plain yellow, 1px black outline,
            # black text label -- lets you identify any building in-engine
            # at a glance. Every face UVs to the full 0..1 square (see
            # MeshBuilder.add_quad) so the label shows up on every face.
            label = "{}-T{}-{:02d}".format(region_cfg["abbr"], tier, variant)
            diffuse_tex = "{}_diffuse.dds".format(name)
            write_label_dds(os.path.join(region_dir, diffuse_tex), 128, 128, label)

            mesh_path = os.path.join(region_dir, name + ".mesh")
            shape_name = "{}Shape".format(name)
            write_mesh_file(mesh_path, mb, shape_name, "PdxMeshAdvanced", diffuse_tex, normal_tex)

            rel_mesh_path = "gfx/models/buildings/{}/{}.mesh".format(region_name, name)
            gfx_lines.append("\tpdxmesh = {")
            gfx_lines.append('\t\tname = "{}_mesh"'.format(name))
            gfx_lines.append('\t\tfile = "{}"'.format(rel_mesh_path))
            gfx_lines.append("\t\tmeshsettings = {")
            gfx_lines.append('\t\t\tname = "{}"'.format(shape_name))
            gfx_lines.append("\t\t\tindex = 0")
            gfx_lines.append('\t\t\ttexture_diffuse = "{}"'.format(diffuse_tex))
            gfx_lines.append('\t\t\ttexture_normal = "{}"'.format(normal_tex))
            gfx_lines.append('\t\t\ttexture_specular = "{}_specular.dds"'.format(region_name))
            gfx_lines.append('\t\t\tshader = "PdxMeshAdvanced"')
            gfx_lines.append("\t\t}")
            gfx_lines.append("\t}")

            asset_lines.append('entity = {')
            asset_lines.append('\tname = "{}_entity"'.format(name))
            asset_lines.append('\tpdxmesh = "{}_mesh"'.format(name))
            asset_lines.append('\tscale = 1.0')
            asset_lines.append('}')
            asset_lines.append('')

            mesh_names_by_tier[tier].append(name)

    gfx_lines.append("}")

    with open(os.path.join(region_dir, "{}_buildings.gfx".format(region_name)), "w") as f:
        f.write("\n".join(gfx_lines) + "\n")
    with open(os.path.join(region_dir, "{}_buildings.asset".format(region_name)), "w") as f:
        f.write("\n".join(asset_lines))

    return mesh_names_by_tier


def generate_cities_txt_fragment(all_mesh_names, out_root):
    """Writes a ready-to-merge city_group per region, in the exact shape
    vanilla's own map/cities.txt uses. `distance` is sorted growing (1 =
    urban edge / sparse, 4 = urban core / dense) per vanilla convention;
    each tier's mesh pool is written as multiple candidate meshes so the
    game can pick between the 4 hand-varied variants (mirrors how vanilla
    lists e.g. "asia_city_01_entity".."asia_city_04_entity" as siblings)."""
    lines = []
    lines.append("# Generated by generate_city_models.py -- merge these city_group")
    lines.append("# blocks into your mod's map/cities.txt. Paint map/cities.bmp with")
    lines.append("# the matching color_index wherever you want each region's style to")
    lines.append("# appear (this is how vanilla assigns Western/Asian/French/uncivilized")
    lines.append("# city looks to different parts of the map).")
    lines.append("")
    for region_name, region_cfg in REGIONS.items():
        lines.append("city_group = {")
        lines.append("\tcolor_index = {} # paint this palette index onto map/cities.bmp for {}".format(
            region_cfg["color_index"], region_name))
        lines.append("\tdensity = 0.5 # fraction of urban-blob pixels that spawn a building -- tune per region")
        lines.append("")
        for tier in sorted(all_mesh_names[region_name].keys()):
            names = all_mesh_names[region_name][tier]
            lines.append("\tbuilding = {")
            lines.append("\t\tdistance = {} # tier {} ({})".format(tier, tier, DENSITY_TIERS[tier]["label"]))
            lines.append("\t\tmesh = {")
            for n in names:
                lines.append('\t\t\t"{}_entity"'.format(n))
            lines.append("\t\t}")
            lines.append("\t}")
        lines.append("}")
        lines.append("")

    with open(os.path.join(out_root, "map", "cities_fragment.txt"), "w") as f:
        f.write("\n".join(lines))


def generate_readme(out_root, all_mesh_names):
    lines = []
    lines.append("# Generated placeholder city models")
    lines.append("")
    lines.append("Everything under `gfx/` and `map/` here is generated by")
    lines.append("`generate_city_models.py`. Nothing has been copied into your live")
    lines.append("mod -- do that yourself once you're happy with the results:")
    lines.append("")
    lines.append("1. Copy `gfx/models/buildings/<region>/` into your mod's own")
    lines.append("   `gfx/models/buildings/<region>/` folder.")
    lines.append("2. Merge the `city_group` blocks in `map/cities_fragment.txt` into")
    lines.append("   your mod's `map/cities.txt` (copy the vanilla file into your mod")
    lines.append("   first if you don't have your own copy yet -- HOI4 mods override")
    lines.append("   base files wholesale, they don't merge automatically).")
    lines.append("3. In `map/cities.bmp`, paint the region(s) you want onto the")
    lines.append("   geographic areas where that building style should appear, using")
    lines.append("   the palette `color_index` values from step 2. This bitmap is")
    lines.append("   indexed-color -- edit the palette, don't just pick RGB values.")
    lines.append("4. Reload the map in-game (or restart) to see the new clutter.")
    lines.append("")
    lines.append("## Regions x density tiers generated")
    lines.append("")
    for region_name in REGIONS:
        lines.append("- **{}** (color_index {}): {}".format(
            region_name, REGIONS[region_name]["color_index"],
            ", ".join("tier {} x{}".format(t, len(all_mesh_names[region_name][t])) for t in sorted(all_mesh_names[region_name]))))
    lines.append("")
    lines.append("Density tiers follow vanilla's `distance` field in cities.txt: tier 1")
    lines.append("is the sparse outer edge of an urban blob, tier 4 is the dense core.")
    lines.append("Edit `DENSITY_TIERS` / `REGIONS` at the top of the script to add more")
    lines.append("regions, retune footprints/heights, or change roof styles.")
    lines.append("")
    lines.append("## About the geometry")
    lines.append("")
    lines.append("These are deliberately crude, low-poly, flat-shaded boxes (with an")
    lines.append("optional pitched roof or a stacked \"setback\" block for the tallest")
    lines.append("tier) -- stand-ins to get the region/density pipeline wired up and")
    lines.append("testable in-game before you commission or model real low-poly")
    lines.append("buildings. The `.mesh` files are real, valid PDX binary meshes")
    lines.append("(reverse-engineered from TGC-Hearts-of-Iron-IV's own files), so they")
    lines.append("will load in-engine as-is.")
    lines.append("")
    lines.append("## About the textures")
    lines.append("")
    lines.append("Every mesh gets its own \"dev texture\" diffuse: plain yellow fill, a")
    lines.append("1px black outline around the whole texture, and a black text label")
    lines.append("(`{REGION_ABBR}-T{tier}-{variant}`, e.g. `EEU-T4-04`) stamped in the")
    lines.append("center. Because every face UVs to the same 0..1 square, the label")
    lines.append("shows up identically on every face of the building -- so you can")
    lines.append("identify any mesh in-engine at a glance. Normal/specular stay flat")
    lines.append("and shared per-region. Edit `FONT_5X7` in the script to add more")
    lines.append("characters, or `write_label_dds()` to change the look.")
    with open(os.path.join(out_root, "README.md"), "w") as f:
        f.write("\n".join(lines))


def main():
    out_root = OUTPUT_DIR
    os.makedirs(os.path.join(out_root, "map"), exist_ok=True)

    all_mesh_names = {}
    total_meshes = 0
    for region_name, region_cfg in REGIONS.items():
        mesh_names_by_tier = generate_region(region_name, region_cfg, out_root)
        all_mesh_names[region_name] = mesh_names_by_tier
        total_meshes += sum(len(v) for v in mesh_names_by_tier.values())

    generate_cities_txt_fragment(all_mesh_names, out_root)
    generate_readme(out_root, all_mesh_names)

    print("Generated {} regions x {} tiers x {} variants = {} meshes".format(
        len(REGIONS), len(DENSITY_TIERS), VARIANTS_PER_TIER, total_meshes))
    print("Output written to: {}/".format(os.path.abspath(out_root)))


if __name__ == "__main__":
    main()
