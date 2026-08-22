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
naming/wiring conventions below mirror vanilla exactly, with one addition:
vanilla only really varies "region" (a palette color) and "distance"
(sparse edge vs. dense core of an urban blob). This script splits things
one level further, by actual BUILDING TYPE first:

  * ARCHETYPE = the actual *kind* of district being generated -- suburb,
              urban_core, metropolis, commie_block, informal. This is what
              decides which BUILDING_TYPES get used and how densely/tall,
              AND how they're physically arranged (see LAYOUTS below). This
              is the "separate buildings based on actual building type"
              part -- a metropolis is a district of skyscrapers organized
              in a tidy block/grid; a suburb is sprawling houses at loose,
              varied angles; a commie_block district is near-identical
              uniform slabs in strict rows; informal is chaotic and packed.
  * REGION    = a regional/material flavor layered on top of an archetype
              (western/east_asian/mediterranean/south_america/eastern_europe)
              -- this is the "regional varieties" part. It nudges footprint/
              height scale and picks the roof style small residential
              buildings use, so e.g. a western suburb and an east asian
              suburb are both unmistakably suburbs, just styled differently.
  * LOCALE    = one curated (archetype, region, color_index) combination --
              see LOCALES below. Each locale is its own city_group / palette
              color in map/cities.bmp, exactly like a vanilla "region" was,
              except now keyed on archetype+region together.
  * DISTANCE  = the `distance` field inside a city_group's `building` blocks,
              unchanged from vanilla: distance = 1 is the outer edge of an
              urban blob (sparsest), distance = 4 is deep in the blob's core
              (densest). Here it's a mild size-growth applied on top of a
              locale's own archetype (a suburb never suddenly becomes a
              skyscraper district just because distance is higher).

Everything this script produces is a fully valid (if crude) asset: real PDX
binary .mesh files, real uncompressed .dds textures, real .gfx/.asset text
files, and a ready-to-merge map/cities.txt fragment. Each .mesh is actually
a small CLUSTER of 10-20 individual low-poly buildings (mixed types drawn
from its archetype's own type_weights) merged into one file -- matching how
vanilla's own city meshes are city-block chunks, not single buildings. Each
building is an axis-aligned box (+ an optional pitched roof / a smaller
"setback" block on top for towers) -- these are meant to be stand-ins you
replace with real hand-modelled low-poly buildings later, not final art.

How a cluster's buildings are actually *spawned* also now varies by
archetype (see LAYOUTS): a "grid" layout places buildings on a tidy,
lightly-jittered lattice (metropolis/urban_core/commie_block -- a block of
towers or slabs really is organized that way); a "scatter" layout places
them with wide positional jitter AND a random yaw rotation per building
(suburb/informal -- sprawling, irregularly angled lots).

Ships with 5 example regions (western, east_asian, mediterranean,
south_america, eastern_europe) and 5 example archetypes (suburb, urban_core,
metropolis, commie_block, informal), combined into 14 example locales -- add/
rename/recombine in REGIONS / ARCHETYPES / LOCALES below. Every mesh's
diffuse is an obvious "dev texture" atlas: a dedicated colored, outlined,
labeled cell for each of the 6 building types (house, rowhouse, shop, shed,
block, tower) plus one per roof shape (pitched, flat), stamped via a tiny
built-in bitmap font, so every wall face and roof face UVs into the cell
matching what it actually is -- a house's walls look different from a
tower's, a pitched roof looks different from a flat one -- plus a top strip
carrying the mesh's own identifying code (e.g. "MET-EAS-T4-04") so the file
itself is still recognizable in-engine at a glance.

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
VARIANTS_PER_TIER = 4            # vanilla always ships 4 variants per distance (01-04)
UNIT_SCALE = 1.0                 # 1 game "map unit" per world unit; tweak to taste
BUILDINGS_PER_MESH = (10, 20)    # each .mesh is a small city block cluster, not a single building
DISTANCE_LEVELS = (1, 2, 3, 4)   # cities.txt `distance`: 1 = edge of this locale's urban blob, 4 = its core

# Regional/material flavor -- layered ON TOP of an archetype (see
# ARCHETYPES below), not a city_group on its own. `footprint_scale` /
# `height_scale` multiply an archetype's own base size; `roof_style` is
# what this region's low-rise residential types (house/rowhouse) use
# (commercial/tall types stay flat-roofed everywhere -- see
# BUILDING_TYPES). `abbr` is baked into a mesh's identifying code strip on
# its diffuse -- keep it <= 3 chars and stick to characters covered by
# FONT_5X7 (A-Z, 0-9, '-') or extend the font first.
REGIONS = {
    "western":       {"abbr": "WST", "roof_style": "pitched", "footprint_scale": 1.00, "height_scale": 1.00},
    "east_asian":    {"abbr": "EAS", "roof_style": "pitched", "footprint_scale": 0.92, "height_scale": 1.10},
    "mediterranean": {"abbr": "MED", "roof_style": "flat",    "footprint_scale": 1.05, "height_scale": 0.95},
    "south_america": {"abbr": "SAM", "roof_style": "flat",    "footprint_scale": 0.95, "height_scale": 0.90},
    "eastern_europe": {"abbr": "EEU", "roof_style": "flat",   "footprint_scale": 1.15, "height_scale": 1.20},
}

# Building "types" mixed into a cluster. w_mult/d_mult/h_mult scale an
# archetype's base footprint/height (below) per building; "roof" is the
# type's usual roof unless the type is low-rise ("house"/"rowhouse"), in
# which case the *region's* roof_style wins instead (small residential
# roofs are the part that actually varies by regional architecture --
# commercial/tall types are flat-roofed everywhere). "setback" stacks a
# slimmer block on top, for a skyscraper silhouette.
BUILDING_TYPES = {
    "house":    {"w_mult": 0.90, "d_mult": 0.90, "h_mult": 0.80, "roof": "pitched"},
    "rowhouse": {"w_mult": 0.60, "d_mult": 1.00, "h_mult": 1.10, "roof": "pitched"},
    "shop":     {"w_mult": 1.30, "d_mult": 1.00, "h_mult": 0.55, "roof": "flat"},
    "shed":     {"w_mult": 1.60, "d_mult": 0.80, "h_mult": 0.45, "roof": "flat"},
    "block":    {"w_mult": 1.10, "d_mult": 1.10, "h_mult": 1.15, "roof": "flat"},
    "tower":    {"w_mult": 0.85, "d_mult": 0.85, "h_mult": 1.60, "roof": "flat", "setback": True},
}

# ---------------------------------------------------------------------------
# ARCHETYPES -- the "actual building type" split: instead of one generic
# 1-4 density gradient reused identically everywhere, each archetype is a
# distinct kind of district (suburb, dense downtown metropolis, Soviet-style
# uniform housing blocks, informal settlement, mixed-use urban core), each
# with its own:
#   - type_weights   which BUILDING_TYPES it draws from, and how often
#   - footprint/height   its own base size range (before region/distance scale)
#   - layout + pos_jitter/rot_jitter_deg/spread_mult   how its buildings are
#     actually arranged -- a metropolis of towers is organized in a tidy
#     block/grid (low jitter, no rotation); a suburb sprawls at irregular
#     angles (loose "scatter" layout, wide spread, real rotation); a
#     commie_block district is almost perfectly uniform repeated rows;
#     an informal settlement is chaotic and tightly packed.
# ---------------------------------------------------------------------------
ARCHETYPES = {
    "suburb": {
        "abbr": "SUB",
        "footprint": (5.0, 5.5), "height": (5.0, 8.0),
        "type_weights": {"house": 0.55, "rowhouse": 0.25, "shop": 0.20},
        "layout": "scatter", "pos_jitter": 0.55, "rot_jitter_deg": 30, "spread_mult": 1.7,
    },
    "urban_core": {
        "abbr": "URB",
        "footprint": (6.5, 7.0), "height": (9.0, 14.0),
        "type_weights": {"block": 0.35, "shop": 0.30, "rowhouse": 0.20, "tower": 0.15},
        "layout": "grid", "pos_jitter": 0.18, "rot_jitter_deg": 10, "spread_mult": 1.0,
    },
    "metropolis": {
        "abbr": "MET",
        "footprint": (7.5, 8.5), "height": (22.0, 36.0),
        "type_weights": {"tower": 0.55, "block": 0.35, "shop": 0.10},
        "layout": "grid", "pos_jitter": 0.05, "rot_jitter_deg": 0, "spread_mult": 1.0,
    },
    "commie_block": {
        "abbr": "CBL",
        "footprint": (7.0, 10.0), "height": (14.0, 20.0),
        "type_weights": {"block": 1.0},   # near-identical repeated slabs -- deliberately monotonous
        "layout": "grid", "pos_jitter": 0.02, "rot_jitter_deg": 0, "spread_mult": 1.0,
    },
    "informal": {
        "abbr": "INF",
        "footprint": (3.0, 3.5), "height": (2.0, 4.0),
        "type_weights": {"shed": 0.50, "house": 0.35, "shop": 0.15},
        "layout": "scatter", "pos_jitter": 0.60, "rot_jitter_deg": 45, "spread_mult": 1.05,
    },
}

# Curated (archetype, region, color_index) combinations to actually
# generate -- this is the "regional varieties" layer on top of archetype:
# a western metropolis and an east asian metropolis are both still
# unmistakably a metropolis (grid of towers), just styled per-region. Each
# combination becomes its own map/cities.bmp color_index / city_group --
# add, remove, or re-pair tuples here (with an unused color_index) to
# change what's generated; it doesn't have to be every archetype x every
# region.
LOCALES = [
    ("suburb", "western", 10),
    ("suburb", "east_asian", 11),
    ("suburb", "south_america", 12),
    ("suburb", "eastern_europe", 13),
    ("urban_core", "western", 14),
    ("urban_core", "mediterranean", 15),
    ("urban_core", "east_asian", 16),
    ("metropolis", "western", 17),
    ("metropolis", "east_asian", 18),
    ("metropolis", "mediterranean", 19),
    ("commie_block", "eastern_europe", 20),
    ("commie_block", "east_asian", 21),
    ("informal", "south_america", 22),
    ("informal", "east_asian", 23),
]

# Mild "edge of blob -> core of blob" growth across cities.txt's 4
# `distance` levels, applied on top of an archetype's own base size --
# keeps the same archetype/character at every distance (a suburb never
# suddenly becomes a skyscraper district), matching what vanilla's
# per-distance mesh swap is actually for more closely than a generic
# tier-swap would.
DISTANCE_SCALE = {1: 0.90, 2: 0.97, 3: 1.05, 4: 1.13}

PITCHED_ROOF_HEIGHT_FRACTION = 0.35  # roof apex height, as a fraction of wall height

# ---------------------------------------------------------------------------
# Texture atlas layout -- shared by geometry (UV mapping) and the
# placeholder texture generator. One diffuse per mesh is still generated
# (so each mesh's file stays individually identifiable), but its CONTENT is
# now a grid with a dedicated wall sub-region PER BUILDING TYPE plus a
# dedicated roof sub-region per roof shape, instead of one generic
# wall/roof split. This is "different segments for each building type
# where it's a good idea": walls are what's actually visible and where a
# house/shed/tower/etc. should read as a different material, so every type
# gets its own wall segment; roofs only have two distinct *shapes*
# (pitched vs flat), so those share just two segments rather than one per
# type (a flat commercial roof doesn't need its own look per building).
# ---------------------------------------------------------------------------

ATLAS_WIDTH = 384
ATLAS_HEIGHT = 288
ATLAS_CODE_STRIP_H = 32          # thin top strip carrying the mesh's own identifying code
ATLAS_GRID_H = ATLAS_HEIGHT - ATLAS_CODE_STRIP_H
ATLAS_MARGIN_PX = 4               # inset per cell so each keeps its own 1px outline

CODE_LABEL_SCALE = 3   # the code strip is one wide cell -- biggest, most legible text
WALL_LABEL_SCALE = 1   # wall cells are narrow (6 across a row) -- keep text small to fit
ROOF_LABEL_SCALE = 2   # roof cells are wide (2 across a row, each 3 columns) -- room to spare

WALL_TYPE_ORDER = ["house", "rowhouse", "shop", "shed", "block", "tower"]   # one column each
ROOF_STYLE_ORDER = ["pitched", "flat"]                                      # each spans 3 columns

# BGRA fill colors -- one per wall type, one per roof style. Distinct hues
# so each type/shape is recognizable at a glance, not just from its label.
WALL_FILL_BY_TYPE = {
    "house": (0, 255, 255, 255),      # yellow
    "rowhouse": (0, 200, 255, 255),   # amber
    "shop": (255, 255, 0, 255),       # cyan
    "shed": (255, 0, 255, 255),       # magenta
    "block": (0, 190, 0, 255),        # green
    "tower": (200, 80, 150, 255),     # purple
}
ROOF_FILL_BY_STYLE = {
    "pitched": (0, 140, 255, 255),    # orange
    "flat": (90, 110, 130, 255),      # slate
}


def _build_atlas_layout():
    """Computes the pixel-space, then UV-space (0..1), rect for every wall
    type and every roof style in the grid described above. Single source
    of truth for both the geometry (which UV rect a face maps into) and
    write_atlas_dds (which pixels to fill/outline/label) -- so they can
    never drift out of sync."""
    cols = len(WALL_TYPE_ORDER)
    col_w = ATLAS_WIDTH / cols
    row_h = ATLAS_GRID_H / 2
    row0_y0, row0_y1 = ATLAS_CODE_STRIP_H, ATLAS_CODE_STRIP_H + row_h
    row1_y0, row1_y1 = row0_y1, row0_y1 + row_h
    m = ATLAS_MARGIN_PX

    wall_px, roof_px = {}, {}
    for i, btype in enumerate(WALL_TYPE_ORDER):
        x0, x1 = i * col_w, (i + 1) * col_w
        wall_px[btype] = (x0 + m, row0_y0 + m, x1 - m, row0_y1 - m)

    span = cols // len(ROOF_STYLE_ORDER)  # each roof style spans this many columns
    for i, style in enumerate(ROOF_STYLE_ORDER):
        x0, x1 = i * span * col_w, (i + 1) * span * col_w
        roof_px[style] = (x0 + m, row1_y0 + m, x1 - m, row1_y1 - m)

    def to_uv(rect):
        x0, y0, x1, y1 = rect
        return (x0 / ATLAS_WIDTH, y0 / ATLAS_HEIGHT, x1 / ATLAS_WIDTH, y1 / ATLAS_HEIGHT)

    wall_uv = {k: to_uv(v) for k, v in wall_px.items()}
    roof_uv = {k: to_uv(v) for k, v in roof_px.items()}
    return wall_uv, roof_uv, wall_px, roof_px


WALL_UV_BY_TYPE, ROOF_UV_BY_STYLE, _WALL_PX_BY_TYPE, _ROOF_PX_BY_STYLE = _build_atlas_layout()


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

    def add_quad(self, p0, p1, p2, p3, uv_rect=(0.0, 0.0, 1.0, 1.0)):
        """Adds a planar quad (p0..p3 wound so the surface normal, computed
        via the right-hand rule, points outward) as two triangles. `uv_rect`
        = (u0, v0, u1, v1) maps the quad's own unit square into that
        sub-region of the shared texture (see WALL_UV_BY_TYPE /
        ROOF_UV_BY_STYLE) -- this is what puts each building type's walls,
        and each roof style, on their own part of the diffuse atlas."""
        n = face_normal(p0, p1, p2)
        u0, v0, u1, v1 = uv_rect
        i0 = self._add_vertex(p0, n, (u0, v0))
        i1 = self._add_vertex(p1, n, (u1, v0))
        i2 = self._add_vertex(p2, n, (u1, v1))
        i3 = self._add_vertex(p3, n, (u0, v1))
        self.tris.append((i0, i1, i2))
        self.tris.append((i0, i2, i3))

    def add_tri(self, p0, p1, p2, uv_rect=(0.0, 0.0, 1.0, 1.0)):
        n = face_normal(p0, p1, p2)
        u0, v0, u1, v1 = uv_rect
        i0 = self._add_vertex(p0, n, (u0, v0))
        i1 = self._add_vertex(p1, n, (u1, v0))
        i2 = self._add_vertex(p2, n, ((u0 + u1) / 2.0, v1))
        self.tris.append((i0, i1, i2))

    def extend(self, other, offset=(0.0, 0.0, 0.0), yaw=0.0):
        """Merges `other` into self, translated by `offset` and (if
        non-zero) first rotated `yaw` radians around the Y (up) axis --
        this is what lets scatter-layout buildings (see layout_scatter)
        sit at varied angles instead of always facing the same way.
        Positions AND normals both get rotated (normals untranslated, since
        rotation doesn't need re-normalizing -- it's already unit length)."""
        base = len(self.positions)
        if abs(yaw) < 1e-9:
            for p in other.positions:
                self.positions.append(v_add(p, offset))
            self.normals.extend(other.normals)
        else:
            cos_y, sin_y = math.cos(yaw), math.sin(yaw)

            def rot_y(v):
                x, y, z = v
                return (x * cos_y + z * sin_y, y, -x * sin_y + z * cos_y)

            for p in other.positions:
                self.positions.append(v_add(rot_y(p), offset))
            for n in other.normals:
                self.normals.append(rot_y(n))
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

def add_box_walls_and_flat_roof(mb, hw, hd, wall_h, wall_uv, roof_uv, y0=0.0, roof=True):
    """4 side walls + (optionally) a flat roof cap. No floor (never seen).
    `wall_uv`/`roof_uv` are the atlas sub-rects this particular building's
    type/roof-style map into (see WALL_UV_BY_TYPE / ROOF_UV_BY_STYLE)."""
    y1 = y0 + wall_h
    # front (-z)
    mb.add_quad((-hw, y0, -hd), (-hw, y1, -hd), (hw, y1, -hd), (hw, y0, -hd), uv_rect=wall_uv)
    # back (+z)
    mb.add_quad((-hw, y0, hd), (hw, y0, hd), (hw, y1, hd), (-hw, y1, hd), uv_rect=wall_uv)
    # left (-x)
    mb.add_quad((-hw, y0, -hd), (-hw, y0, hd), (-hw, y1, hd), (-hw, y1, -hd), uv_rect=wall_uv)
    # right (+x)
    mb.add_quad((hw, y0, -hd), (hw, y1, -hd), (hw, y1, hd), (hw, y0, hd), uv_rect=wall_uv)
    if roof:
        # top (+y)
        mb.add_quad((-hw, y1, -hd), (-hw, y1, hd), (hw, y1, hd), (hw, y1, -hd), uv_rect=roof_uv)
    return y1


def add_pitched_roof(mb, hw, hd, y1, roof_h, roof_uv):
    """Pitched roof slopes UV into `roof_uv` (the same sub-region a flat
    roof cap of this roof_style would use -- both are "the roof" as far as
    the texture atlas is concerned)."""
    apex = (0.0, y1 + roof_h, 0.0)
    tl, tr = (-hw, y1, -hd), (hw, y1, -hd)   # front top edge
    bl, br = (-hw, y1, hd), (hw, y1, hd)     # back top edge
    mb.add_tri(tr, tl, apex, uv_rect=roof_uv)   # front slope
    mb.add_tri(bl, br, apex, uv_rect=roof_uv)   # back slope
    mb.add_tri(tl, bl, apex, uv_rect=roof_uv)   # left slope
    mb.add_tri(br, tr, apex, uv_rect=roof_uv)   # right slope


def build_building_mesh(width, depth, height, roof_style, setback, building_type, rng):
    """Builds one low-poly building. `building_type` selects this
    building's own wall sub-region in the shared atlas (WALL_UV_BY_TYPE);
    `roof_style` selects its roof sub-region (ROOF_UV_BY_STYLE). `rng` is a
    seeded random.Random used for small per-variant jitter so the 4
    variants in a tier don't look identical (matching vanilla's approach of
    shipping 4 hand-varied meshes/tier)."""
    hw, hd = width / 2.0, depth / 2.0
    mb = MeshBuilder()
    wall_uv = WALL_UV_BY_TYPE[building_type]
    roof_uv = ROOF_UV_BY_STYLE[roof_style]

    if roof_style == "pitched":
        wall_h = height * (1.0 - PITCHED_ROOF_HEIGHT_FRACTION)
        y1 = add_box_walls_and_flat_roof(mb, hw, hd, wall_h, wall_uv, roof_uv, roof=False)
        add_pitched_roof(mb, hw, hd, y1, height - wall_h, roof_uv)
    else:
        main_h = height
        if setback:
            # downtown towers: a slimmer block stacked on the main block,
            # like a rooftop massing setback on a skyscraper.
            main_h = height * rng.uniform(0.55, 0.7)
        # always cap the main box, even when a smaller setback block sits on
        # top of it -- that block doesn't cover the main box's full
        # footprint, so without its own roof/ledge here the main box would
        # be open-topped around the setback (the bug: "tower blocks don't
        # have roofs on layer 1", layer 1 being this main/lower block).
        y1 = add_box_walls_and_flat_roof(mb, hw, hd, main_h, wall_uv, roof_uv, roof=True)
        if setback:
            top_w = width * rng.uniform(0.45, 0.65)
            top_d = depth * rng.uniform(0.45, 0.65)
            top_h = height - main_h
            add_box_walls_and_flat_roof(mb, top_w / 2.0, top_d / 2.0, top_h, wall_uv, roof_uv, y0=y1, roof=True)

    return mb


def layout_grid(count, cell_w, cell_d, rng, pos_jitter, rot_jitter_deg):
    """Tidy, mostly-regular lattice: a metropolis block of towers, an
    urban_core street grid, or a commie_block district's repeated rows all
    read as *organized*, so positions/rotations only get a small fraction of
    a cell's own jitter (pos_jitter) and a small yaw jitter (rot_jitter_deg),
    both scaled down further here vs. layout_scatter's use of the same
    knobs -- a "grid" archetype should still look like a grid even with its
    own pos_jitter/rot_jitter_deg dialed up a little for variety."""
    cols = rng.randint(3, 5)
    rows = math.ceil(count / cols)
    placements = []
    for row in range(rows):
        for col in range(cols):
            if len(placements) >= count:
                break
            bx = (col - (cols - 1) / 2.0) * cell_w + rng.uniform(-pos_jitter, pos_jitter) * cell_w * 0.5
            bz = (row - (rows - 1) / 2.0) * cell_d + rng.uniform(-pos_jitter, pos_jitter) * cell_d * 0.5
            yaw_deg = rng.uniform(-rot_jitter_deg, rot_jitter_deg)
            placements.append((bx, bz, math.radians(yaw_deg)))
    return placements


def layout_scatter(count, cell_w, cell_d, rng, pos_jitter, rot_jitter_deg, spread_mult):
    """Loose, irregular sprawl: a suburb's houses on curving/angled lots, or
    an informal settlement's chaotic packing. Still starts from the same
    grid of cells as layout_grid (so buildings still don't just pile on top
    of each other), but `spread_mult` widens the cells themselves and both
    pos_jitter and rot_jitter_deg are used at full strength (typically much
    higher values than a "grid" archetype passes in), so buildings sit at
    visibly varied positions AND angles instead of facing a uniform way."""
    cols = rng.randint(3, 5)
    rows = math.ceil(count / cols)
    cell_w *= spread_mult
    cell_d *= spread_mult
    placements = []
    for row in range(rows):
        for col in range(cols):
            if len(placements) >= count:
                break
            bx = (col - (cols - 1) / 2.0) * cell_w + rng.uniform(-pos_jitter, pos_jitter) * cell_w
            bz = (row - (rows - 1) / 2.0) * cell_d + rng.uniform(-pos_jitter, pos_jitter) * cell_d
            yaw_deg = rng.uniform(-rot_jitter_deg, rot_jitter_deg)
            placements.append((bx, bz, math.radians(yaw_deg)))
    return placements


LAYOUTS = {"grid": layout_grid, "scatter": layout_scatter}


def build_city_cluster(archetype_cfg, region_cfg, distance, rng):
    """Builds ONE .mesh's worth of content: a small city-block cluster of
    BUILDINGS_PER_MESH (10-20) individual buildings of mixed types, laid out
    and merged into a single combined mesh (this mirrors vanilla's own city
    meshes, which are likewise multi-building clusters, not single buildings
    -- one clutter placement = one little block, not one house).

    Building types are picked from the LOCALE's archetype (`type_weights`),
    not from region or distance -- that's the actual-building-type split:
    a metropolis leans on tower/block regardless of which region it's
    styled as, a suburb leans on house/rowhouse/shop regardless of distance.
    Region only nudges footprint/height scale and (for small residential
    types) roof style; distance only applies a mild overall size bump
    (DISTANCE_SCALE) on top of the archetype's own base size.

    How buildings are actually arranged also comes from the archetype: its
    `layout` ("grid" or "scatter", see LAYOUTS) plus `pos_jitter` /
    `rot_jitter_deg` / `spread_mult` decide whether this cluster reads as an
    organized block or a sprawling, irregularly angled district."""
    count = rng.randint(*BUILDINGS_PER_MESH)
    weights = archetype_cfg["type_weights"]
    type_names = list(weights.keys())
    type_probs = list(weights.values())

    base_fw, base_fd = archetype_cfg["footprint"]
    h_lo, h_hi = archetype_cfg["height"]
    dist_scale = DISTANCE_SCALE[distance]

    # cell size wide enough that even the biggest type in this archetype
    # won't overlap its neighbours before the layout's own jitter/spread.
    cell_w = base_fw * region_cfg["footprint_scale"] * dist_scale * 2.3
    cell_d = base_fd * region_cfg["footprint_scale"] * dist_scale * 2.3

    layout_fn = LAYOUTS[archetype_cfg["layout"]]
    placements = layout_fn(
        count, cell_w, cell_d, rng,
        pos_jitter=archetype_cfg["pos_jitter"],
        rot_jitter_deg=archetype_cfg["rot_jitter_deg"],
        **({"spread_mult": archetype_cfg["spread_mult"]} if archetype_cfg["layout"] == "scatter" else {})
    )

    cluster = MeshBuilder()
    for (bx, bz, yaw) in placements:
        btype = rng.choices(type_names, weights=type_probs, k=1)[0]
        type_cfg = BUILDING_TYPES[btype]

        fw = base_fw * type_cfg["w_mult"] * region_cfg["footprint_scale"] * dist_scale * rng.uniform(0.85, 1.15)
        fd = base_fd * type_cfg["d_mult"] * region_cfg["footprint_scale"] * dist_scale * rng.uniform(0.85, 1.15)
        height = rng.uniform(h_lo, h_hi) * type_cfg["h_mult"] * region_cfg["height_scale"] * dist_scale

        # small residential types take on the region's own roof style
        # (that's the regional-architecture cue); everything else - shops,
        # blocks, towers - is flat-roofed regardless of region.
        if btype in ("house", "rowhouse"):
            roof_style = region_cfg["roof_style"]
        else:
            roof_style = "flat"
        setback = type_cfg.get("setback", False)

        building = build_building_mesh(
            width=fw * UNIT_SCALE, depth=fd * UNIT_SCALE, height=height * UNIT_SCALE,
            roof_style=roof_style, setback=setback, building_type=btype, rng=rng,
        )
        cluster.extend(building, offset=(bx * UNIT_SCALE, 0.0, bz * UNIT_SCALE), yaw=yaw)

    return cluster, len(placements)


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
# "{REGION_ABBR}-T{tier}-{variant}" code strip and the building-type /
# roof-style segment labels (BLOCK, HOUSE, ROWHOUSE, SHOP, SHED, TOWER,
# PITCHED, FLAT). Add more glyphs here (5 rows of 5 bits, MSB-first,
# '1'=ink) if you use different label text.
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
    "B": ["11110", "10001", "11110", "10001", "10001", "10001", "11110"],
    "C": ["01111", "10000", "10000", "10000", "10000", "10000", "01111"],
    "D": ["11110", "10001", "10001", "10001", "10001", "10001", "11110"],
    "E": ["11111", "10000", "10000", "11110", "10000", "10000", "11111"],
    "F": ["11111", "10000", "10000", "11110", "10000", "10000", "10000"],
    "H": ["10001", "10001", "10001", "11111", "10001", "10001", "10001"],
    "I": ["01110", "00100", "00100", "00100", "00100", "00100", "01110"],
    "K": ["10001", "10010", "10100", "11000", "10100", "10010", "10001"],
    "L": ["10000", "10000", "10000", "10000", "10000", "10000", "11111"],
    "M": ["10001", "11011", "10101", "10101", "10001", "10001", "10001"],
    "N": ["10001", "11001", "10101", "10101", "10011", "10001", "10001"],
    "O": ["01110", "10001", "10001", "10001", "10001", "10001", "01110"],
    "P": ["11110", "10001", "10001", "11110", "10000", "10000", "10000"],
    "R": ["11110", "10001", "10001", "11110", "10100", "10010", "10001"],
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


def write_atlas_dds(path, mesh_code):
    """Writes this mesh's 'dev texture' placeholder diffuse: a thin top
    strip with the mesh's own identifying code (so the file is still
    recognizable in an asset browser even though its content below is now
    shared/generic), then a grid with one labeled, outlined, colored
    sub-region per building type (WALL_UV_BY_TYPE) and per roof style
    (ROOF_UV_BY_STYLE) -- see _build_atlas_layout(). Every building's walls
    and roof UV into exactly the matching cell here, so each type/shape
    reads as a visibly different material in-engine. The gutter
    between/around cells is a neutral dark gray so each cell's own 1px
    black outline stays visible."""
    width, height = ATLAS_WIDTH, ATLAS_HEIGHT
    GUTTER = (40, 40, 40, 255)      # BGRA dark gray
    CODE_BG = (200, 200, 200, 255)  # BGRA light gray
    BLACK = (0, 0, 0, 255)

    pixels = [GUTTER] * (width * height)

    def set_px(x, y, color):
        if 0 <= x < width and 0 <= y < height:
            pixels[y * width + x] = color

    def fill_rect(x0, y0, x1, y1, color):
        for y in range(int(y0), int(y1)):
            for x in range(int(x0), int(x1)):
                set_px(x, y, color)

    def outline_rect(x0, y0, x1, y1, color):
        x0, y0, x1, y1 = int(x0), int(y0), int(x1), int(y1)
        for x in range(x0, x1):
            set_px(x, y0, color)
            set_px(x, y1 - 1, color)
        for y in range(y0, y1):
            set_px(x0, y, color)
            set_px(x1 - 1, y, color)

    def draw_label(text, x0, y0, x1, y1, s):
        coords, text_w, text_h = _text_pixel_positions(text, s)
        ox = int(x0) + max(0, (int(x1 - x0) - text_w) // 2)
        oy = int(y0) + max(0, (int(y1 - y0) - text_h) // 2)
        for (x, y) in coords:
            set_px(ox + x, oy + y, BLACK)

    def draw_cell(x0, y0, x1, y1, fill, label, s):
        fill_rect(x0, y0, x1, y1, fill)
        outline_rect(x0, y0, x1, y1, BLACK)
        draw_label(label, x0, y0, x1, y1, s)

    # 1px black outline around the whole texture
    outline_rect(0, 0, width, height, BLACK)

    # thin top strip: this mesh's own identifying code (roomiest cell, so
    # it gets the biggest, most legible text)
    draw_cell(ATLAS_MARGIN_PX, ATLAS_MARGIN_PX, width - ATLAS_MARGIN_PX, ATLAS_CODE_STRIP_H - ATLAS_MARGIN_PX,
              CODE_BG, mesh_code, CODE_LABEL_SCALE)

    # one cell per building type (walls) and per roof style (roofs) --
    # wall cells are narrower (6 across) so their labels stay at scale 1;
    # roof cells are 3x wider (2 across) so theirs can go bigger.
    for btype in WALL_TYPE_ORDER:
        x0, y0, x1, y1 = _WALL_PX_BY_TYPE[btype]
        draw_cell(x0, y0, x1, y1, WALL_FILL_BY_TYPE[btype], btype.upper(), WALL_LABEL_SCALE)
    for style in ROOF_STYLE_ORDER:
        x0, y0, x1, y1 = _ROOF_PX_BY_STYLE[style]
        draw_cell(x0, y0, x1, y1, ROOF_FILL_BY_STYLE[style], style.upper(), ROOF_LABEL_SCALE)

    header = _dds_header(width, height)
    with open(path, "wb") as f:
        f.write(header)
        for p in pixels:
            f.write(bytes(p))


# ---------------------------------------------------------------------------
# generation driver
# ---------------------------------------------------------------------------

def locale_mesh_name(locale_name, distance, variant):
    return "{}_buildings_{}_{:02d}".format(locale_name, distance, variant)


def generate_locale_shared_textures(locale_dir, locale_name):
    """Normal/specular stay flat, neutral, and shared across every mesh in
    the locale -- only the diffuse becomes a unique labeled texture per
    mesh (see generate_locale)."""
    write_flat_dds(os.path.join(locale_dir, "{}_normal.dds".format(locale_name)), 4, 4, (255, 128, 128, 255))  # flat "up" normal (BGRA)
    write_flat_dds(os.path.join(locale_dir, "{}_specular.dds".format(locale_name)), 4, 4, (20, 20, 20, 255))   # low, uniform shininess


def generate_locale(archetype_key, archetype_cfg, region_key, region_cfg, color_index, out_root):
    """Generates every mesh/texture/gfx/asset for one (archetype, region)
    LOCALE -- e.g. "metropolis_western" -- across all 4 DISTANCE_LEVELS x
    VARIANTS_PER_TIER variants, exactly like a vanilla "region" folder used
    to, just keyed on archetype+region together instead of region alone."""
    locale_name = "{}_{}".format(archetype_key, region_key)
    locale_dir = os.path.join(out_root, "gfx", "models", "buildings", locale_name)
    os.makedirs(locale_dir, exist_ok=True)
    generate_locale_shared_textures(locale_dir, locale_name)

    normal_tex = "{}_normal.dds".format(locale_name)

    gfx_lines = ["objectTypes = {"]
    asset_lines = []
    mesh_names_by_distance = {}
    total_buildings = 0

    for distance in DISTANCE_LEVELS:
        mesh_names_by_distance[distance] = []
        for variant in range(1, VARIANTS_PER_TIER + 1):
            name = locale_mesh_name(locale_name, distance, variant)
            rng = random.Random("{}-{}-{}".format(locale_name, distance, variant))

            # each .mesh is a small city-block cluster of 10-20 mixed
            # building types drawn from this LOCALE's archetype, arranged by
            # its archetype's own layout (see build_city_cluster).
            mb, building_count = build_city_cluster(archetype_cfg, region_cfg, distance, rng)
            total_buildings += building_count

            # unique per-mesh "dev texture" atlas: one labeled, outlined,
            # colored cell per building type + per roof style, plus a top
            # strip carrying this mesh's own identifying code -- lets you
            # tell every building type and roof shape apart, and identify
            # any mesh, in-engine at a glance (see write_atlas_dds).
            label = "{}-{}-T{}-{:02d}".format(archetype_cfg["abbr"], region_cfg["abbr"], distance, variant)
            diffuse_tex = "{}_diffuse.dds".format(name)
            write_atlas_dds(os.path.join(locale_dir, diffuse_tex), label)

            mesh_path = os.path.join(locale_dir, name + ".mesh")
            shape_name = "{}Shape".format(name)
            write_mesh_file(mesh_path, mb, shape_name, "PdxMeshAdvanced", diffuse_tex, normal_tex)

            rel_mesh_path = "gfx/models/buildings/{}/{}.mesh".format(locale_name, name)
            gfx_lines.append("\tpdxmesh = {")
            gfx_lines.append('\t\tname = "{}_mesh"'.format(name))
            gfx_lines.append('\t\tfile = "{}"'.format(rel_mesh_path))
            gfx_lines.append("\t\tmeshsettings = {")
            gfx_lines.append('\t\t\tname = "{}"'.format(shape_name))
            gfx_lines.append("\t\t\tindex = 0")
            gfx_lines.append('\t\t\ttexture_diffuse = "{}"'.format(diffuse_tex))
            gfx_lines.append('\t\t\ttexture_normal = "{}"'.format(normal_tex))
            gfx_lines.append('\t\t\ttexture_specular = "{}_specular.dds"'.format(locale_name))
            gfx_lines.append('\t\t\tshader = "PdxMeshAdvanced"')
            gfx_lines.append("\t\t}")
            gfx_lines.append("\t}")

            asset_lines.append('entity = {')
            asset_lines.append('\tname = "{}_entity"'.format(name))
            asset_lines.append('\tpdxmesh = "{}_mesh"'.format(name))
            asset_lines.append('\tscale = 1.0')
            asset_lines.append('}')
            asset_lines.append('')

            mesh_names_by_distance[distance].append(name)

    gfx_lines.append("}")

    with open(os.path.join(locale_dir, "{}_buildings.gfx".format(locale_name)), "w") as f:
        f.write("\n".join(gfx_lines) + "\n")
    with open(os.path.join(locale_dir, "{}_buildings.asset".format(locale_name)), "w") as f:
        f.write("\n".join(asset_lines))

    return locale_name, mesh_names_by_distance, total_buildings


def generate_cities_txt_fragment(locale_results, out_root):
    """Writes a ready-to-merge city_group per LOCALE, in the exact shape
    vanilla's own map/cities.txt uses. `distance` is sorted growing (1 =
    urban edge / sparse, 4 = urban core / dense) per vanilla convention;
    each distance's mesh pool is written as multiple candidate meshes so the
    game can pick between the 4 hand-varied variants (mirrors how vanilla
    lists e.g. "asia_city_01_entity".."asia_city_04_entity" as siblings).
    `locale_results` is a list of (archetype_key, region_key, color_index,
    locale_name, mesh_names_by_distance) tuples, one per LOCALES entry."""
    lines = []
    lines.append("# Generated by generate_city_models.py -- merge these city_group")
    lines.append("# blocks into your mod's map/cities.txt. Paint map/cities.bmp with")
    lines.append("# the matching color_index wherever you want each locale's style to")
    lines.append("# appear (this is how vanilla assigns Western/Asian/French/uncivilized")
    lines.append("# city looks to different parts of the map -- here it's per")
    lines.append("# archetype+region combination instead of per region alone).")
    lines.append("")
    for (archetype_key, region_key, color_index, locale_name, mesh_names_by_distance) in locale_results:
        lines.append("city_group = {{ # {} ({} archetype, {} region)".format(locale_name, archetype_key, region_key))
        lines.append("\tcolor_index = {} # paint this palette index onto map/cities.bmp for {}".format(
            color_index, locale_name))
        lines.append("\tdensity = 0.5 # fraction of urban-blob pixels that spawn a building -- tune per locale")
        lines.append("")
        for distance in sorted(mesh_names_by_distance.keys()):
            names = mesh_names_by_distance[distance]
            lines.append("\tbuilding = {")
            lines.append("\t\tdistance = {} # 1 = sparse urban-blob edge, 4 = dense urban-blob core".format(distance))
            lines.append("\t\tmesh = {")
            for n in names:
                lines.append('\t\t\t"{}_entity"'.format(n))
            lines.append("\t\t}")
            lines.append("\t}")
        lines.append("}")
        lines.append("")

    with open(os.path.join(out_root, "map", "cities_fragment.txt"), "w") as f:
        f.write("\n".join(lines))


def generate_readme(out_root, locale_results):
    lines = []
    lines.append("# Generated placeholder city models")
    lines.append("")
    lines.append("Everything under `gfx/` and `map/` here is generated by")
    lines.append("`generate_city_models.py`. Nothing has been copied into your live")
    lines.append("mod -- do that yourself once you're happy with the results:")
    lines.append("")
    lines.append("1. Copy `gfx/models/buildings/<archetype>_<region>/` into your mod's")
    lines.append("   own `gfx/models/buildings/<archetype>_<region>/` folder.")
    lines.append("2. Merge the `city_group` blocks in `map/cities_fragment.txt` into")
    lines.append("   your mod's `map/cities.txt` (copy the vanilla file into your mod")
    lines.append("   first if you don't have your own copy yet -- HOI4 mods override")
    lines.append("   base files wholesale, they don't merge automatically).")
    lines.append("3. In `map/cities.bmp`, paint the locale(s) you want onto the")
    lines.append("   geographic areas where that building style should appear, using")
    lines.append("   the palette `color_index` values from step 2. This bitmap is")
    lines.append("   indexed-color -- edit the palette, don't just pick RGB values.")
    lines.append("4. Reload the map in-game (or restart) to see the new clutter.")
    lines.append("")
    lines.append("## Archetypes")
    lines.append("")
    lines.append("Buildings are split first by *actual building type* -- the")
    lines.append("archetype -- not just by region. Each archetype has its own mix of")
    lines.append("`BUILDING_TYPES` and its own spawn layout:")
    lines.append("")
    for key, cfg in ARCHETYPES.items():
        type_mix = ", ".join("{} {:.0%}".format(t, w) for t, w in cfg["type_weights"].items())
        lines.append("- **{}** ({}): {} layout, types: {}".format(key, cfg["abbr"], cfg["layout"], type_mix))
    lines.append("")
    lines.append("Two spawn layouts (see `layout_grid` / `layout_scatter` / `LAYOUTS`):")
    lines.append("")
    lines.append("- **grid** -- a tidy, lightly-jittered lattice. Used by archetypes that")
    lines.append("  should read as organized city blocks (`metropolis`'s towers,")
    lines.append("  `urban_core`'s street grid, `commie_block`'s uniform rows).")
    lines.append("- **scatter** -- wide positional jitter *and* a random yaw rotation per")
    lines.append("  building. Used by archetypes that should sprawl at irregular angles")
    lines.append("  (`suburb`'s houses on varied lots, `informal`'s chaotic packing).")
    lines.append("")
    lines.append("## Regions")
    lines.append("")
    lines.append("A region is a lighter-weight flavor layered on top of an archetype --")
    lines.append("it nudges footprint/height scale and picks the roof style small")
    lines.append("residential buildings use, so e.g. a western suburb and an east asian")
    lines.append("suburb are both unmistakably suburbs, just styled differently:")
    lines.append("")
    for key, cfg in REGIONS.items():
        lines.append("- **{}** ({}): {} roofs, footprint x{:.2f}, height x{:.2f}".format(
            key, cfg["abbr"], cfg["roof_style"], cfg["footprint_scale"], cfg["height_scale"]))
    lines.append("")
    lines.append("## Locales generated")
    lines.append("")
    lines.append("A LOCALE is one curated (archetype, region, color_index) combination --")
    lines.append("its own city_group / palette color, same as a vanilla \"region\" was:")
    lines.append("")
    for (archetype_key, region_key, color_index, locale_name, mesh_names_by_distance) in locale_results:
        lines.append("- **{}** (color_index {}): {}".format(
            locale_name, color_index,
            ", ".join("distance {} x{}".format(d, len(mesh_names_by_distance[d])) for d in sorted(mesh_names_by_distance))))
    lines.append("")
    lines.append("`distance` still follows vanilla's field in cities.txt: 1 is the sparse")
    lines.append("outer edge of an urban blob, 4 is the dense core -- here it's a mild")
    lines.append("size bump (`DISTANCE_SCALE`) applied on top of a locale's own")
    lines.append("archetype, so a locale never changes character across distances, only")
    lines.append("size. Edit `ARCHETYPES` / `REGIONS` / `LOCALES` at the top of the")
    lines.append("script to add more, retune footprints/heights/layouts, or change roof")
    lines.append("styles.")
    lines.append("")
    lines.append("## About the geometry")
    lines.append("")
    lines.append("Each `.mesh` file is a small city-block CLUSTER of 10-20 individual")
    lines.append("low-poly buildings (see `BUILDINGS_PER_MESH`), not a single building --")
    lines.append("this matches vanilla's own city meshes, which are likewise multi-")
    lines.append("building chunks. Building *type* is picked per building from its")
    lines.append("locale's archetype `type_weights` (see `ARCHETYPES`), and *layout* --")
    lines.append("grid vs. scatter, including per-building yaw rotation for scatter --")
    lines.append("also comes from the archetype (see `build_city_cluster`,")
    lines.append("`layout_grid`/`layout_scatter`). Each individual building is a")
    lines.append("deliberately crude, low-poly, flat-shaded box (with an optional")
    lines.append("pitched roof or a stacked \"setback\" block for towers) -- stand-ins to")
    lines.append("get the archetype/region/distance pipeline wired up and testable")
    lines.append("in-game before you commission or model real low-poly buildings. The")
    lines.append("`.mesh` files are real, valid PDX binary meshes (reverse-engineered")
    lines.append("from TGC-Hearts-of-Iron-IV's own files), so they will load in-engine")
    lines.append("as-is.")
    lines.append("")
    lines.append("## About the textures")
    lines.append("")
    lines.append("Every mesh gets its own \"dev texture\" diffuse atlas (see")
    lines.append("`_build_atlas_layout()` / `write_atlas_dds()`): a thin top strip")
    lines.append("carrying the mesh's own identifying code (`{ARCHETYPE_ABBR}-")
    lines.append("{REGION_ABBR}-T{distance}-{variant}`, e.g. `MET-EAS-T4-04`), then a")
    lines.append("grid with one colored, outlined, labeled cell per building type --")
    lines.append("`HOUSE`, `ROWHOUSE`, `SHOP`, `SHED`, `BLOCK`, `TOWER` -- and one per")
    lines.append("roof shape -- `PITCHED`, `FLAT`. Every wall face UVs into its")
    lines.append("building's own type cell (`WALL_UV_BY_TYPE`) and every roof face into")
    lines.append("its roof-shape cell (`ROOF_UV_BY_STYLE`), so a house's walls, a")
    lines.append("tower's walls, and a pitched vs. flat roof all read as visibly")
    lines.append("different materials in-engine -- not just \"wall\" vs. \"roof\", but a")
    lines.append("distinct look per building type, wherever that's actually useful")
    lines.append("(every type gets its own wall look; roofs only have two distinct")
    lines.append("*shapes*, so those two share their cells across every type that uses")
    lines.append("them). Normal/specular stay flat and shared per-locale. Edit")
    lines.append("`WALL_FILL_BY_TYPE` / `ROOF_FILL_BY_STYLE` to change colors, `FONT_5X7`")
    lines.append("to add characters, or `_build_atlas_layout()` to change the grid.")
    with open(os.path.join(out_root, "README.md"), "w") as f:
        f.write("\n".join(lines))


def main():
    out_root = OUTPUT_DIR
    os.makedirs(os.path.join(out_root, "map"), exist_ok=True)

    locale_results = []
    total_meshes = 0
    total_buildings = 0
    for (archetype_key, region_key, color_index) in LOCALES:
        archetype_cfg = ARCHETYPES[archetype_key]
        region_cfg = REGIONS[region_key]
        locale_name, mesh_names_by_distance, locale_buildings = generate_locale(
            archetype_key, archetype_cfg, region_key, region_cfg, color_index, out_root)
        locale_results.append((archetype_key, region_key, color_index, locale_name, mesh_names_by_distance))
        total_meshes += sum(len(v) for v in mesh_names_by_distance.values())
        total_buildings += locale_buildings

    generate_cities_txt_fragment(locale_results, out_root)
    generate_readme(out_root, locale_results)

    print("Generated {} locales ({} archetypes x {} regions curated) x {} distances x {} variants = {} mesh files, {} buildings total".format(
        len(LOCALES), len(ARCHETYPES), len(REGIONS), len(DISTANCE_LEVELS), VARIANTS_PER_TIER, total_meshes, total_buildings))
    print("Output written to: {}/".format(os.path.abspath(out_root)))


if __name__ == "__main__":
    main()
