/***
*
*	License here.
*
*	@file
* 
*   Client Game Temp Entities.
*
***/
//! Main Headers.
#include "Game/Client/ClientGameMain.h"
//! ClientGame Local headers.
#include "Game/Client/ClientGameLocals.h"

#include "Game/Client/Debug.h"
#include "Game/Client/TemporaryEntities.h"

// Exports.
#include "Game/Client/Exports/View.h"

#include "Game/Client/Effects/DynamicLights.h"
#include "Game/Client/Effects/Particles.h"
#include "Game/Client/Effects/ParticleEffects.h"

// ClientGameWorld.
#include "Game/Client/World/ClientGameWorld.h"

//
// CVars.
//
static color_t  railcore_color;
static color_t  railspiral_color;

static cvar_t* cl_railtrail_type;
static cvar_t* cl_railtrail_time;
static cvar_t* cl_railcore_color;
static cvar_t* cl_railcore_width;
static cvar_t* cl_railspiral_color;
static cvar_t* cl_railspiral_radius;

//
// Handles to Sound Effects.
//
qhandle_t   cl_sfx_ric1;
qhandle_t   cl_sfx_ric2;
qhandle_t   cl_sfx_ric3;
qhandle_t   cl_sfx_lashit;
qhandle_t   cl_sfx_flare;
qhandle_t   cl_sfx_spark5;
qhandle_t   cl_sfx_spark6;
qhandle_t   cl_sfx_spark7;
qhandle_t   cl_sfx_railg;
qhandle_t   cl_sfx_explosion;
qhandle_t   cl_sfx_grenexp;
qhandle_t   cl_sfx_water_explosion;
qhandle_t   cl_sfx_footsteps[4];

qhandle_t   cl_sfx_lightning;
qhandle_t   cl_sfx_disrexp;

//
// Handles to Models.
//
qhandle_t   cl_mod_explode;
qhandle_t   cl_mod_smoke;
qhandle_t   cl_mod_flash;
qhandle_t   cl_mod_parasite_segment;
qhandle_t   cl_mod_grapple_cable;
qhandle_t   cl_mod_explo4;
qhandle_t   cl_mod_laser;
qhandle_t   cl_mod_explosions[4];

qhandle_t   cl_mod_lightning;
qhandle_t   cl_mod_heatbeam;
qhandle_t   cl_mod_explo4_big;


//extern qboolean SCR_ParseColor(const char* s, color_t* color);

static void cl_railcore_color_changed(cvar_t* self)
{
	// TODO: Implement alternative methods for this of course.
	//if (!SCR_ParseColor(self->string, &railcore_color)) {
		//Com_WPrint("Invalid value '%s' for '%s'\n", self->string, self->name);
		CLG_Print( PrintType::Warning, "Rail Color Parsing is currently unsupported.\n");	
		clgi.Cvar_Reset(self);
		railcore_color.u32 = U32Colors::Red;
	//}
}

static void cl_railspiral_color_changed(cvar_t* self)
{
	//if (!SCR_ParseColor(self->string, &railspiral_color)) {
		//Com_WPrint("Invalid value '%s' for '%s'\n", self->string, self->name);
		CLG_Print( PrintType::Warning, "Rail Color Parsing is currently unsupported.\n");
		clgi. Cvar_Reset(self);
		railspiral_color.u32 = U32Colors::Orange;
	//}
}

//
//=============================================================================
//
//	LASER MANAGEMENT
//
//=============================================================================
//

#define MAX_LASERS  32

typedef struct {
	vec3_t      start;
	vec3_t      end;
	int         color;
	color_t     rgba;
	int         width;
	int         lifetime, starttime;
} laser_t;

static laser_t  clg_lasers[MAX_LASERS];

static void CLG_ClearLasers(void)
{
	memset(clg_lasers, 0, sizeof(clg_lasers));
}

static laser_t* CLG_AllocLaser(void)
{
	laser_t* l;
	int i;

	for (i = 0, l = clg_lasers; i < MAX_LASERS; i++, l++) {
		if (cl->time - l->starttime >= l->lifetime) {
			memset(l, 0, sizeof(*l));
			l->starttime = cl->time;
			return l;
		}
	}

	return NULL;
}

static void CLG_AddLasers(void)
{
	laser_t* l;
	r_entity_t    ent;
	int         i;
	int         time;

	memset(&ent, 0, sizeof(ent));

	for (i = 0, l = clg_lasers; i < MAX_LASERS; i++, l++) {
		time = l->lifetime - (cl->time - l->starttime);
		if (time < 0) {
			continue;
		}

		if (l->color == -1) {
			float f = (float)time / (float)l->lifetime;

			ent.rgba.u8[0] = l->rgba.u8[0];
			ent.rgba.u8[1] = l->rgba.u8[1];
			ent.rgba.u8[2] = l->rgba.u8[2];
			ent.rgba.u8[3] = l->rgba.u8[3] * f;
			ent.alpha = f;
		}
		else {
			ent.alpha = 0.30f;
		}

		ent.skinNumber = l->color;
		ent.flags = RenderEffects::Translucent | RenderEffects::Beam;
		VectorCopy(l->start, ent.origin);
		VectorCopy(l->end, ent.oldorigin);
		ent.frame = l->width;

		clge->view->AddRenderEntity(ent);
	}
}

static void CLG_ParseLaser(int colors)
{
	laser_t* l;

	l = CLG_AllocLaser();
	if (!l)
		return;

	VectorCopy(teParameters.position1, l->start);
	VectorCopy(teParameters.position2, l->end);
	l->lifetime = 100;
	l->color = (colors >> ((rand() % 4) * 8)) & 0xff;
	l->width = 4;
}


//
//=============================================================================
//
//	EXPLOSION MANAGEMENT
//
//=============================================================================
//
explosion_t  clg_explosions[MAX_EXPLOSIONS];

static void CLG_ClearExplosions(void)
{
	memset(clg_explosions, 0, sizeof(clg_explosions));
}

static explosion_t* CLG_AllocExplosion(void)
{
	explosion_t* e, * oldest;
	int     i;
	int     time;

	for (i = 0, e = clg_explosions; i < MAX_EXPLOSIONS; i++, e++) {
		if (e->type == explosion_t::ex_free) { // CPP: Enum
			memset(e, 0, sizeof(*e));
			return e;
		}
	}
	// find the oldest explosion
	time = cl->time;
	oldest = clg_explosions;

	for (i = 0, e = clg_explosions; i < MAX_EXPLOSIONS; i++, e++) {
		if (e->start < time) {
			time = e->start;
			oldest = e;
		}
	}
	memset(oldest, 0, sizeof(*oldest));
	return oldest;
}

static explosion_t* CLG_PlainExplosion(qboolean big, const vec3_t &origin) {
	explosion_t* ex;

	ex = CLG_AllocExplosion();
	ex->ent.origin = origin; //VectorCopy(teParameters.position1, ex->ent.origin);
	ex->type = explosion_t::ex_poly; // CPP: Enum
	ex->ent.flags = RenderEffects::FullBright;
	ex->start = cl->serverTime - CLG_FRAMETIME;
	ex->light = 350;
	VectorSet(ex->lightcolor, 1.0, 0.5, 0.5);
	ex->ent.angles[1] = rand() % 360;

	int model_idx = rand() % (sizeof(cl_mod_explosions) / sizeof(*cl_mod_explosions));
	model_t* sprite_model = clgi.CL_Model_GetModelByHandle(cl_mod_explosions[model_idx]);

	if (cl_explosion_sprites->integer && !big && sprite_model)
	{
		ex->ent.model = cl_mod_explosions[model_idx];
		ex->frames = sprite_model->numframes;
		ex->frameTime = cl_explosion_frametime->integer;
	}
	else
	{
		ex->ent.model = big ? cl_mod_explo4_big : cl_mod_explo4;
		if (frand() < 0.5)
			ex->baseFrame = 15;
		ex->frames = 15;
	}

	return ex;
}

/*
=================
CL_SmokeAndFlash
=================
*/
void CLG_SmokeAndFlash(vec3_t origin)
{
	explosion_t* ex;

	ex = CLG_AllocExplosion();
	VectorCopy(origin, ex->ent.origin);
	ex->type = explosion_t::ex_misc; // CPP: Enum
	ex->frames = 4;
	ex->ent.flags = RenderEffects::Translucent | RF_NOSHADOW;
	ex->start = cl->serverTime - CLG_FRAMETIME;
	ex->ent.model = cl_mod_smoke;

	ex = CLG_AllocExplosion();
	VectorCopy(origin, ex->ent.origin);
	ex->type = explosion_t::ex_flash; // CPP: Enum
	ex->ent.flags = RenderEffects::FullBright;
	ex->frames = 2;
	ex->start = cl->serverTime - CLG_FRAMETIME;
	ex->ent.model = cl_mod_flash;
}

#define LENGTH(a) ((sizeof (a)) / (sizeof(*(a))))

typedef struct light_curve_s {
	vec3_t color;
	float radius;
	float offset;
} light_curve_t;

static light_curve_t ex_poly_light[] = {
	{ { 0.4f,       0.2f,       0.02f     }, 12.5f, 20.00f },
	{ { 0.351563f,  0.175781f,  0.017578f }, 15.0f, 23.27f },
	{ { 0.30625f,   0.153125f,  0.015312f }, 20.0f, 24.95f },
	{ { 0.264062f,  0.132031f,  0.013203f }, 22.5f, 25.01f },
	{ { 0.225f,     0.1125f,    0.01125f  }, 25.0f, 27.53f },
	{ { 0.189063f,  0.094531f,  0.009453f }, 27.5f, 28.55f },
	{ { 0.15625f,   0.078125f,  0.007813f }, 30.0f, 30.80f },
	{ { 0.126563f,  0.063281f,  0.006328f }, 27.5f, 40.43f },
	{ { 0.1f,       0.05f,      0.005f    }, 25.0f, 49.02f },
	{ { 0.076563f,  0.038281f,  0.003828f }, 22.5f, 58.15f },
	{ { 0.05625f,   0.028125f,  0.002812f }, 20.0f, 61.03f },
	{ { 0.039063f,  0.019531f,  0.001953f }, 17.5f, 63.59f },
	{ { 0.025f,     0.0125f,    0.00125f  }, 15.0f, 66.47f },
	{ { 0.014063f,  0.007031f,  0.000703f }, 12.5f, 71.34f },
	{ { 0.f,        0.f,        0.f       }, 10.0f, 72.00f }
};

static light_curve_t ex_blaster_light[] = {
	{ { 0.04f,      0.02f,      0.0f      },  5.f, 15.00f },
	{ { 0.2f,       0.15f,      0.01f     }, 15.f, 15.00f },
	{ { 0.04f,      0.02f,      0.0f      },  5.f, 15.00f },
};

static void CLG_AddExplosionLight(explosion_t* ex, float phase)
{
	int curve_size;
	light_curve_t* curve;

	switch (ex->type)
	{
	case explosion_t::ex_poly: // CPP: Enum
		curve = ex_poly_light;
		curve_size = LENGTH(ex_poly_light);
		break;
	case explosion_t::ex_blaster: // CPP: Enum
		curve = ex_blaster_light;
		curve_size = LENGTH(ex_blaster_light);
		break;
	default:
		return;
	}

	float timeAlpha = ((float)(curve_size - 1)) * phase;
	int baseSample = (int)floorf(timeAlpha);
	baseSample = max(0, min(curve_size - 2, baseSample));

	float w1 = timeAlpha - (float)(baseSample);
	float w0 = 1.f - w1;

	light_curve_t* s0 = curve + baseSample;
	light_curve_t* s1 = curve + baseSample + 1;

	float offset = w0 * s0->offset + w1 * s1->offset;
	float radius = w0 * s0->radius + w1 * s1->radius;

	vec3_t origin;
	vec3_t up;
	AngleVectors(ex->ent.angles, NULL, NULL, &up);
	VectorMA(ex->ent.origin, offset, up, origin);

	vec3_t color;
	VectorClear(color);
	VectorMA(color, w0, s0->color, color);
	VectorMA(color, w1, s1->color, color);

	clge->view->AddLight(origin, color, 500.f, radius);
}

static void CLG_AddExplosions(void)
{
	r_entity_t* ent;
	int         i;
	explosion_t* ex;
	float       frac;
	int         f;

	memset(&ent, 0, sizeof(ent));

	for (i = 0, ex = clg_explosions; i < MAX_EXPLOSIONS; i++, ex++) {
		if (ex->type == explosion_t::ex_free) // CPP: Cast
			continue;
		float inv_frametime = ex->frameTime ? 1.f / (float)ex->frameTime : BASE_1_FRAMETIME;
		frac = (cl->time - ex->start) * inv_frametime;
		f = floor(frac);

		ent = &ex->ent;

		switch (ex->type) {
		case explosion_t::ex_mflash:
			if (f >= ex->frames - 1) {
				ex->type = explosion_t::ex_free;
			}
			break;
		case explosion_t::ex_misc:
		case explosion_t::ex_blaster:
		case explosion_t::ex_flare:
		case explosion_t::ex_light:
			if (f >= ex->frames - 1) {
				ex->type = explosion_t::ex_free;
				break;
			}
			ent->alpha = 1.0 - frac / (ex->frames - 1);
			break;
		case explosion_t::ex_flash:
			if (f >= 1) {
				ex->type = explosion_t::ex_free;
				break;
			}
			ent->alpha = 1.0;
			break;
		case explosion_t::ex_poly:
			if (f >= ex->frames - 1) {
				ex->type = explosion_t::ex_free;
				break;
			}

			ent->alpha = ((float)ex->frames - (float)f) / (float)ex->frames;
			ent->alpha = max(0.f, min(1.f, ent->alpha));
			ent->alpha = ent->alpha * ent->alpha * (3.f - 2.f * ent->alpha); // smoothstep

			if (f < 10) {
				ent->skinNumber = (f >> 1);
				if (ent->skinNumber < 0) {
					ent->skinNumber = 0;
				}
			}
			else {
				ent->flags |= RenderEffects::Translucent;
				if (f < 13) {
					ent->skinNumber = 5;
				} else {
					ent->skinNumber = 6;
				}
			}
			break;
		case explosion_t::ex_poly2:
			if (f >= ex->frames - 1) {
				ex->type = explosion_t::ex_free;
				break;
			}

			ent->alpha = (5.0 - (float)f) / 5.0;
			ent->skinNumber = 0;
			ent->flags |= RenderEffects::Translucent;
			break;
		default:
			break;
		}

		if (ex->type == explosion_t::ex_free) {
			continue;
		}

		if (vid_rtx->integer) {
			CLG_AddExplosionLight(ex, frac / (ex->frames - 1));
		} else {
			if (ex->light) {
				clge->view->AddLight(ent->origin, ex->lightcolor, ex->light * ent->alpha);
			}
		}

		if (ex->type != explosion_t::ex_light) {
			VectorCopy(ent->origin, ent->oldorigin);

			if (f < 0)
				f = 0;
			ent->frame = ex->baseFrame + f + 1;
			ent->oldframe = ex->baseFrame + f;
			ent->backlerp = 1.0 - (frac - f);

			clge->view->AddRenderEntity(*ent);
		}
	}
}

//
//=============================================================================
//
//	BEAM MANAGEMENT
//
//=============================================================================
//

#define MAX_BEAMS   32

typedef struct {
	int         entity;
	int         dest_entity;
	qhandle_t   model;
	int         endTime;
	vec3_t      offset;
	vec3_t      start, end;
} beam_t;

static beam_t   clg_beams[MAX_BEAMS];
static beam_t   clg_playerbeams[MAX_BEAMS];

static void CLG_ClearBeams(void)
{
	memset(clg_beams, 0, sizeof(clg_beams));
	memset(clg_playerbeams, 0, sizeof(clg_playerbeams));
}

static void CLG_ParseBeam(qhandle_t model)
{
	beam_t* b;
	int     i;

	// override any beam with the same source AND destination entities
	for (i = 0, b = clg_beams; i < MAX_BEAMS; i++, b++)
		if (b->entity == teParameters.entity1 && b->dest_entity == teParameters.entity2)
			goto override;

	// find a free beam
	for (i = 0, b = clg_beams; i < MAX_BEAMS; i++, b++) {
		if (!b->model || b->endTime < cl->time) {
			override :
				b->entity = teParameters.entity1;
			b->dest_entity = teParameters.entity2;
			b->model = model;
			b->endTime = cl->time + 200;
			VectorCopy(teParameters.position1, b->start);
			VectorCopy(teParameters.position2, b->end);
			VectorCopy(teParameters.offset, b->offset);
			return;
		}
	}
}

static void CLG_ParsePlayerBeam(qhandle_t model)
{
	beam_t* b;
	int     i;

	// override any beam with the same entity
	for (i = 0, b = clg_playerbeams; i < MAX_BEAMS; i++, b++) {
		if (b->entity == teParameters.entity1) {
			b->entity = teParameters.entity1;
			b->model = model;
			b->endTime = cl->time + 200;
			VectorCopy(teParameters.position1, b->start);
			VectorCopy(teParameters.position2, b->end);
			VectorCopy(teParameters.offset, b->offset);
			return;
		}
	}

	// find a free beam
	for (i = 0, b = clg_playerbeams; i < MAX_BEAMS; i++, b++) {
		if (!b->model || b->endTime < cl->time) {
			b->entity = teParameters.entity1;
			b->model = model;
			b->endTime = cl->time + 100;     // PMM - this needs to be 100 to prevent multiple heatbeams
			VectorCopy(teParameters.position1, b->start);
			VectorCopy(teParameters.position2, b->end);
			VectorCopy(teParameters.offset, b->offset);
			return;
		}
	}

}

/*
=================
CL_AddBeams
=================
*/
static void CLG_AddBeams(void)
{
	int         i, j;
	beam_t* b;
	vec3_t      dist, org;
	float       d;
	r_entity_t    ent;
	vec3_t      angles;
	float       len, steps;
	float       model_length;

	// update beams
	for (i = 0, b = clg_beams; i < MAX_BEAMS; i++, b++) {
		if (!b->model || b->endTime < cl->time)
			continue;

		// if coming from the player, update the start position
		if (b->entity == cl->frame.clientNumber + 1)
			VectorAdd(cl->playerEntityOrigin, b->offset, org);
		else
			VectorAdd(b->start, b->offset, org);

		// calculate pitch and yaw
		VectorSubtract(b->end, org, dist);
		angles = vec3_euler(dist);

		// add new entities for the beams
		d = VectorNormalize(dist);
		if (b->model == cl_mod_lightning) {
			model_length = 35.0;
			d -= 20.0; // correction so it doesn't end in middle of tesla
		}
		else {
			model_length = 30.0;
		}
		steps = ceil(d / model_length);
		len = (d - model_length) / (steps - 1);

		memset(&ent, 0, sizeof(ent));
		ent.model = b->model;

		// PMM - special case for lightning model .. if the real length is shorter than the model,
		// flip it around & draw it from the end to the start.  This prevents the model from going
		// through the tesla mine (instead it goes through the target)
		if ((b->model == cl_mod_lightning) && (d <= model_length)) {
			VectorCopy(b->end, ent.origin);
			ent.flags = RenderEffects::FullBright;
			ent.angles[0] = angles[0];
			ent.angles[1] = angles[1];
			ent.angles[2] = rand() % 360;
			clge->view->AddRenderEntity(ent);
			return;
		}

		while (d > 0) {
			VectorCopy(org, ent.origin);
			if (b->model == cl_mod_lightning) {
				ent.flags = RenderEffects::FullBright;
				ent.angles[0] = -angles[0];
				ent.angles[1] = angles[1] + 180.0;
				ent.angles[2] = rand() % 360;
			}
			else {
				ent.angles[0] = angles[0];
				ent.angles[1] = angles[1];
				ent.angles[2] = rand() % 360;
			}

			clge->view->AddRenderEntity(ent);

			for (j = 0; j < 3; j++)
				org[j] += dist[j] * len;
			d -= model_length;
		}
	}
}

/*
=================
CL_AddPlayerBeams
Draw player locked beams. Currently only used by the plasma beam.
=================
*/
static void CLG_AddPlayerBeams(void)
{
	int         i, j;
	beam_t* b;
	vec3_t      dist, org;
	float       d;
	r_entity_t    ent;
	vec3_t      angles;
	float       len, steps;
	int         frameNumber;
	float       model_length;
	float       hand_multiplier;
	PlayerState* ps, * ops;

	if (info_hand->integer == 2)
		hand_multiplier = 0;
	else if (info_hand->integer == 1)
		hand_multiplier = -1;
	else
		hand_multiplier = 1;

	// Acquire access to the view camera.
	ViewCamera *viewCamera = clge->view->GetViewCamera();

	// update beams
	for (i = 0, b = clg_playerbeams; i < MAX_BEAMS; i++, b++) {
		if (!b->model || b->endTime < cl->time)
			continue;

		// if coming from the player, update the start position
		if (b->entity == cl->frame.clientNumber + 1) {
			// set up gun position
			ps = &cl->frame.playerState;
			ops = &cl->oldframe.playerState;

			for (j = 0; j < 3; j++)
				b->start[j] = cl->refdef.vieworg[j] + ops->gunOffset[j] +
				cl->lerpFraction * (ps->gunOffset[j] - ops->gunOffset[j]);

			org = vec3_fmaf(b->start, (hand_multiplier * b->offset[0]), viewCamera->GetRightViewVector());
			org = vec3_fmaf(org, b->offset[1], viewCamera->GetForwardViewVector());
			org = vec3_fmaf(org, b->offset[2], viewCamera->GetUpViewVector());
			if (info_hand->integer == 2) {
				org = vec3_fmaf(org, -1, viewCamera->GetUpViewVector());
			}

			// calculate pitch and yaw
			VectorSubtract(b->end, org, dist);

			// FIXME: don't add offset twice?
			d = VectorLength(dist);
			dist = vec3_scale(viewCamera->GetForwardViewVector(), d);
			dist = vec3_fmaf(dist, (hand_multiplier * b->offset[0]), viewCamera->GetRightViewVector());
			dist = vec3_fmaf(dist, b->offset[1], viewCamera->GetForwardViewVector());
			dist = vec3_fmaf(dist, b->offset[2], viewCamera->GetUpViewVector());
			if (info_hand->integer == 2) {
				org = vec3_fmaf(org, -1, viewCamera->GetUpViewVector());
			}
				

			// FIXME: use cl.refdef.viewAngles?
			angles = vec3_euler(dist);

			// if it's the heatbeam, draw the particle effect
			ParticleEffects::HeatBeam(org, dist);

			frameNumber = 1;
		}
		else {
			org = b->start;

			// calculate pitch and yaw
			dist = b->end - org;
			angles = vec3_euler(dist);

			// if it's a non-origin offset, it's a player, so use the hardcoded player offset
			if (!vec3_equal(b->offset, vec3_zero())) {
				vec3_t  tmp, f, r, u;

				tmp[0] = angles[0];
				tmp[1] = angles[1] + 180.0;
				tmp[2] = 0;
				AngleVectors(tmp, &f, &r, &u);

				VectorMA(org, -b->offset[0] + 1, r, org);
				VectorMA(org, -b->offset[1], f, org);
				VectorMA(org, -b->offset[2] - 10, u, org);
			}
			else {
				// if it's a monster, do the particle effect
				ParticleEffects::MonsterPlasmaShell(b->start);
			}

			frameNumber = 2;
		}

		// add new entities for the beams
		d = VectorNormalize(dist);
		model_length = 32.0;
		steps = ceil(d / model_length);
		len = (d - model_length) / (steps - 1);

		memset(&ent, 0, sizeof(ent));
		ent.model = b->model;
		ent.frame = frameNumber;
		ent.flags = RenderEffects::FullBright;
		ent.angles[0] = -angles[0];
		ent.angles[1] = angles[1] + 180.0;
		ent.angles[2] = cl->time % 360;

		while (d > 0) {
			VectorCopy(org, ent.origin);

			clge->view->AddRenderEntity(ent);

			for (j = 0; j < 3; j++)
				org[j] += dist[j] * len;
			d -= model_length;
		}
	}
}

//
//=============================================================================
//
//	SUSTAIN MANAGEMENT
//
//=============================================================================
//

#define MAX_SUSTAINS    32

static cl_sustain_t     clg_sustains[MAX_SUSTAINS];

static void CLG_ClearSustains(void)
{
	memset(clg_sustains, 0, sizeof(clg_sustains));
}

static cl_sustain_t* CLG_AllocSustain(void)
{
	cl_sustain_t* s;
	int             i;

	for (i = 0, s = clg_sustains; i < MAX_SUSTAINS; i++, s++) {
		if (s->id == 0)
			return s;
	}

	return NULL;
}

static void CLG_ProcessSustain(void)
{
	cl_sustain_t* s;
	int             i;

	for (i = 0, s = clg_sustains; i < MAX_SUSTAINS; i++, s++) {
		if (s->id) {
			if ((s->endTime >= cl->time) && (cl->time >= s->nextThinkTime))
				s->Think(s);
			else if (s->endTime < cl->time)
				s->id = 0;
		}
	}
}

void SteamSustainThink(cl_sustain_t* self)
{
    int         i, j;
    cparticle_t* p;
    float       d;
    vec3_t      r, u;
    vec3_t      dir;

    VectorCopy(self->dir, dir);
    MakeNormalVectors(dir, r, u);

    for (i = 0; i < self->count; i++) {
        p = Particles::GetFreeParticle();
        if (!p)
            return;

        p->time = cl->time;
        p->color = self->color + (rand() & 7);

        for (j = 0; j < 3; j++) {
            p->org[j] = self->org[j] + self->magnitude * 0.1 * crand();
        }
        VectorScale(dir, self->magnitude, p->vel);
        d = crand() * self->magnitude / 3;
        VectorMA(p->vel, d, r, p->vel);
        d = crand() * self->magnitude / 3;
        VectorMA(p->vel, d, u, p->vel);

        p->acceleration[0] = p->acceleration[1] = 0;
        p->acceleration[2] = -ParticleEffects::ParticleGravity / 2;
        p->alpha = 1.0;

        p->alphavel = -1.0 / (0.5 + frand() * 0.3);
    }

    self->nextThinkTime += self->thinkinterval;
}

static void CLG_ParseSteam(void)
{
	cl_sustain_t* s;

	if (teParameters.entity1 == -1) {
		ParticleEffects::SteamPuffs(teParameters.position1, teParameters.dir, teParameters.color & 0xff, teParameters.count, teParameters.entity2);
		return;
	}

	s = CLG_AllocSustain();
	if (!s)
		return;

	s->id = teParameters.entity1;
	s->count = teParameters.count;
	VectorCopy(teParameters.position1, s->org);
	VectorCopy(teParameters.dir, s->dir);
	s->color = teParameters.color & 0xff;
	s->magnitude = teParameters.entity2;
	s->endTime = cl->time + teParameters.time;
	s->Think = SteamSustainThink;
	s->thinkinterval = 100;
	s->nextThinkTime = cl->time;
}

//
//=============================================================================
//
//	TEMP ENTITY MANAGEMENT
//
//=============================================================================
//

static void CLG_RailCore(void)
{
	laser_t* l;

	l = CLG_AllocLaser();
	if (!l)
		return;

	VectorCopy(teParameters.position1, l->start);
	VectorCopy(teParameters.position2, l->end);
	l->color = -1;
	l->lifetime = 1000 * cl_railtrail_time->value;
	l->width = cl_railcore_width->integer;
	l->rgba.u32 = railcore_color.u32;
}

static void CLG_RailSpiral(void)
{
	vec3_t      move;
	vec3_t      vec;
	float       len;
	int         j;
	cparticle_t* p;
	vec3_t      right, up;
	int         i;
	float       d, c, s;
	vec3_t      dir;

	VectorCopy(teParameters.position1, move);
	VectorSubtract(teParameters.position2, teParameters.position1, vec);
	len = VectorNormalize(vec);

	MakeNormalVectors(vec, right, up);

	for (i = 0; i < len; i++) {
		p = Particles::GetFreeParticle();
		if (!p)
			return;

		p->time = cl->time;
		VectorClear(p->acceleration);

		d = i * 0.1;
		c = cosf(d);
		s = sinf(d);

		VectorScale(right, c, dir);
		VectorMA(dir, s, up, dir);

		p->alpha = 1.0;
		p->alphavel = -1.0 / (cl_railtrail_time->value + frand() * 0.2);
		p->color = -1;
		p->rgba.u32 = railspiral_color.u32;
		p->brightness = Particles::GetParticleEmissiveFactor();
		for (j = 0; j < 3; j++) {
			p->org[j] = move[j] + dir[j] * cl_railspiral_radius->value;
			p->vel[j] = dir[j] * 6;
		}

		VectorAdd(move, vec, move);
	}
}

static void CLG_RailLights(color_t color)
{
	vec3_t fcolor;
	fcolor[0] = (float)color.u8[0] / 255.f;
	fcolor[1] = (float)color.u8[1] / 255.f;
	fcolor[2] = (float)color.u8[2] / 255.f;

	vec3_t      move;
	vec3_t      vec;
	float       len;

	VectorCopy(teParameters.position1, move);
	VectorSubtract(teParameters.position2, teParameters.position1, vec);
	len = VectorNormalize(vec);

	float num_segments = ceilf(len / 100.f);
	float segment_size = len / num_segments;

	for (float segment = 0; segment < num_segments; segment++)
	{
		float offset = (segment + 0.25f) * segment_size;
		vec3_t pos;
		VectorMA(move, offset, vec, pos);

		cdlight_t* dl = DynamicLights::GetDynamicLight(0);
		VectorScale(fcolor, 0.25f, dl->color);
		VectorCopy(pos, dl->origin);
		dl->radius = 400;
		dl->decay = 400;
		dl->die = cl->time + 1000;
		VectorScale(vec, segment_size * 0.5f, dl->velocity);
	}
}

//extern uint32_t d_8to24table[256];
extern cvar_t* cvar_pt_beam_lights;

static void CLG_RailTrail(void)
{
	color_t rail_color;

	if (!cl_railtrail_type->integer)
	{
		rail_color.u32 = 0xff0000; //d_8to24table[0x74];
	}
	else
	{
		rail_color = railcore_color;

		CLG_RailCore();
		if (cl_railtrail_type->integer > 1) {
			CLG_RailSpiral();
		}
	}

	if (!cl_railtrail_type->integer || cvar_pt_beam_lights->value <= 0)
	{
		CLG_RailLights(rail_color);
	}
}


/**
*	
**/
static void dirtoangles(vec3_t angles)
{
	angles[0] = acosf(teParameters.dir[2]) / M_PI * 180.f;
	if (teParameters.dir[0])
		angles[1] = atan2f(teParameters.dir[1], teParameters.dir[0]) / M_PI * 180.f;
	else if (teParameters.dir[1] > 0)
		angles[1] = 90;
	else if (teParameters.dir[1] < 0)
		angles[1] = 270;
	else
		angles[1] = 0;
}


/**
*	@brief	Parses a temporary entity message and acts accordingly.
**/
static void TE_SpawnGibs(const vec3_t &origin, const vec3_t &size, const vec3_t &velocity, int32_t count) {
    // Throw some gibs around, true horror oh boy.
    ClientGameWorld* gameWorld = GetGameWorld();

	CLG_Print(PrintType::DeveloperWarning, fmt::format( "Throwing Gib: origin({}, {}, {}), size({}, {}, {}), velocity({}, {}, {})\n",
			  origin.x, origin.y, origin.z, size.x, size.y, size.z, velocity.x, velocity.y, velocity.z) );
	// Let's go bonkers!
	int32_t damage = 50;
	gameWorld->ThrowGib(origin, size, velocity, "models/gibs/gibhead.iqm", damage, GibType::Organic);
	damage = 80;
	gameWorld->ThrowGib(origin, size, velocity, "models/gibs/gibtorso.iqm", damage, GibType::Organic);
	damage = 120;
	gameWorld->ThrowGib(origin, size, velocity, "models/gibs/gibarmleft.iqm", damage, GibType::Organic);
	damage = 100;
	gameWorld->ThrowGib(origin, size, velocity, "models/gibs/gibarmright.iqm", damage, GibType::Organic);
	damage = 140;
	gameWorld->ThrowGib(origin, size, velocity, "models/gibs/giblegupleft.iqm", damage, GibType::Organic);
	damage = 130;
	gameWorld->ThrowGib(origin, size, velocity, "models/gibs/giblegupright.iqm", damage, GibType::Organic);
	damage = 70;
	gameWorld->ThrowGib(origin, size, velocity, "models/gibs/gibleglowleft.iqm", damage, GibType::Organic);
	damage = 60;
	gameWorld->ThrowGib(origin, size, velocity, "models/gibs/gibleglowright.iqm", damage, GibType::Organic);

	//for (int32_t i = 0; i < count; i++) {
	//	int32_t damage = 50;
	//    gameWorld->ThrowGib(origin, size, velocity, "models/gibs/gibhead.iqm", damage, GibType::Organic);
	//}
}

/**
*	@brief	Speaks to the actual GameWorld in order to spawn the debris entity.
**/
static void TE_SpawnDebris(GameEntity *geDebrisser, const int32_t debrisModelIndex, const vec3_t &origin, const float speed, const int32_t damage ) {
    // Throw some debris around, madness!
    ClientGameWorld* gameWorld = GetGameWorld();

	// Default.
	std::string debrisModel = "models/objects/debris1/tris.md2";

	if (!debrisModelIndex || debrisModelIndex == 1) {
		debrisModel = "models/objects/debris1/tris.md2";
	} else if (debrisModelIndex == 2) {
		debrisModel = "models/objects/debris2/tris.md2";
	} else if (debrisModelIndex == 3) {
		debrisModel = "models/objects/debris3/tris.md2";
	}
	// Let's go bonkers!
    gameWorld->ThrowDebris(geDebrisser, debrisModel, origin, speed, damage);
}


/**
*	@brief	Parses a temporary entity message and acts accordingly.
**/
static const byte splash_color[] = { 0x00, 0xe0, 0xb0, 0x50, 0xd0, 0xe0, 0xe8 };

void CLG_ParseTempEntity(void)
{
	explosion_t* ex;
	int r;

	ClientGameWorld *gameWorld = GetGameWorld();

	switch (teParameters.type) {

// ---------------------- START OF: Client Side Debris / Explosions / Gibs ---------------------- 
	case TempEntityEvent::BodyGib: {
			GameEntity *geGibber = gameWorld->GetGameEntityByIndex(teParameters.entity1);

			if (geGibber) {
				// We oughta get it from previous frame since by the time it is gibbing, the entity has already "disappeared" in that
				// it has no model set and its state is already gone.
				const vec3_t previousMins = geGibber->GetPODEntity()->previousState.mins;
				const vec3_t previousAbsMins = geGibber->GetPODEntity()->previousState.origin + previousMins;
				const vec3_t gibberSize = geGibber->GetSize();
				const vec3_t gibberVelocity = geGibber->GetVelocity();

				// Calculate the actual start origin for offset spawning the gibs at.
				const vec3_t teOrigin = previousAbsMins + vec3_scale( gibberSize, 0.5f );

				// Spawn the gibs.
				TE_SpawnGibs( teOrigin, gibberSize, gibberVelocity, teParameters.count );
			}
		break;
	}
	case TempEntityEvent::DebrisGib: {
			GameEntity *geDebrisser = gameWorld->GetGameEntityByIndex(teParameters.entity1);

			if (geDebrisser) {
				// Get the 'debrisser' its origin and add our received offset position to that.
				const vec3_t teOrigin = geDebrisser->GetOrigin() + teParameters.position1;
				const int32_t teDebrisModelIndex = teParameters.modelIndex1;
				const float teSpeed = teParameters.speed;
				const float teDamage = teParameters.damage;

				TE_SpawnDebris(geDebrisser, teDebrisModelIndex, teOrigin, teSpeed, teDamage);
			}
		break;
	}
			//teParameters.entity1 = clgi.MSG_ReadUint16(); // Position for Gib spawning.
			//// We add the position1(debris spawn offset origin) to the spawn origin of debrisser entity.
			//teParameters.position1 = clgi.MSG_ReadVector3(true);
			//// Speed will in most cases be a float of 0 to 2. So encode it as an Uint8.
			//teParameters.speed = static_cast<float>(clgi.MSG_ReadUint8()) / 255.f;

// ------------------------------------  EXPLOSIONS  ---------------------------------------//

	case TempEntityEvent::PlainExplosion: {
		GameEntity *geExploder = gameWorld->GetGameEntityByIndex(teParameters.entity1);

		if (geExploder) {
			const vec3_t teOrigin = geExploder->GetOrigin();
			CLG_PlainExplosion(false, teOrigin);
		}
		break;
	}

	case TempEntityEvent::Explosion2: {
		GameEntity *geExploder = gameWorld->GetGameEntityByIndex(teParameters.entity1);

		if (geExploder) {
			const vec3_t teOrigin = geExploder->GetOrigin();
			
			ex = CLG_PlainExplosion(false, teOrigin);
			if (!cl_explosion_sprites->integer)
			{
				ex->frames = 19;
				ex->baseFrame = 30;
			}
			
			ParticleEffects::ExplosionSparks(teOrigin);
			clgi.S_StartSound(&teOrigin, 0, 0, cl_sfx_grenexp, 1, Attenuation::Normal, 0);
		}

		break;
	}
	case TempEntityEvent::Explosion1: {
		GameEntity *geExploder = gameWorld->GetGameEntityByIndex(teParameters.entity1);

		if (geExploder) {
			const vec3_t teOrigin = geExploder->GetOrigin();

			CLG_PlainExplosion(false, teOrigin);
			ParticleEffects::ExplosionSparks(teOrigin);
			clgi.S_StartSound(&teOrigin, 0, 0, cl_sfx_explosion, 1, Attenuation::Normal, 0);
		}
		break;
	}

	case TempEntityEvent::NoParticleExplosion1: {
		GameEntity *geExploder = gameWorld->GetGameEntityByIndex(teParameters.entity1);

		if (geExploder) {
			const vec3_t teOrigin = geExploder->GetOrigin();

			CLG_PlainExplosion(false, teOrigin);
			clgi.S_StartSound(&teOrigin, 0, 0, cl_sfx_explosion, 1, Attenuation::Normal, 0);
		}
		break;
	}

	case TempEntityEvent::BigExplosion1: {
		GameEntity *geExploder = gameWorld->GetGameEntityByIndex(teParameters.entity1);

		if (geExploder) {
			const vec3_t teOrigin = geExploder->GetOrigin();

			ex = CLG_PlainExplosion(true, teOrigin);
			clgi.S_StartSound(&teOrigin, 0, 0, cl_sfx_explosion, 1, Attenuation::Normal, 0);
		}
		break;
	}


// ---------------------- END OF: Client Side Debris / Explosions / Gibs ---------------------- 


	case TempEntityEvent::Blood:          // bullet hitting flesh
		if (!(cl_disable_particles->integer & NOPART_BLOOD))
		{
			// CL_ParticleEffect(teParameters.position1, teParameters.dir, 0xe8, 60);
			ParticleEffects::BloodSplatters(teParameters.position1, teParameters.dir, 0xe8, 1000);
		}
		break;

	case TempEntityEvent::Gunshot:            // bullet hitting wall
		ParticleEffects::DirtAndSparks(teParameters.position1, teParameters.dir, 0, 40);
	case TempEntityEvent::Shotgun:            // bullet hitting wall
		ParticleEffects::DirtAndSparks(teParameters.position1, teParameters.dir, 0, 20);
		CLG_SmokeAndFlash(teParameters.position1);
		break;

	case TempEntityEvent::Sparks:
	case TempEntityEvent::BulletSparks:
		ParticleEffects::DirtAndSparks(teParameters.position1, teParameters.dir, 0xe0, 6);

		if (teParameters.type != TempEntityEvent::Sparks) {
			CLG_SmokeAndFlash(teParameters.position1);

			// impact sound
			r = rand() & 15;
			if (r == 1)
				clgi.S_StartSound(&teParameters.position1, 0, 0, cl_sfx_ric1, 1, Attenuation::Normal, 0);
			else if (r == 2)
				clgi.S_StartSound(&teParameters.position1, 0, 0, cl_sfx_ric2, 1, Attenuation::Normal, 0);
			else if (r == 3)
				clgi.S_StartSound(&teParameters.position1, 0, 0, cl_sfx_ric3, 1, Attenuation::Normal, 0);
		}
		break;

	case TempEntityEvent::Splash:         // bullet hitting water
		if (teParameters.color < 0 || teParameters.color > 6)
			r = 0x00;
		else
			r = splash_color[teParameters.color];
		ParticleEffects::WaterSplash(teParameters.position1, teParameters.dir, r, teParameters.count);

		if (teParameters.color == SplashType::Sparks) {
			r = rand() & 3;
			if (r == 0)
				clgi.S_StartSound(&teParameters.position1, 0, 0, cl_sfx_spark5, 1, Attenuation::Static, 0);
			else if (r == 1)
				clgi.S_StartSound(&teParameters.position1, 0, 0, cl_sfx_spark6, 1, Attenuation::Static, 0);
			else
				clgi.S_StartSound(&teParameters.position1, 0, 0, cl_sfx_spark7, 1, Attenuation::Static, 0);
		}
		break;

	case TempEntityEvent::BubbleTrailA:
		ParticleEffects::BubbleTrailA(teParameters.position1, teParameters.position2);
		break;

	case TempEntityEvent::DebugTrail:
		CLG_DrawDebugLine(teParameters.position1, teParameters.position2);
		break;

	case TempEntityEvent::ForceWall:
		ParticleEffects::ForceWall(teParameters.position1, teParameters.position2, teParameters.color);
		break;

	case TempEntityEvent::BubbleTrailB:
		ParticleEffects::BubbleTrailB(teParameters.position1, teParameters.position2, 8);
		clgi.S_StartSound(&teParameters.position1, 0, 0, cl_sfx_lashit, 1, Attenuation::Normal, 0);
		break;

	case TempEntityEvent::MoreBlood:
		ParticleEffects::DirtAndSparks(teParameters.position1, teParameters.dir, 0xe8, 250);
		break;

	case TempEntityEvent::ElectricSparks:
		ParticleEffects::DirtAndSparks(teParameters.position1, teParameters.dir, 0x75, 40);
		//FIXME : replace or remove this sound
		clgi.S_StartSound(&teParameters.position1, 0, 0, cl_sfx_lashit, 1, Attenuation::Normal, 0);
		break;
	case TempEntityEvent::TeleportEffect: {
		GameEntity *geTeleporter = gameWorld->GetGameEntityByIndex(teParameters.entity1);

		if (geTeleporter) {
			ParticleEffects::Teleporter(geTeleporter->GetOrigin());
		}
		break;
	}

	default:
		CLG_Error( ErrorType::Drop, fmt::format( "{}: bad type", __func__, teParameters.type ) );
	}
}

//
//===============
// CLG_RegisterTEntModels
// 
// Registers all sounds used for temporary entities.
//===============
//
void CLG_RegisterTempEntitySounds(void)
{
	int     i;
	char    name[MAX_QPATH];

	// Register SFX sounds.
	cl_sfx_ric1 = clgi.S_RegisterSound("world/ric1.wav");
	cl_sfx_ric2 = clgi.S_RegisterSound("world/ric2.wav");
	cl_sfx_ric3 = clgi.S_RegisterSound("world/ric3.wav");
	cl_sfx_lashit = clgi.S_RegisterSound("weapons/lashit.wav");
	cl_sfx_flare = clgi.S_RegisterSound("weapons/flare.wav");
	cl_sfx_spark5 = clgi.S_RegisterSound("world/spark5.wav");
	cl_sfx_spark6 = clgi.S_RegisterSound("world/spark6.wav");
	cl_sfx_spark7 = clgi.S_RegisterSound("world/spark7.wav");
	cl_sfx_railg = clgi.S_RegisterSound("weapons/railgf1a.wav");
	cl_sfx_explosion = clgi.S_RegisterSound("weapons/rocklx1a.wav");
	cl_sfx_grenexp = clgi.S_RegisterSound("weapons/grenlx1a.wav");
	cl_sfx_water_explosion = clgi.S_RegisterSound("weapons/xpld_wat.wav");

	// Register Player sounds.
	clgi.S_RegisterSound("player/land1.wav");
	clgi.S_RegisterSound("player/fall2.wav");
	clgi.S_RegisterSound("player/fall1.wav");

	// Register Footstep sounds.
	for (i = 0; i < 4; i++) {
		Q_snprintf(name, sizeof(name), "player/step%i.wav", i + 1);
		cl_sfx_footsteps[i] = clgi.S_RegisterSound(name);
	}

	cl_sfx_lightning = clgi.S_RegisterSound("weapons/tesla.wav");
	cl_sfx_disrexp = clgi.S_RegisterSound("weapons/disrupthit.wav");
}

//
//===============
// CLG_RegisterTempEntityModels
// 
// Registers all models used for temporary entities.
//===============
//
void CLG_RegisterTempEntityModels(void)
{
	// Register FX models.
	cl_mod_explode = clgi.R_RegisterModel("models/objects/explode/tris.md2");
	cl_mod_smoke = clgi.R_RegisterModel("models/objects/smoke/tris.md2");
	cl_mod_flash = clgi.R_RegisterModel("models/objects/flash/tris.md2");
	cl_mod_parasite_segment = clgi.R_RegisterModel("models/monsters/parasite/segment/tris.md2");
	cl_mod_grapple_cable = clgi.R_RegisterModel("models/ctf/segment/tris.md2");
	cl_mod_explo4 = clgi.R_RegisterModel("models/objects/r_explode/tris.md2");
	cl_mod_explosions[0] = clgi.R_RegisterModel("sprites/rocket_0.sp2");
	cl_mod_explosions[1] = clgi.R_RegisterModel("sprites/rocket_1.sp2");
	cl_mod_explosions[2] = clgi.R_RegisterModel("sprites/rocket_5.sp2");
	cl_mod_explosions[3] = clgi.R_RegisterModel("sprites/rocket_6.sp2");
	cl_mod_laser = clgi.R_RegisterModel("models/objects/laser/tris.md2");

	cl_mod_lightning = clgi.R_RegisterModel("models/proj/lightning/tris.md2");
	cl_mod_heatbeam = clgi.R_RegisterModel("models/proj/beam/tris.md2");
	cl_mod_explo4_big = clgi.R_RegisterModel("models/objects/r_explode2/tris.md2");

	//
	// Configure certain models to be a vertical spriteParameters.
	//
	model_t* model = clgi.CL_Model_GetModelByHandle(cl_mod_explode);
	if (model)
		model->sprite_vertical = true;

	model = clgi.CL_Model_GetModelByHandle(cl_mod_smoke);
	if (model)
		model->sprite_vertical = true;

	model = clgi.CL_Model_GetModelByHandle(cl_mod_flash);
	if (model)
		model->sprite_vertical = true;

	model = clgi.CL_Model_GetModelByHandle(cl_mod_parasite_segment);
	if (model)
		model->sprite_vertical = true;

	model = clgi.CL_Model_GetModelByHandle(cl_mod_grapple_cable);
	if (model)
		model->sprite_vertical = true;

	model = clgi.CL_Model_GetModelByHandle(cl_mod_explo4);
	if (model)
		model->sprite_vertical = true;

	model = clgi.CL_Model_GetModelByHandle(cl_mod_laser);
	if (model)
		model->sprite_vertical = true;

	model = clgi.CL_Model_GetModelByHandle(cl_mod_lightning);
	if (model)
		model->sprite_vertical = true;

	model = clgi.CL_Model_GetModelByHandle(cl_mod_heatbeam);
	if (model)
		model->sprite_vertical = true;

	model = clgi.CL_Model_GetModelByHandle(cl_mod_explo4_big);
	if (model)
		model->sprite_vertical = true;
}

//
//===============
// CLG_AddTempEntities
// 
// Adds all temporal entities to the current frame scene.
//===============
//
void CLG_AddTempEntities(void)
{
	CLG_AddBeams();
	CLG_AddPlayerBeams();
	CLG_AddExplosions();
	CLG_ProcessSustain();
	CLG_AddLasers();
}

//
//===============
// CLG_ClearTempEntities
// 
// Clear the current temporary entities.
//===============
//
void CLG_ClearTempEntities(void)
{
	CLG_ClearBeams();
	CLG_ClearExplosions();
	CLG_ClearLasers();
	CLG_ClearSustains();
}

//
//===============
// CLG_InitTempEntities
// 
// Initialize temporary entity CVars.
//===============
//
void CLG_InitTempEntities(void)
{
	cl_railtrail_type = clgi.Cvar_Get("cl_railtrail_type", "0", 0);
	cl_railtrail_time = clgi.Cvar_Get("cl_railtrail_time", "1.0", 0);
	cl_railcore_color = clgi.Cvar_Get("cl_railcore_color", "red", 0);
	cl_railcore_color->changed = cl_railcore_color_changed;
	//cl_railcore_color->generator = Com_Color_g;
	cl_railcore_color_changed(cl_railcore_color);
	cl_railcore_width = clgi.Cvar_Get("cl_railcore_width", "2", 0);
	cl_railspiral_color = clgi.Cvar_Get("cl_railspiral_color", "orange", 0);
	cl_railspiral_color->changed = cl_railspiral_color_changed;
	//cl_railspiral_color->generator = Com_Color_g;
	cl_railspiral_color_changed(cl_railspiral_color);
	cl_railspiral_radius = clgi.Cvar_Get("cl_railspiral_radius", "3", 0);
	
	// 
	cvar_pt_beam_lights = clgi.Cvar_Get("pt_beam_lights", NULL, 0);
}