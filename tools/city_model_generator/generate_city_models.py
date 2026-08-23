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
              AND how they're physically arranged (see LAYOUT_PARAMS below). This
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
a small CLUSTER of individual low-poly buildings (mixed types drawn from
its archetype's own type_weights) merged into one file -- matching how
vanilla's own city meshes are city-block chunks, not single buildings. Most
archetypes cluster 10-20 buildings per mesh (BUILDINGS_PER_MESH); an
archetype can override this via its own "count_range" -- commie_block does,
at 5-10, since its buildings (long Soviet-style slabs) are individually much
bigger. Each building is an axis-aligned box (+ an optional pitched roof / a
smaller "setback" block on top for towers) -- these are meant to be
stand-ins you replace with real hand-modelled low-poly buildings later, not
final art. Whatever the count or layout, buildings within a cluster are
never allowed to actually overlap: every placement is checked against every
building already placed (accounting for its real size and yaw rotation)
before being committed, so a mesh can end up with slightly fewer buildings
than its count_range's upper bound suggests, but never with two models
visibly intersecting (see build_city_cluster).

How a cluster's buildings are actually *spawned* also varies by archetype
(see LAYOUT_PARAMS): a "grid" layout places buildings on a tidy,
lightly-jittered lattice (metropolis/urban_core/commie_block -- a block of
towers or slabs really is organized that way); a "scatter" layout places
them with wide positional jitter AND a random yaw rotation per building
(suburb/informal -- sprawling, irregularly angled lots).

Ships with 5 example regions (western, east_asian, mediterranean,
south_america, eastern_europe) and 5 example archetypes (suburb, urban_core,
metropolis, commie_block, informal), combined into 13 example locales -- add/
rename/recombine in REGIONS / ARCHETYPES / LOCALES below.

Texturing is one dedicated "dev texture" diffuse + matching normal +
matching specular PER KIND (house, rowhouse, shop, shed, block, tower,
pitched-roof, flat-roof) actually used by a given locale -- e.g.
`commie_block_eastern_europe_block_diffuse.dds` -- not one shared atlas
per locale, and not one generated per individual mesh either (see
generate_locale_textures). Content is identical for a given kind across
every locale that uses it (write_diffuse_texture/write_normal_texture/
write_specular_texture each cache their pixels once, module-wide, the
first time that kind is needed, then reuse them for every locale that
also needs it). The diffuse is colored and labeled via a tiny built-in
bitmap font (see write_diffuse_texture); the normal holds that kind's own
small bump pattern -- brick coursing, siding grooves, corrugated metal, or
a window/panel grid (see write_normal_texture / BUMP_PROFILES); the
specular holds that kind's own shininess, e.g. a tower/shop/block's window
panes reading brighter than their frame (see write_specular_texture).

Because a building's actual wall or roof face is almost never square --
a metropolis tower's wall is far taller than it is wide; a commie_block
slab's long face is far wider than it is tall -- every face still needs
to TILE its texture rather than stretch it. Each kind now being its OWN
standalone texture (not packed into a shared atlas) means this can be done
with the engine's own hardware UV-wrap/repeat addressing instead of extra
geometry: a face is written as a single quad (2 triangles) whose UVs
extend past 0..1 (e.g. 0..5 along one axis), and the bound texture simply
repeats across it for free (see MeshBuilder.add_tiled_quad). The tile
count on each axis comes from that axis's own absolute length divided by
WORLD_UNITS_PER_TILE, independently of the other axis, so a face tiles
because it's genuinely big, not merely because it's far more one
dimension than the other. (Scope note: this applies to every wall and
every flat roof cap; a pitched roof's sloped triangular faces still map
onto their texture as a single, untiled read -- tiling a triangle cleanly
takes meaningfully more geometry than a quad, and pitched roofs here are
only ever modest, low-rise shapes, so the stretching this mostly avoids
barely arises for them.)

Because one .mesh cluster mixes several building types together (a
suburb's houses, rowhouses, and shops, say), and each type now needs a
DIFFERENT bound texture, a single mesh file is written as several
sibling "shape" sub-meshes -- one per kind actually present in that
cluster -- each with its own local geometry and its own material, wired
up in the .gfx via multiple indexed `meshsettings` blocks under one
`pdxmesh` (see generate_locale / write_mesh_file). This leans on the
`index` field the vanilla .gfx format already exposes for exactly this
kind of multi-material grouping -- plausible, but not something this
script has been able to confirm renders correctly for city-clutter
meshes specifically, so it is worth a real in-game check after
generating (see the README's "About the textures" section).

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
BUILDINGS_PER_MESH = (10, 20)    # default per-mesh building count; an archetype can override via its own "count_range"
DISTANCE_LEVELS = (1, 2, 3, 4)   # cities.txt `distance`: 1 = edge of this locale's urban blob, 4 = its core

# Regional/material flavor -- layered ON TOP of an archetype (see
# ARCHETYPES below), not a city_group on its own. `footprint_scale` /
# `height_scale` multiply an archetype's own base size; `roof_style` is
# what this region's low-rise residential types (house/rowhouse) use
# (commercial/tall types stay flat-roofed everywhere -- see
# BUILDING_TYPES). `abbr` is just a short label used in the generated
# README's summary tables (see generate_readme) -- no length limit tied to
# the bitmap font, since it's never rendered onto a texture.
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
# slimmer block on top, for a skyscraper silhouette. "wall_type" (optional)
# makes this type reuse another type's diffuse-atlas wall segment instead
# of needing its own dedicated cell -- e.g. the commie_block slab_* types
# below are all the same "material" as a regular block, just a different
# massing (much longer, and each a different height tier), so they borrow
# "block"'s cell rather than bloating the shared atlas with near-duplicates.
BUILDING_TYPES = {
    "house":     {"w_mult": 0.90, "d_mult": 0.90, "h_mult": 0.80, "roof": "pitched"},
    "rowhouse":  {"w_mult": 0.60, "d_mult": 1.00, "h_mult": 1.10, "roof": "pitched"},
    "shop":      {"w_mult": 1.30, "d_mult": 1.00, "h_mult": 0.55, "roof": "flat"},
    "shed":      {"w_mult": 1.60, "d_mult": 0.80, "h_mult": 0.45, "roof": "flat"},
    "block":     {"w_mult": 1.10, "d_mult": 1.10, "h_mult": 1.15, "roof": "flat"},
    "tower":     {"w_mult": 0.85, "d_mult": 0.85, "h_mult": 1.60, "roof": "flat", "setback": True},
    # long Soviet-style residential slabs (commie_block archetype only) --
    "slab_low":  {"w_mult": 1.00, "d_mult": 2.2, "h_mult": 0.65, "roof": "flat", "wall_type": "block"},
    "slab_mid":  {"w_mult": 1.00, "d_mult": 2.6, "h_mult": 1.15, "roof": "flat", "wall_type": "block"},
    "slab_tall": {"w_mult": 0.95, "d_mult": 2.0, "h_mult": 1.85, "roof": "flat", "wall_type": "block"},
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
        "footprint": (7.0, 10.0), "height": (10.0, 16.0),
        # long slabs at 3 distinct height tiers ("various heights") instead
        # of one generic type -- see slab_low/slab_mid/slab_tall above.
        "type_weights": {"slab_low": 0.30, "slab_mid": 0.40, "slab_tall": 0.30},
        "layout": "grid", "pos_jitter": 0.02, "rot_jitter_deg": 0, "spread_mult": 1.0,
        # fewer buildings per mesh than other archetypes -- each one is
        # much longer, so the same 10-20 count would sprawl this cluster
        # far larger than every other locale's.
        "count_range": (5, 10),
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
# Per-KIND texture config. A "kind" is one wall building-type (house,
# rowhouse, shop, shed, block, tower) or one roof style (pitched, flat) --
# each kind gets its OWN standalone diffuse+normal+specular set (see
# generate_locale_textures / write_diffuse_texture), NOT a shared atlas.
# That's deliberate: hardware texture wrap/repeat addressing can only ever
# repeat the WHOLE bound texture, never just a sub-region of a multi-cell
# atlas -- so giving every kind its own file is what lets a face tile via
# plain UV coordinates past 0..1 (see MeshBuilder.add_tiled_quad) instead
# of subdividing geometry to fake it. One mesh cluster still mixes several
# kinds together (a suburb's houses + rowhouses + shops); see
# generate_locale/write_mesh_file for how that's wired up as multiple
# "shape" sub-meshes, each bound to its own kind's texture, inside one
# .mesh file.
# ---------------------------------------------------------------------------

WALL_TYPE_ORDER = ["house", "rowhouse", "shop", "shed", "block", "tower"]
ROOF_STYLE_ORDER = ["pitched", "flat"]
KIND_ORDER = WALL_TYPE_ORDER + ROOF_STYLE_ORDER   # canonical, deterministic kind ordering

KIND_TEX_PX = 128        # every kind's own diffuse/normal/specular is this many pixels square
LABEL_SCALE = 2

MAX_TILE_REPEATS = 24        # safety cap on how many times a face's UV extent repeats a kind's texture
WORLD_UNITS_PER_TILE = 5.0   # ~how many world units of wall one kind's own texture should cover

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


def _kind_fill(kind):
    return WALL_FILL_BY_TYPE.get(kind, ROOF_FILL_BY_STYLE.get(kind))


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
        = (u0, v0, u1, v1) maps the quad's own unit square into that UV
        range -- ordinarily just the texture's full (0,0)-(1,1), but
        add_tiled_quad passes a range that extends past 1 on purpose (see
        below)."""
        n = face_normal(p0, p1, p2)
        u0, v0, u1, v1 = uv_rect
        i0 = self._add_vertex(p0, n, (u0, v0))
        i1 = self._add_vertex(p1, n, (u1, v0))
        i2 = self._add_vertex(p2, n, (u1, v1))
        i3 = self._add_vertex(p3, n, (u0, v1))
        self.tris.append((i0, i1, i2))
        self.tris.append((i0, i2, i3))

    def add_tiled_quad(self, p0, p1, p2, p3):
        """Adds ONE quad (2 triangles, via add_quad) whose UVs extend past
        0..1 -- e.g. (0,0)-(5,3) -- so the ENGINE's own texture wrap/repeat
        addressing tiles this quad's bound texture across the face
        natively, at no extra geometry cost no matter how large the face
        is. This only works because each kind (house/rowhouse/.../pitched/
        flat) now gets its own STANDALONE texture (see
        generate_locale_textures) rather than being packed into a shared
        atlas -- hardware wrap always repeats the WHOLE bound texture, so
        it could never tile just one sub-region of a multi-cell atlas; an
        earlier version of this method subdivided the quad into many
        smaller sub-quads to fake tiling for exactly that reason, at a
        real triangle-count cost on large flat faces (a commie_block
        slab's long side, a tower's tall wall, a big flat roof cap).

        tiles_u/tiles_v are each computed from that axis's own absolute
        edge length divided by WORLD_UNITS_PER_TILE, independently of the
        other axis -- NOT from the two edges' ratio to each other -- so a
        face tiles because it's genuinely big, regardless of its aspect
        ratio (same reasoning as the old geometric version, just expressed
        as a UV extent instead of a sub-quad count).

        NOTE: this assumes the target texture's sampler uses wrap/repeat
        addressing (not clamp) -- the sane default for a tiling material,
        and what every reverse-engineered vanilla building material
        already implied, but not something this script can verify without
        an in-game look (see the module docstring / README)."""
        edge_u = v_len(v_sub(p1, p0))   # length of the quad's own "u" edge (p0->p1)
        edge_v = v_len(v_sub(p3, p0))   # length of the quad's own "v" edge (p0->p3)
        tiles_u = 1 if edge_u < 1e-6 else min(MAX_TILE_REPEATS, max(1, round(edge_u / WORLD_UNITS_PER_TILE)))
        tiles_v = 1 if edge_v < 1e-6 else min(MAX_TILE_REPEATS, max(1, round(edge_v / WORLD_UNITS_PER_TILE)))
        self.add_quad(p0, p1, p2, p3, uv_rect=(0.0, 0.0, float(tiles_u), float(tiles_v)))

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
        this is what lets scatter-layout buildings (see build_city_cluster /
        LAYOUT_PARAMS) sit at varied angles instead of always facing the
        same way.
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

def add_box_walls_and_flat_roof(wall_mb, roof_mb, hw, hd, wall_h, y0=0.0, roof=True):
    """4 side walls (into `wall_mb`) + optionally a flat roof cap (into
    `roof_mb`). No floor (never seen). Walls and the roof cap go into
    SEPARATE mesh builders because they're different "kinds" now -- each
    bound to its own standalone texture (see generate_locale_textures) --
    rather than different sub-rects of one shared atlas. Every face is
    still a single TILED quad (add_tiled_quad), not stretched, so a very
    tall wall or a long flat roof cap (e.g. commie_block's slabs) reads as
    a repeated texture instead of one squashed copy -- just via hardware
    UV-wrap now instead of extra geometry."""
    y1 = y0 + wall_h
    # Every wall quad below is deliberately wound so p1-p0 (add_quad/
    # add_tiled_quad's "u" edge) is always the HORIZONTAL edge (around the
    # building) and p3-p0 ("v" edge) is always the VERTICAL edge (up) --
    # same convention on all 4 walls. That consistency is what this bug
    # report ("each face is rotated differently") was missing: front/back
    # and left/right used to each be wound independently (whatever
    # happened to trace out the correct OUTWARD normal), so 2 of the 4
    # walls ended up with u/v transposed relative to the other 2 -- with a
    # flat fill color + tiny label that was nearly invisible, but once
    # walls got real directional bump patterns (brick coursing, corrugation
    # -- see BUMP_PROFILES) and hardware-wrap tiling (see add_tiled_quad),
    # a transposed face very visibly shows its pattern running sideways
    # instead of upright, and its texture repeating across the wrong axis.
    # Every quad below keeps its original (already-correct) outward
    # normal -- only which corner is p0 changes (a cyclic shift around the
    # same 4 corners doesn't change a planar quad's winding/normal).
    # front (-z)
    wall_mb.add_tiled_quad((-hw, y1, -hd), (hw, y1, -hd), (hw, y0, -hd), (-hw, y0, -hd))
    # back (+z)
    wall_mb.add_tiled_quad((-hw, y0, hd), (hw, y0, hd), (hw, y1, hd), (-hw, y1, hd))
    # left (-x)
    wall_mb.add_tiled_quad((-hw, y0, -hd), (-hw, y0, hd), (-hw, y1, hd), (-hw, y1, -hd))
    # right (+x)
    wall_mb.add_tiled_quad((hw, y1, -hd), (hw, y1, hd), (hw, y0, hd), (hw, y0, -hd))
    if roof:
        # top (+y)
        roof_mb.add_tiled_quad((-hw, y1, -hd), (-hw, y1, hd), (hw, y1, hd), (hw, y1, -hd))
    return y1


def add_pitched_roof(roof_mb, hw, hd, y1, roof_h):
    """Pitched roof slopes, into `roof_mb` (its own kind's standalone
    texture, same as a flat roof cap would use -- both are just "the
    roof"). Unlike the walls/flat roof cap (see add_tiled_quad), these
    triangular slopes are NOT tiled -- cleanly subdividing a triangle into
    square repeats is meaningfully more geometry than a quad, and pitched
    roofs here are only ever modest, low-rise shapes (house/rowhouse), so
    the stretching that tiling exists to avoid barely arises for them.
    (uv_rect left at add_tri's default (0,0,1,1) -- the whole standalone
    roof texture, one untiled read.)"""
    apex = (0.0, y1 + roof_h, 0.0)
    tl, tr = (-hw, y1, -hd), (hw, y1, -hd)   # front top edge
    bl, br = (-hw, y1, hd), (hw, y1, hd)     # back top edge
    roof_mb.add_tri(tr, tl, apex)   # front slope
    roof_mb.add_tri(bl, br, apex)   # back slope
    roof_mb.add_tri(tl, bl, apex)   # left slope
    roof_mb.add_tri(br, tr, apex)   # right slope


def build_building_mesh(width, depth, height, roof_style, setback, building_type, rng):
    """Builds one low-poly building's geometry, split by KIND rather than
    merged into one mesh: returns a dict {kind: MeshBuilder} with exactly
    two entries -- `building_type` (all its wall geometry, main box +
    setback box alike) and `roof_style` (its roof geometry, flat cap(s) or
    pitched slopes) -- each already carrying the right UVs for its own
    standalone texture (see generate_locale_textures). Splitting here,
    building-by-building, is what lets build_city_cluster merge many
    buildings of different kinds into one shared set of per-kind
    sub-meshes for the whole cluster (see write_mesh_file). `rng` is a
    seeded random.Random used for small per-variant jitter so the 4
    variants in a tier don't look identical (matching vanilla's approach of
    shipping 4 hand-varied meshes/tier)."""
    hw, hd = width / 2.0, depth / 2.0
    wall_mb = MeshBuilder()
    roof_mb = MeshBuilder()

    if roof_style == "pitched":
        wall_h = height * (1.0 - PITCHED_ROOF_HEIGHT_FRACTION)
        y1 = add_box_walls_and_flat_roof(wall_mb, roof_mb, hw, hd, wall_h, roof=False)
        add_pitched_roof(roof_mb, hw, hd, y1, height - wall_h)
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
        y1 = add_box_walls_and_flat_roof(wall_mb, roof_mb, hw, hd, main_h, roof=True)
        if setback:
            top_w = width * rng.uniform(0.45, 0.65)
            top_d = depth * rng.uniform(0.45, 0.65)
            top_h = height - main_h
            add_box_walls_and_flat_roof(wall_mb, roof_mb, top_w / 2.0, top_d / 2.0, top_h, y0=y1, roof=True)

    return {building_type: wall_mb, roof_style: roof_mb}


def _rect_axes(yaw):
    """The world-space unit axes (u, v) of a building's local (width,
    depth) directions after MeshBuilder.extend's yaw rotation. Must match
    extend's own rot_y exactly ((x,z) -> (x*cos+z*sin, -x*sin+z*cos)), or
    this collision check would be testing the wrong orientation."""
    cos_y, sin_y = math.cos(yaw), math.sin(yaw)
    u = (cos_y, -sin_y)   # local +x (half-extent hw) in world (x, z)
    v = (sin_y, cos_y)    # local +z (half-extent hd) in world (x, z)
    return u, v


def rects_too_close(cA, hwA, hdA, yawA, cB, hwB, hdB, yawB, margin):
    """Separating-Axis-Theorem overlap test for two independently rotated
    rectangles in the XZ plane, with `margin` extra clearance required on
    top of merely touching. This is what actually handles very elongated
    footprints correctly (e.g. commie_block's long slabs): a simple
    circumscribed-circle check sizes its clearance off a shape's DIAGONAL,
    which is wildly too conservative for two long, thin slabs standing
    side by side (close together across their short axis, nowhere near
    touching along their long one) -- SAT instead checks each rectangle's
    own two axes, so two long-but-narrow buildings can sit as close
    together, side to side, as their actual width allows."""
    uA, vA = _rect_axes(yawA)
    uB, vB = _rect_axes(yawB)
    dx, dz = cB[0] - cA[0], cB[1] - cA[1]

    for axis in (uA, vA, uB, vB):
        proj_a = hwA * abs(uA[0] * axis[0] + uA[1] * axis[1]) + hdA * abs(vA[0] * axis[0] + vA[1] * axis[1])
        proj_b = hwB * abs(uB[0] * axis[0] + uB[1] * axis[1]) + hdB * abs(vB[0] * axis[0] + vB[1] * axis[1])
        center_dist = abs(dx * axis[0] + dz * axis[1])
        if center_dist > proj_a + proj_b + margin:
            return False   # separated along this axis -- definitely not too close
    return True   # no separating axis found among the 4 candidates -- overlapping or within margin


def compute_grid_cells(count, cell_w, cell_d, rng):
    """Lays out `count` cell CENTERS on a roughly square lattice (3-5
    columns). This is only a starting proposal for where each building
    goes -- build_city_cluster jitters around these centers and, more
    importantly, checks every building's real footprint against every
    already-placed one before committing to a position, so the lattice
    itself doesn't need to be collision-proof on its own."""
    cols = rng.randint(3, 5)
    rows = math.ceil(count / cols)
    cells = []
    for row in range(rows):
        for col in range(cols):
            if len(cells) >= count:
                break
            cx = (col - (cols - 1) / 2.0) * cell_w
            cz = (row - (rows - 1) / 2.0) * cell_d
            cells.append((cx, cz))
    return cells


def jittered_candidate(cell_center, cell_w, cell_d, rng, pos_jitter, rot_jitter_deg, pos_scale, jitter_frac):
    """One candidate (x, z, yaw_radians) near `cell_center`. `jitter_frac`
    (1.0 down to 0.0) shrinks pos_jitter/rot_jitter_deg on repeated overlap
    retries in build_city_cluster -- at jitter_frac == 0.0 this collapses to
    the bare, unrotated cell center: the fallback of last resort."""
    cx, cz = cell_center
    bx = cx + rng.uniform(-pos_jitter, pos_jitter) * cell_w * pos_scale * jitter_frac
    bz = cz + rng.uniform(-pos_jitter, pos_jitter) * cell_d * pos_scale * jitter_frac
    yaw_deg = rng.uniform(-rot_jitter_deg, rot_jitter_deg) * jitter_frac
    return bx, bz, math.radians(yaw_deg)


# How tightly each layout hugs its cell lattice: "grid" only ever uses half
# of an archetype's own pos_jitter (and doesn't widen its cells) so a
# metropolis/urban_core/commie_block cluster still reads as organized;
# "scatter" uses the full jitter AND widens cells by the archetype's own
# spread_mult, so a suburb/informal cluster sprawls at real angles.
LAYOUT_PARAMS = {
    "grid": {"pos_scale": 0.5, "use_spread": False},
    "scatter": {"pos_scale": 1.0, "use_spread": True},
}

# On overlap, a building's position/rotation is retried with progressively
# less jitter before giving up -- 0.0 (a bare, unrotated cell center) is
# always tried last as the fallback of last resort.
OVERLAP_RETRY_JITTER_FRACS = (1.0, 0.7, 0.45, 0.2, 0.0)
MIN_BUILDING_GAP = 0.4   # small clear gap kept between any two footprints, so they never even touch


def _resolve_wall_and_roof(btype, region_cfg):
    """The wall KIND and roof KIND one BUILDING_TYPES entry actually uses,
    given a region: small residential types (house/rowhouse) take on the
    *region's* own roof_style (the regional-architecture cue); everything
    else -- shops, blocks, slabs, towers -- is flat-roofed regardless of
    region. A type may also borrow another type's wall kind entirely (see
    BUILDING_TYPES's "wall_type") instead of needing its own dedicated
    texture -- e.g. commie_block's slab_low/mid/tall all render as "block".
    Single source of truth shared by build_city_cluster (per building
    actually placed) and locale_used_kinds (every kind an archetype+region
    combo could ever need, up front, so their textures get generated)."""
    type_cfg = BUILDING_TYPES[btype]
    wall_kind = type_cfg.get("wall_type", btype)
    roof_kind = region_cfg["roof_style"] if btype in ("house", "rowhouse") else "flat"
    return wall_kind, roof_kind


def locale_used_kinds(archetype_cfg, region_cfg):
    """Every wall/roof KIND this (archetype, region) locale could ever
    need -- i.e. every kind any of its `type_weights` entries resolves to
    via _resolve_wall_and_roof -- in canonical KIND_ORDER. Computed up
    front so generate_locale_textures only ever writes the texture files
    this locale actually uses, not all 8 kinds unconditionally."""
    used = set()
    for btype in archetype_cfg["type_weights"]:
        wall_kind, roof_kind = _resolve_wall_and_roof(btype, region_cfg)
        used.add(wall_kind)
        used.add(roof_kind)
    return [k for k in KIND_ORDER if k in used]


def build_city_cluster(archetype_cfg, region_cfg, distance, rng):
    """Builds ONE .mesh's worth of content: a small city-block cluster of
    individual buildings of mixed types (count drawn from the archetype's
    own `count_range`, or BUILDINGS_PER_MESH if it doesn't set one), laid
    out and merged into a shared set of per-KIND sub-meshes (see
    build_building_mesh / write_mesh_file) -- one MeshBuilder per wall
    type or roof style actually present in this cluster, not one single
    combined mesh -- this mirrors vanilla's own city meshes, which are
    likewise multi-building clusters, not single buildings (one clutter
    placement = one little block, not one house).

    Building types are picked from the LOCALE's archetype (`type_weights`),
    not from region or distance -- that's the actual-building-type split:
    a metropolis leans on tower/block regardless of which region it's
    styled as, a suburb leans on house/rowhouse/shop regardless of distance.
    Region only nudges footprint/height scale and (for small residential
    types) roof style; distance only applies a mild overall size bump
    (DISTANCE_SCALE) on top of the archetype's own base size.

    How buildings are actually arranged also comes from the archetype: its
    `layout` ("grid" or "scatter", see LAYOUT_PARAMS) plus `pos_jitter` /
    `rot_jitter_deg` / `spread_mult` decide whether this cluster reads as an
    organized block or a sprawling, irregularly angled district. Whatever
    the layout, no two buildings are ever allowed to actually overlap: each
    one's real oriented footprint (accounting for its own randomized size
    AND its yaw rotation, via `rects_too_close`'s separating-axis test) is
    checked against every already-placed building before it's committed,
    retrying with
    progressively less jitter/rotation and, as an absolute last resort,
    dropping the building from this cluster entirely rather than ever let
    two models visibly intersect (see OVERLAP_RETRY_JITTER_FRACS) -- so a
    cluster can end up with slightly fewer buildings than `count` asked
    for, but never with overlapping ones."""
    count_lo, count_hi = archetype_cfg.get("count_range", BUILDINGS_PER_MESH)
    count = rng.randint(count_lo, count_hi)
    weights = archetype_cfg["type_weights"]
    type_names = list(weights.keys())
    type_probs = list(weights.values())

    base_fw, base_fd = archetype_cfg["footprint"]
    h_lo, h_hi = archetype_cfg["height"]
    dist_scale = DISTANCE_SCALE[distance]
    fscale = region_cfg["footprint_scale"] * dist_scale

    # Size the lattice's cells off the LARGEST building this archetype can
    # actually produce (not just its base footprint) -- a commie_block's
    # long slabs need much more per-cell room along their long axis than a
    # generic "block" would. This is only a starting proposal (the
    # per-building overlap-retry loop below is what actually guarantees no
    # overlap), so it just needs to be a reasonable guess that keeps most
    # buildings collision-free on their first placement attempt.
    max_w_mult = max(BUILDING_TYPES[t]["w_mult"] for t in type_names)
    max_d_mult = max(BUILDING_TYPES[t]["d_mult"] for t in type_names)
    cell_w = base_fw * max_w_mult * fscale * 1.35
    cell_d = base_fd * max_d_mult * fscale * 1.35

    layout_params = LAYOUT_PARAMS[archetype_cfg["layout"]]
    if layout_params["use_spread"]:
        cell_w *= archetype_cfg["spread_mult"]
        cell_d *= archetype_cfg["spread_mult"]

    cells = compute_grid_cells(count, cell_w, cell_d, rng)
    pos_jitter = archetype_cfg["pos_jitter"]
    rot_jitter_deg = archetype_cfg["rot_jitter_deg"]
    pos_scale = layout_params["pos_scale"]

    cluster_by_kind = {}   # {kind: MeshBuilder} -- accumulates every building's geometry, split by kind
    placed = []   # (x, z, half_width, half_depth, yaw) of every building already committed
    placed_count = 0

    for cell_center in cells:
        btype = rng.choices(type_names, weights=type_probs, k=1)[0]
        type_cfg = BUILDING_TYPES[btype]

        fw = base_fw * type_cfg["w_mult"] * fscale * rng.uniform(0.85, 1.15)
        fd = base_fd * type_cfg["d_mult"] * fscale * rng.uniform(0.85, 1.15)
        height = rng.uniform(h_lo, h_hi) * type_cfg["h_mult"] * region_cfg["height_scale"] * dist_scale
        hw, hd = fw / 2.0, fd / 2.0

        wall_type, roof_style = _resolve_wall_and_roof(btype, region_cfg)
        setback = type_cfg.get("setback", False)

        chosen = None
        for jitter_frac in OVERLAP_RETRY_JITTER_FRACS:
            bx, bz, yaw = jittered_candidate(
                cell_center, cell_w, cell_d, rng, pos_jitter, rot_jitter_deg, pos_scale, jitter_frac)
            if not any(rects_too_close(
                    (bx, bz), hw, hd, yaw, (px, pz), phw, phd, pyaw, MIN_BUILDING_GAP)
                    for (px, pz, phw, phd, pyaw) in placed):
                chosen = (bx, bz, yaw)
                break
        if chosen is None:
            # every attempt -- down to a dead-centered, unrotated placement
            # -- still overlapped a neighbour. Skip this slot rather than
            # ever let two building models visibly intersect.
            continue

        bx, bz, yaw = chosen
        building_by_kind = build_building_mesh(
            width=fw * UNIT_SCALE, depth=fd * UNIT_SCALE, height=height * UNIT_SCALE,
            roof_style=roof_style, setback=setback, building_type=wall_type, rng=rng,
        )
        offset = (bx * UNIT_SCALE, 0.0, bz * UNIT_SCALE)
        for kind, building_mb in building_by_kind.items():
            cluster_by_kind.setdefault(kind, MeshBuilder()).extend(building_mb, offset=offset, yaw=yaw)
        placed.append((bx, bz, hw, hd, yaw))
        placed_count += 1

    return cluster_by_kind, placed_count


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


def write_mesh_file(path, shapes, shader):
    """Writes one .mesh file containing MULTIPLE sibling "shape" nodes --
    one per (shape_name, mesh_builder, diffuse_tex, normal_tex) tuple in
    `shapes` -- each with its own fully self-contained mesh/aabb/material
    sub-tree, all under one top-level "object" node. This is what lets one
    city-cluster mesh mix several building kinds (house walls, block
    walls, a flat roof, ...) while each kind still binds its OWN
    standalone, hardware-tiled texture (see generate_locale_textures /
    MeshBuilder.add_tiled_quad) instead of everything sharing one atlas.
    The corresponding .gfx wires each shape up via its own indexed
    `meshsettings` block under one shared `pdxmesh` (see generate_locale)
    -- relying on the vanilla .gfx format's `index` field being meant for
    exactly this kind of multi-material grouping, which is plausible but
    not something confirmed in-game for city-clutter meshes specifically."""
    w = PDXWriter()
    w.raw(b"@@b@")
    w.prop_ints("pdxasset", [1, 0])

    w.node_open("object", 1)
    for shape_name, mb, diffuse_tex, normal_tex in shapes:
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


_DIFFUSE_BYTES_BY_KIND = {}   # cache: content only depends on `kind`, identical across every locale that uses it


def _build_diffuse_bytes(kind):
    size = KIND_TEX_PX
    BLACK = (0, 0, 0, 255)
    pixels = [_kind_fill(kind)] * (size * size)

    def set_px(x, y, color):
        if 0 <= x < size and 0 <= y < size:
            pixels[y * size + x] = color

    coords, text_w, text_h = _text_pixel_positions(kind.upper(), LABEL_SCALE)
    ox = max(0, (size - text_w) // 2)
    oy = max(0, (size - text_h) // 2)
    for (x, y) in coords:
        set_px(ox + x, oy + y, BLACK)

    buf = bytearray(size * size * 4)
    for i, p in enumerate(pixels):
        buf[i * 4:i * 4 + 4] = bytes(p)
    return bytes(buf)


def write_diffuse_texture(path, kind):
    """Writes this KIND's own standalone 'dev texture' placeholder diffuse
    -- a single flat fill color (see WALL_FILL_BY_TYPE/ROOF_FILL_BY_STYLE)
    plus its name stamped in the middle via a tiny built-in bitmap font --
    NOT a shared atlas cell any more (see generate_locale_textures for why:
    one standalone texture per kind is what lets a face rely on hardware
    UV-wrap tiling instead of subdividing geometry). Every locale that uses
    this kind gets a byte-identical copy; only the filename differs (e.g.
    `commie_block_eastern_europe_block_diffuse.dds` vs.
    `metropolis_western_block_diffuse.dds`) -- the actual pixels are
    computed once per kind and cached at module scope, then just
    rewritten to each locale's own path."""
    if kind not in _DIFFUSE_BYTES_BY_KIND:
        _DIFFUSE_BYTES_BY_KIND[kind] = _build_diffuse_bytes(kind)
    with open(path, "wb") as f:
        f.write(_dds_header(KIND_TEX_PX, KIND_TEX_PX))
        f.write(_DIFFUSE_BYTES_BY_KIND[kind])


# ---------------------------------------------------------------------------
# normal/specular textures -- one standalone image per kind (same size as
# its diffuse, KIND_TEX_PX square), with real per-kind bump/shininess
# detail instead of a single flat color -- a plain neutral normal + uniform
# low specular everywhere would look identical whether the wall behind it
# is "house" brick or "tower" curtain-wall glass. Each wall type / roof
# style gets a small deterministic, seamlessly-tileable procedural pattern
# (see BUMP_PROFILES): brick coursing, siding grooves, corrugated metal, or
# a window/panel grid for the normal map, and a matching base/accent
# shininess (e.g. glazing brighter than its frame) for the specular map.
# Every pattern's period evenly divides KIND_TEX_PX (128), so a texture
# butts up against a repeat of itself with no seam -- required since
# add_tiled_quad relies on hardware wrap to repeat it edge-to-edge across
# a face.
# ---------------------------------------------------------------------------

BUMP_PROFILES = {
    # wall types
    "house":    {"pattern": "ridge_h",     "period": 16, "amp": 0.35, "spec_base": 25,  "spec_accent": None},
    "rowhouse": {"pattern": "ridge_v",      "period": 16, "amp": 0.30, "spec_base": 20,  "spec_accent": None},
    "shop":     {"pattern": "grid", "period": 32, "mullion": 4, "amp": 0.45, "spec_base": 30, "spec_accent": 190},
    "shed":     {"pattern": "corrugated_v", "period": 8,  "amp": 0.55, "spec_base": 90,  "spec_accent": None},
    "block":    {"pattern": "grid", "period": 32, "mullion": 5, "amp": 0.35, "spec_base": 35, "spec_accent": 140},
    "tower":    {"pattern": "grid", "period": 16, "mullion": 2, "amp": 0.40, "spec_base": 40, "spec_accent": 210},
    # roof styles
    "pitched":  {"pattern": "ridge_h",  "period": 16, "amp": 0.30, "spec_base": 18, "spec_accent": None},
    "flat":     {"pattern": "stipple",  "period": 8,  "amp": 0.20, "spec_base": 22, "spec_accent": None},
}


def _bump_height(kind, px, py):
    """Deterministic small height field for `kind`'s bump pattern, sampled
    at LOCAL texture pixel coords (can go outside 0..KIND_TEX_PX-1 --
    callers sample neighbors for a gradient). Every pattern is built from
    sin/tri waves whose period evenly divides KIND_TEX_PX, so it's exactly
    periodic across one texture -- required for a seamless tile-to-tile
    repeat (see add_tiled_quad)."""
    prof = BUMP_PROFILES[kind]
    pattern, period, amp = prof["pattern"], prof["period"], prof["amp"]
    if pattern == "ridge_h":            # horizontal courses (brick/shingle rows)
        return amp * math.sin(2 * math.pi * py / period)
    if pattern == "ridge_v":            # vertical grooves (siding)
        return amp * math.sin(2 * math.pi * px / period)
    if pattern == "corrugated_v":       # tight vertical ridges (corrugated metal)
        t = (px % period) / period
        return amp * (4 * abs(t - 0.5) - 1)   # triangle wave, range [-amp, amp]
    if pattern == "grid":               # window/panel grid (frame raised, pane recessed)
        mullion = prof["mullion"]
        on_frame = (px % period) < mullion or (py % period) < mullion
        return amp if on_frame else -amp
    if pattern == "stipple":            # mottled roof membrane/gravel
        return amp * 0.5 * (math.sin(2 * math.pi * px / period) + math.sin(2 * math.pi * py / period))
    return 0.0


def _normal_bgra(kind, px, py, strength=60.0):
    """Tangent-space normal at local texture coords (px, py), encoded BGRA
    (matching _dds_header's B8G8R8A8 layout): neutral "flat, facing up"
    is (255,128,128,255) -- same convention the old flat placeholder used,
    just perturbed per-pixel here via a finite-difference gradient of
    _bump_height instead of being uniform everywhere."""
    h_l = _bump_height(kind, px - 1, py)
    h_r = _bump_height(kind, px + 1, py)
    h_u = _bump_height(kind, px, py - 1)
    h_d = _bump_height(kind, px, py + 1)
    nx, ny, nz = -(h_r - h_l) * strength, -(h_d - h_u) * strength, 1.0
    length = math.sqrt(nx * nx + ny * ny + nz * nz)
    nx, ny, nz = nx / length, ny / length, nz / length
    b = int(round((nz * 0.5 + 0.5) * 255))
    g = int(round((ny * 0.5 + 0.5) * 255))
    r = int(round((nx * 0.5 + 0.5) * 255))
    return (max(0, min(255, b)), max(0, min(255, g)), max(0, min(255, r)), 255)


def _specular_gray(kind, px, py):
    """Grayscale shininess (0-255) at local texture coords. Flat per kind
    except "grid"-pattern kinds (shop/block/tower), where the recessed
    "pane" area gets a distinct, brighter `spec_accent` (glazing is more
    reflective than its frame) -- so specular reads as a matching set of
    materials, not one uniform value regardless of wall type."""
    prof = BUMP_PROFILES[kind]
    if prof["pattern"] == "grid" and prof["spec_accent"] is not None:
        mullion = prof["mullion"]
        on_frame = (px % prof["period"]) < mullion or (py % prof["period"]) < mullion
        return prof["spec_base"] if on_frame else prof["spec_accent"]
    return prof["spec_base"]


def _build_kind_bytes(kind, pixel_fn):
    """Shared driver for write_normal_texture/write_specular_texture: fills
    one KIND_TEX_PX x KIND_TEX_PX standalone image for `kind`, calling
    `pixel_fn(kind, local_x, local_y)` -> BGRA per pixel. Computed once and
    cached by each caller, since -- like the diffuse -- content is
    identical across every locale that uses this kind; only the filename
    differs, so there's no need to redo this per locale."""
    size = KIND_TEX_PX
    buf = bytearray(size * size * 4)
    for py in range(size):
        row_base = py * size
        for px in range(size):
            idx = (row_base + px) * 4
            buf[idx:idx + 4] = bytes(pixel_fn(kind, px, py))
    return bytes(buf)


_NORMAL_BYTES_BY_KIND = {}
_SPECULAR_BYTES_BY_KIND = {}


def write_normal_texture(path, kind):
    """Writes this KIND's own standalone normal map -- same size as its
    diffuse (write_diffuse_texture), holding its own bump pattern (see
    BUMP_PROFILES/_normal_bgra) instead of a flat neutral normal. Cached at
    module scope per kind after the first call: the pixel content never
    varies by locale, only the destination filename does."""
    if kind not in _NORMAL_BYTES_BY_KIND:
        _NORMAL_BYTES_BY_KIND[kind] = _build_kind_bytes(kind, _normal_bgra)
    with open(path, "wb") as f:
        f.write(_dds_header(KIND_TEX_PX, KIND_TEX_PX))
        f.write(_NORMAL_BYTES_BY_KIND[kind])


def write_specular_texture(path, kind):
    """Writes this KIND's own standalone specular map -- same size as its
    diffuse/normal, a per-kind grayscale shininess (see BUMP_PROFILES/
    _specular_gray), e.g. a tower/shop/block's window panes reading
    brighter than their frame. Cached at module scope per kind for the
    same reason as write_normal_texture."""
    if kind not in _SPECULAR_BYTES_BY_KIND:
        _SPECULAR_BYTES_BY_KIND[kind] = _build_kind_bytes(
            kind, lambda k, x, y: (lambda v: (v, v, v, 255))(_specular_gray(k, x, y)))
    with open(path, "wb") as f:
        f.write(_dds_header(KIND_TEX_PX, KIND_TEX_PX))
        f.write(_SPECULAR_BYTES_BY_KIND[kind])


# ---------------------------------------------------------------------------
# generation driver
# ---------------------------------------------------------------------------

def locale_mesh_name(locale_name, distance, variant):
    return "{}_buildings_{}_{:02d}".format(locale_name, distance, variant)


def generate_locale_textures(locale_dir, locale_name, used_kinds):
    """Writes one dedicated diffuse/normal/specular set PER KIND in
    `used_kinds` (see locale_used_kinds) -- e.g.
    `commie_block_eastern_europe_block_diffuse.dds` -- instead of one
    shared-atlas set for the whole locale. Every texture still lives in
    `locale_dir` itself, right beside the meshes that reference it, so
    every reference to it (both a shape's own internal material node and
    the .gfx's texture_diffuse/texture_normal/texture_specular -- see
    generate_locale) can use a bare filename, matching the one
    confirmed-real vanilla convention (TGC-Hearts-of-Iron-IV's own
    TEST_building1.mesh: `diffs = "test_buildings_diffuse.dds"`, no path
    prefix) -- no cross-folder path to get wrong.

    Splitting by kind (rather than one packed atlas) is what lets a face
    tile via hardware UV-wrap instead of extra geometry (see
    MeshBuilder.add_tiled_quad) -- each kind's own pixels are cached once,
    module-wide, and reused for every locale that also needs that kind.

    Returns {kind: (diffuse_name, normal_name, specular_name)}."""
    textures = {}
    for kind in used_kinds:
        diffuse_name = "{}_{}_diffuse.dds".format(locale_name, kind)
        normal_name = "{}_{}_normal.dds".format(locale_name, kind)
        specular_name = "{}_{}_specular.dds".format(locale_name, kind)
        write_diffuse_texture(os.path.join(locale_dir, diffuse_name), kind)
        write_normal_texture(os.path.join(locale_dir, normal_name), kind)
        write_specular_texture(os.path.join(locale_dir, specular_name), kind)
        textures[kind] = (diffuse_name, normal_name, specular_name)
    return textures


def generate_locale(archetype_key, archetype_cfg, region_key, region_cfg, color_index, out_root):
    """Generates every mesh/texture/gfx/asset for one (archetype, region)
    LOCALE -- e.g. "metropolis_western" -- across all 4 DISTANCE_LEVELS x
    VARIANTS_PER_TIER variants, exactly like a vanilla "region" folder used
    to, just keyed on archetype+region together instead of region alone.
    Every mesh in this locale draws on this ONE locale's own per-kind
    texture sets (see generate_locale_textures) -- not one per mesh (too
    much duplication) and not one shared globally across every locale
    (broke texture lookup, back when it was one shared atlas -- see
    generate_locale_textures).

    Each .mesh file is written as several sibling "shape" sub-meshes, one
    per kind actually present in that cluster (see build_city_cluster /
    write_mesh_file), each wired to its own kind's texture set via its own
    indexed `meshsettings` block in the .gfx below."""
    locale_name = "{}_{}".format(archetype_key, region_key)
    locale_dir = os.path.join(out_root, "gfx", "models", "buildings", locale_name)
    os.makedirs(locale_dir, exist_ok=True)
    used_kinds = locale_used_kinds(archetype_cfg, region_cfg)
    textures_by_kind = generate_locale_textures(locale_dir, locale_name, used_kinds)

    gfx_lines = ["objectTypes = {"]
    asset_lines = []
    mesh_names_by_distance = {}
    total_buildings = 0

    for distance in DISTANCE_LEVELS:
        mesh_names_by_distance[distance] = []
        for variant in range(1, VARIANTS_PER_TIER + 1):
            name = locale_mesh_name(locale_name, distance, variant)
            rng = random.Random("{}-{}-{}".format(locale_name, distance, variant))

            # each .mesh is a small city-block cluster of mixed building
            # types drawn from this LOCALE's archetype, arranged by its
            # archetype's own layout, split by kind into per-kind
            # sub-meshes (see build_city_cluster).
            cluster_by_kind, building_count = build_city_cluster(archetype_cfg, region_cfg, distance, rng)
            total_buildings += building_count

            # deterministic order (KIND_ORDER), and only kinds this
            # specific cluster actually ended up using (a small cluster
            # can easily miss a low-probability type_weights entry).
            shapes = []   # one entry per sub-mesh: (shape_name, mb, diffuse, normal, specular)
            for kind in KIND_ORDER:
                if kind not in cluster_by_kind:
                    continue
                diffuse_tex, normal_tex, specular_tex = textures_by_kind[kind]
                shape_name = "{}Shape_{}".format(name, kind)
                shapes.append((shape_name, cluster_by_kind[kind], diffuse_tex, normal_tex, specular_tex))

            mesh_path = os.path.join(locale_dir, name + ".mesh")
            mesh_shapes = [(sn, mb, dt, nt) for (sn, mb, dt, nt, st) in shapes]
            write_mesh_file(mesh_path, mesh_shapes, "PdxMeshAdvanced")

            rel_mesh_path = "gfx/models/buildings/{}/{}.mesh".format(locale_name, name)
            gfx_lines.append("\tpdxmesh = {")
            gfx_lines.append('\t\tname = "{}_mesh"'.format(name))
            gfx_lines.append('\t\tfile = "{}"'.format(rel_mesh_path))
            for idx, (shape_name, mb, diffuse_tex, normal_tex, specular_tex) in enumerate(shapes):
                gfx_lines.append("\t\tmeshsettings = {")
                gfx_lines.append('\t\t\tname = "{}"'.format(shape_name))
                gfx_lines.append("\t\t\tindex = {}".format(idx))
                gfx_lines.append('\t\t\ttexture_diffuse = "{}"'.format(diffuse_tex))
                gfx_lines.append('\t\t\ttexture_normal = "{}"'.format(normal_tex))
                gfx_lines.append('\t\t\ttexture_specular = "{}"'.format(specular_tex))
                gfx_lines.append('\t\t\tshader = "PdxMeshAdvanced"')
                gfx_lines.append("\t\t}")
            gfx_lines.append("\t}")

            asset_lines.append('entity = {')
            asset_lines.append('\tname = "{}_entity"'.format(name))
            asset_lines.append('\tpdxmesh = "{}_mesh"'.format(name))
            asset_lines.append('\tscale = 0.06')
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
        lines.append("city_group = {")
        lines.append("\tcolor_index = {} ".format(
            color_index))
        lines.append("\tdensity = 0.5")
        lines.append("")
        for distance in sorted(mesh_names_by_distance.keys()):
            names = mesh_names_by_distance[distance]
            lines.append("\tbuilding = {")
            lines.append("\t\tdistance = {}".format(distance))
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
    lines.append("1. Copy `gfx/models/buildings/<archetype>_<region>/` (one self-contained")
    lines.append("   folder per locale -- meshes AND that locale's own diffuse/normal/")
    lines.append("   specular together, see \"About the textures\" below) into your mod's")
    lines.append("   own `gfx/models/buildings/`.")
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
        c_lo, c_hi = cfg.get("count_range", BUILDINGS_PER_MESH)
        lines.append("- **{}** ({}): {} layout, {}-{} buildings/mesh, types: {}".format(
            key, cfg["abbr"], cfg["layout"], c_lo, c_hi, type_mix))
    lines.append("")
    lines.append("Two spawn layouts (see `build_city_cluster` / `LAYOUT_PARAMS`):")
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
    lines.append("Each `.mesh` file is a small city-block CLUSTER of individual")
    lines.append("low-poly buildings (count per archetype's own `count_range`, or")
    lines.append("`BUILDINGS_PER_MESH` = 10-20 by default), not a single building -- this")
    lines.append("matches vanilla's own city meshes, which are likewise multi-building")
    lines.append("chunks. Building *type* is picked per building from its locale's")
    lines.append("archetype `type_weights` (see `ARCHETYPES`), and *layout* -- grid vs.")
    lines.append("scatter, including per-building yaw rotation for scatter -- also comes")
    lines.append("from the archetype (see `build_city_cluster`, `LAYOUT_PARAMS`). No two")
    lines.append("buildings in a cluster are ever allowed to actually overlap: each")
    lines.append("placement is checked (accounting for its real size and yaw) against")
    lines.append("every building already placed, retrying with progressively less jitter")
    lines.append("before dropping the building entirely as a last resort -- so a cluster")
    lines.append("can occasionally land a little under its `count_range`, but never with")
    lines.append("two models visibly intersecting. Each individual building is a")
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
    lines.append("Each LOCALE gets one dedicated diffuse/normal/specular set PER KIND it")
    lines.append("actually uses -- a kind being one wall building-type (`house`,")
    lines.append("`rowhouse`, `shop`, `shed`, `block`, `tower`) or one roof style")
    lines.append("(`pitched`, `flat`) -- e.g.")
    lines.append("`commie_block_eastern_europe_block_diffuse.dds`, living right beside")
    lines.append("that locale's own `.mesh`/`.gfx`/`.asset` files (see")
    lines.append("`generate_locale_textures()` / `locale_used_kinds()`). This is NOT one")
    lines.append("shared atlas per locale any more -- every kind gets its own small,")
    lines.append("standalone, square image. Content is identical for a given kind across")
    lines.append("every locale that uses it (e.g. every archetype using `block` gets a")
    lines.append("byte-identical `..._block_diffuse.dds`, just under a different")
    lines.append("locale-prefixed filename) -- each kind's pixels are computed once,")
    lines.append("module-wide, and reused. Every reference to a texture -- both a shape's")
    lines.append("own internal material node and the `.gfx`'s `texture_diffuse`/")
    lines.append("`texture_normal`/`texture_specular` -- still uses a bare filename, no")
    lines.append("path, matching the one confirmed-real vanilla convention (a")
    lines.append("TGC-Hearts-of-Iron-IV sample .mesh's own material node: `diffs =")
    lines.append("\"test_buildings_diffuse.dds\"`, no path prefix), since the texture")
    lines.append("always sits in the same folder as whatever references it.")
    lines.append("")
    lines.append("Splitting textures by kind instead of packing them into a shared atlas")
    lines.append("is specifically what lets a face TILE ITS TEXTURE WITH ONLY 1 QUAD (2")
    lines.append("triangles), no matter how large that face is (see")
    lines.append("`MeshBuilder.add_tiled_quad()`). Hardware texture wrap/repeat addressing")
    lines.append("can only ever repeat the WHOLE bound texture, never a sub-region of a")
    lines.append("multi-cell atlas -- so an earlier version of this script, which packed")
    lines.append("every kind into one shared atlas, had to fake tiling by subdividing")
    lines.append("each face into many small sub-quads instead, at a real triangle-count")
    lines.append("cost on large flat surfaces (a commie_block slab's long side, a")
    lines.append("tower's tall wall, a big flat roof cap). Now that each kind owns its")
    lines.append("own texture, a face's UVs simply extend past 0..1 (e.g. 0..5 along one")
    lines.append("axis) and the engine's own wrap addressing repeats the texture across")
    lines.append("it for free. The UV extent on each axis (`tiles_u`/`tiles_v`) still")
    lines.append("comes from that axis's own absolute length divided by")
    lines.append("`WORLD_UNITS_PER_TILE`, independently of the other axis -- so a face")
    lines.append("tiles because it's genuinely large, regardless of its aspect ratio, not")
    lines.append("because it happens to be far more one dimension than the other.")
    lines.append("(Pitched roof slopes are the one exception -- still a single untiled")
    lines.append("read per triangular face, since tiling a triangle cleanly takes")
    lines.append("meaningfully more geometry than a quad, and pitched roofs here are only")
    lines.append("ever modest, low-rise shapes where the stretching this mostly avoids")
    lines.append("barely arises anyway.)")
    lines.append("")
    lines.append("**This does rely on one assumption worth an in-game check:** that the")
    lines.append("bound texture's sampler actually uses wrap/repeat addressing (not")
    lines.append("clamp). That's the sane default for a tiling building material, and")
    lines.append("what every reverse-engineered vanilla sample already implied, but it's")
    lines.append("not something this script can verify without seeing it render. If a")
    lines.append("tiled face looks stretched (one smear across the whole face) rather")
    lines.append("than repeated in-game, that's the symptom to look for.")
    lines.append("")
    lines.append("A `BUILDING_TYPES` entry can set `wall_type` to borrow another type's")
    lines.append("texture set entirely instead of getting its own -- e.g. commie_block's")
    lines.append("`slab_low`/`slab_mid`/`slab_tall` (same material as a regular block,")
    lines.append("just longer and at a different height tier) all render with `block`'s")
    lines.append("texture set. `locale_used_kinds()` only generates the kinds a given")
    lines.append("archetype+region combination could actually need, so a locale that")
    lines.append("never uses, say, `tower` doesn't get a `tower` texture set at all.")
    lines.append("")
    lines.append("Because one `.mesh` cluster mixes several kinds together (a suburb's")
    lines.append("houses, rowhouses, and shops in the same file, say), and each kind now")
    lines.append("needs a DIFFERENT bound texture, one mesh file is written as several")
    lines.append("sibling \"shape\" sub-meshes -- one per kind actually present in that")
    lines.append("cluster -- each with its own local geometry and its own material,")
    lines.append("wired up in the `.gfx` via multiple indexed `meshsettings` blocks under")
    lines.append("one shared `pdxmesh` (see `generate_locale()` / `write_mesh_file()`).")
    lines.append("This leans on the `index` field the vanilla `.gfx` format already")
    lines.append("exposes, which strongly suggests it's meant for exactly this kind of")
    lines.append("multi-material grouping -- but, like the wrap-addressing point above,")
    lines.append("it's an assumption this script hasn't been able to confirm renders")
    lines.append("correctly for city-clutter meshes specifically. If a cluster mesh")
    lines.append("shows some building kinds correctly but others missing or wrong, that's")
    lines.append("the thing to check next -- worth a real in-game look either way before")
    lines.append("you build on this at scale.")
    lines.append("")
    lines.append("The normal and specular maps are generated the same way, one per kind,")
    lines.append("same size as the diffuse (see `write_normal_texture()` /")
    lines.append("`write_specular_texture()` / `BUMP_PROFILES`) -- not just a flat value:")
    lines.append("the normal map holds a small kind-specific bump pattern (brick coursing")
    lines.append("for `house`, siding grooves for `rowhouse`, corrugation for `shed`, a")
    lines.append("window/panel grid for `shop`/`block`/`tower`, roof shingle ridges/")
    lines.append("membrane stipple for the two roof kinds), and the specular map holds a")
    lines.append("matching per-kind shininess (e.g. `tower`'s glazing reads brighter than")
    lines.append("its frame). Every pattern's period evenly divides `KIND_TEX_PX` so a")
    lines.append("texture repeats seamlessly against itself when tiled.")
    lines.append("")
    lines.append("A per-kind (rather than per-mesh) diffuse still means there's no room")
    lines.append("for a per-mesh identifying code baked into the texture the way an even")
    lines.append("earlier version of this script did (e.g. `MET-EAS-T4-04`) -- text on it")
    lines.append("can only ever identify the kind, which the filename already does. Each")
    lines.append("mesh, entity, shape, and pdxmesh name is still fully identifying on")
    lines.append("disk and in any asset browser/outliner (e.g.")
    lines.append("`metropolis_western_buildings_2_01_entity`), just not on the texture")
    lines.append("itself.")
    lines.append("")
    lines.append("Edit `WALL_FILL_BY_TYPE` / `ROOF_FILL_BY_STYLE` to change diffuse colors,")
    lines.append("`BUMP_PROFILES` to change normal/specular patterns and shininess,")
    lines.append("`FONT_5X7` to add characters, `KIND_TEX_PX` to change resolution, or")
    lines.append("`WORLD_UNITS_PER_TILE` to change how densely faces tile.")
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
