/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2008 Andrey Nazarov
Copyright (C) 2019, NVIDIA CORPORATION. All rights reserved.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef BSP_H
#define BSP_H

#include "../Shared/List.h"
#include "Error.h"
#include "../System/Hunk.h"
#include "../Shared/Formats/Bsp.h"

#ifndef MIPLEVELS
#define MIPLEVELS 4
#endif

// maximum size of a PVS row, in bytes
#define VIS_MAX_BYTES   (MAX_MAP_LEAFS  >> 3)

// take advantage of 64-bit systems
#define VIS_FAST_LONGS(bsp) \
    (((bsp)->visrowsize + sizeof(uint_fast32_t) - 1) / sizeof(uint_fast32_t))

typedef struct mtexinfo_s {  // used internally due to name len probs //ZOID
    CollisionSurface          c;
    char                name[MAX_TEXNAME];

#if USE_REF
    int                 radiance;
    vec3_t              axis[2];
    vec2_t              offset;
#if REF_VKPT
	struct pbr_material_s *material;
#endif
#if REF_GL
	struct image_s      *image; // used for texturing
#endif
    int                 numframes;
    struct mtexinfo_s   *next; // used for animation
#if USE_REF == REF_SOFT
    vec_t               mipadjust;
#endif
#endif
} mtexinfo_t;

#if USE_REF
typedef struct {
    vec3_t      point;
} mvertex_t;

typedef struct {
    // indices into the bsp->basisvectors array
    uint32_t normal;
    uint32_t tangent;
    uint32_t bitangent;
} mbasis_t;

typedef struct {
    mvertex_t   *v[2];
#if USE_REF == REF_SOFT
    uintptr_t   cachededgeoffset;
#endif
} medge_t;

typedef struct {
    medge_t     *edge;
    int         vert;
} msurfedge_t;

#define SURF_TRANS_MASK (SurfaceFlags::Transparent33 | SurfaceFlags::Transparent66)
#define SURF_COLOR_MASK (SURF_TRANS_MASK | SurfaceFlags::Warp)
#define SURF_NOLM_MASK  (SURF_COLOR_MASK | SurfaceFlags::Flowing | SurfaceFlags::Sky)

#define DSURF_PLANEBACK     1

// for lightmap block calculation
#define S_MAX(surf) (((surf)->extents[0] >> 4) + 1)
#define T_MAX(surf) (((surf)->extents[1] >> 4) + 1)

typedef struct mface_s {
    msurfedge_t     *firstsurfedge;
    int             numsurfedges;

    CollisionPlane        *plane;
    int             drawflags; // DSURF_PLANEBACK, etc

    byte            *lightmap;
    byte            styles[MAX_LIGHTMAPS];
    int             numstyles;

    mtexinfo_t      *texinfo;
    int             texturemins[2];
    int             extents[2];

#if USE_REF == REF_GL
    int             texnum[2];
    int             statebits;
    int             firstvert;
    int             light_s, light_t;
    float           stylecache[MAX_LIGHTMAPS];
#else
    struct surfcache_s    *cachespots[MIPLEVELS]; // surface generation data
#endif

    int             firstbasis;

    int             drawframe;

    int             dlightframe;
    int             dlightbits;

    struct mface_s  *next;
} mface_t;
#endif

typedef struct mnode_s {
    /* ======> */
    CollisionPlane            *plane;     // never NULL to differentiate from leafs

	// PH: We need this data.
	bbox3_t bounds;
	//int visframe;
	//int                 numfaces;
//#if USE_REF
//    union {
//        vec_t           minmaxs[6];
        //struct bounds_t {
        //    bounds_t() = default;
        //    vec3_t mins;
        //    vec3_t maxs;
        //} bounds;
    //};
    int                 visframe;
//#endif
    struct mnode_s      *parent;
    /* <====== */

    struct mnode_s      *children[2];
// PH: We need this data.
#if USE_REF
    int                 numfaces;
    mface_t             *firstface;
#endif
} mnode_t;

typedef struct {
    CollisionPlane            *plane;
    mtexinfo_t          *texinfo;
} mbrushside_t;

typedef struct {
    int                 contents;
    int                 numsides;
    mbrushside_t        *firstbrushside;
    int                 checkcount;        // to avoid repeated testings
} mbrush_t;

typedef struct {
    /* ======> */
    CollisionPlane            *plane;     // always NULL to differentiate from nodes

	// PH: For testing axial planes
	bbox3_t bounds;

// PH: We need this data.
//#if USE_REF
//#if USE_REF
//    vec3_t              mins;
//    vec3_t              maxs;

    int                 visframe;
//#endif
    struct mnode_s      *parent;
    /* <====== */

    int             contents;
    int             cluster;
    int             area;
    mbrush_t        **firstleafbrush;
    int             numleafbrushes;
// PH: We need this data.
#if USE_REF
    mface_t         **firstleafface;
    int             numleaffaces;
#if USE_REF == REF_SOFT
    unsigned        key;
#endif
#endif

	// PH: For 'Shape' leaf nodes.
	int32_t shapeType;
	sphere_t sphereShape;
} mleaf_t;

typedef struct {
    unsigned    portalnum;
    unsigned    otherarea;
} mareaportal_t;

typedef struct {
    int             numareaportals;
    mareaportal_t   *firstareaportal;
    int             floodvalid;
} marea_t;

typedef struct mmodel_s {
#if USE_REF
    /* ======> */
    int             type;
    /* <====== */
#endif
    vec3_t          mins, maxs;
    vec3_t          origin;        // for sounds or lights
    mnode_t         *headNode;

#if USE_REF
    float           radius;

    int             numfaces;
    mface_t         *firstface;

#if USE_REF == REF_GL
    int             drawframe;
#endif
#endif
} mmodel_t;

typedef struct bsp_s {
    list_t      entry;
    int         refcount;

    unsigned    checksum;

    memhunk_t   hunk;

    int             numbrushsides;
    mbrushside_t    *brushsides;

    int             numtexinfo;
    mtexinfo_t      *texinfo;

    int             numplanes;
    CollisionPlane        *planes;

    int             numnodes;
    mnode_t         *nodes;

    int             numleafs;
    mleaf_t         *leafs;

    int             numleafbrushes;
    mbrush_t        **leafbrushes;

    int             nummodels;
    mmodel_t        *models;

    int             numbrushes;
    mbrush_t        *brushes;

    int             numvisibility;
    int             visrowsize;
    dvis_t          *vis;

    int             numentitychars;
    char            *entityString;

    int             numareas;
    marea_t         *areas;

    int             lastareaportal; // largest portal number used
    int             numareaportals; // size of the array below
    mareaportal_t   *areaportals;

#if USE_REF
    int             numfaces;
    mface_t         *faces;

    int             numleaffaces;
    mface_t         **leaffaces;

    int             numlightmapbytes;
    byte            *lightmap;

    int             numvertices;
    mvertex_t       *vertices;

    int             numedges;
    medge_t         *edges;

    int             numsurfedges;
    msurfedge_t     *surfedges;

    int             numbasisvectors;
    vec3_t          *basisvectors;

    int             numbases;
    mbasis_t        *bases;
#endif

    byte            *pvs_matrix;
    byte            *pvs2_matrix;
	qboolean        pvs_patched;

    qboolean extended;

	// WARNING: the 'name' string is actually longer than this, and the bsp_t structure is allocated larger than sizeof(bsp_t) in BSP_Load
    char            name[1];
} bsp_t;

qerror_t BSP_Load(const char *name, bsp_t **bsp_p);
void BSP_Free(bsp_t *bsp);
const char *BSP_GetError(void);

#if USE_REF
typedef struct {
    mface_t     *surf;
    CollisionPlane    plane;
    int         s, t;
    float       fraction;
} lightpoint_t;

void BSP_LightPoint(lightpoint_t *point, const vec3_t &start, const vec3_t &end, mnode_t *headNode);
void BSP_TransformedLightPoint(lightpoint_t* point, const vec3_t &start, const vec3_t &end,
                               mnode_t *headNode, const vec3_t &origin, vec3_t *angles);
#endif

byte *BSP_ClusterVis(bsp_t *bsp, byte *mask, int cluster, int vis);
mleaf_t *BSP_PointLeaf(mnode_t *node, const vec3_t &p);
mmodel_t *BSP_InlineModel(bsp_t *bsp, const char *name);

byte* BSP_GetPvs(bsp_t *bsp, int cluster);
byte* BSP_GetPvs2(bsp_t *bsp, int cluster);

qboolean BSP_SavePatchedPVS(bsp_t *bsp);

void BSP_Init(void);

#endif // BSP_H
