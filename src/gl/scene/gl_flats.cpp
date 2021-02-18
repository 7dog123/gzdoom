/*
** gl_flat.cpp
** Flat rendering
**
**---------------------------------------------------------------------------
** Copyright 2000-2005 Christoph Oelckers
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
** 4. When not used as part of GZDoom or a GZDoom derivative, this code will be
**    covered by the terms of the GNU Lesser General Public License as published
**    by the Free Software Foundation; either version 2.1 of the License, or (at
**    your option) any later version.
** 5. Full disclosure of the entire project's source code, except for third
**    party libraries is mandatory. (NOTE: This clause is non-negotiable!)
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

#include "gl/system/gl_system.h"
#include "a_sharedglobal.h"
#include "r_defs.h"
#include "r_sky.h"
#include "r_utility.h"
#include "g_level.h"
#include "doomstat.h"
#include "d_player.h"

#include "gl/system/gl_interface.h"
#include "gl/system/gl_cvars.h"
#include "gl/renderer/gl_renderer.h"
#include "gl/renderer/gl_lightdata.h"
#include "gl/renderer/gl_renderstate.h"
#include "gl/data/gl_data.h"
#include "gl/data/gl_vertexbuffer.h"
#include "gl/dynlights/gl_dynlight.h"
#include "gl/dynlights/gl_glow.h"
#include "gl/dynlights/gl_lightbuffer.h"
#include "gl/scene/gl_drawinfo.h"
#include "gl/shaders/gl_shader.h"
#include "gl/textures/gl_material.h"
#include "gl/utility/gl_clock.h"
#include "gl/utility/gl_convert.h"
#include "gl/utility/gl_templates.h"
#include "gl/textures/gl_combiners.h"//[GEC]

#ifdef _DEBUG
CVAR(Int, gl_breaksec, -1, 0)
#endif

//==========================================================================
//
// [GEC]
//
//==========================================================================
static GLuint SoftLightFlat = -1;
static int lastenvcolor = 0;
static int lastlightlevel = 0;

static unsigned int generate_softlightflat(int color, int distan)
{
    int i;
	int W = 1;
	int H = 400;
	byte *buffer;
	int li = 0;
	int l = 0;
	int k = 0;
	byte lightpalette[400];

	if(lastenvcolor == color) 
	{
		glBindTexture( GL_TEXTURE_2D, SoftLightFlat);
        return SoftLightFlat;
    }

	glDeleteTextures(1, &SoftLightFlat);

    lastenvcolor    = color;

	buffer = (byte *)calloc(sizeof(byte),(W*H));
	memset(buffer, 0xff, sizeof(byte) * (W*H));

	li = 200;
	for (i = 0;i < 200; i++)
	{
		lightpalette[i] = PSXDoomLightingEquation(color/255.0, li, 1);
		li--;
	}
	li = 0;
	for (i = 200;i < 400; i++)//H
	{
		lightpalette[i] = PSXDoomLightingEquation(color/255.0, li, 1);
		li++;
	}

	int count = 0;
	for(i = 0; i < H; i++)
	{
		buffer[count] = (unsigned char)(lightpalette[i] & 0xff);
		count++;
	}

	if(SoftLightFlat == 0)
		glGenTextures(1, &SoftLightFlat);

    glBindTexture( GL_TEXTURE_2D, SoftLightFlat);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, W, H, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, buffer);

	free (buffer);
	return SoftLightFlat;
}

Vector nearPt2, up, right, t1;
float scale;
float Dist;

void GLFlat::GetLightPSX()
{
	Plane p;

	player_t *player = &players[consoleplayer];
	fixed_t planeh = plane.plane.ZatPoint(player->camera);//[GEC]
	p.Set(plane.plane);

	if (((planeh<viewz && ceiling) || (planeh>viewz && !ceiling)))
	{
		return;
	}

	Vector fn, pos;

	float x = FIXED2FLOAT(viewx);
	float y = FIXED2FLOAT(viewy);
	float z = FIXED2FLOAT(viewz);
	z = FIXED2FLOAT(planeh);

	float dist = fabsf(p.DistToPoint(x, z, y));
	Dist = fabsf(p.DistToPoint(x, FIXED2FLOAT(viewz), y));
	float radius = (200);//Floor

	if (radius <= 0.f) return;
	if (dist > radius) return;
	if (p.PointOnSide(x, z, y))
	{
		return;
	}

	scale = 1.0f / ((2.f * radius) - dist);

	pos.Set(x,z,y);
	fn=p.Normal();
	fn.GetRightUp(right, up);

	#ifdef _MSC_VER
		nearPt2 = pos + fn * dist;
	#else
		Vector tmpVec = fn * dist;
		nearPt = pos + tmpVec;
	#endif

	float angle;
	angle = ANGLE_TO_FLOAT(viewangle);

	// Render the light
	right.SetX(1.0);
	right.SetY(0.0);
	right.SetZ(0.0);
	up.SetX(0.0);
	up.SetY(0.0);
	up.SetZ(-1.0);
}

int SetTexNum = GL_TEXTURE0;
//==========================================================================
//
// [GEC] Aply blends and PSX LIGHT
//
//==========================================================================

void GLFlat::SetSpecials(int pass, int light, int rellight, int lightlevel64)//[GEC]
{
	/*if(pass == GLPASS_TEXTURE)
	{
		light = 0;
		rellight = 0;
	}*/

	int texturemode;//[GEC]
	gl_RenderState.GetTextureMode(&texturemode);

	//Pass 1: Light64
	if(pass == GLPASS_ALL || pass == GLPASS_PLAIN || pass == GLPASS_TEXTURE)
	{
		if(!gl_fixedcolormap)
			gl_RenderState.SetLight64(lightlevel64);//[GEC]
		else
			gl_RenderState.SetLight64(0);//[GEC]
	}

	//Pass 2: Blend Psx if true
	if(pass == GLPASS_ALL || pass == GLPASS_PLAIN || pass == GLPASS_TEXTURE)
	{
		if(psx_blend)
		{
			gl_RenderState.SetBlendColor(flashcolor, 0);
		}
	}

	//Pass 4: Light Software
	if(pass == GLPASS_ALL || pass == GLPASS_PLAIN || pass == GLPASS_BASE_MASKED)
	{
		if(glset.lightmode == 16 && (!gl_fixedcolormap))
			gl_RenderState.SetLightMode(2);//[GEC]
	}

	//Pass 5: Blend D64 if false
	if(pass == GLPASS_ALL || pass == GLPASS_PLAIN)
	if(!psx_blend)
	{
		gl_RenderState.SetBlendColor(flashcolor, 1);
	}

	//Pass 6: SetSuperBright
	if(pass == GLPASS_ALL || pass == GLPASS_PLAIN || pass == GLPASS_TEXTURE)
	{
		if(glset.lightmode == 16 && (!gl_fixedcolormap))
			gl_RenderState.SetPsxBrightLevel(255);//[GEC]
	}

	gl_RenderState.Apply();//[GEC]

	int texnum = 0;
	if ((gl.shadermodel < 4))
	{
		//Pass 0: Base Textrue
		SetTextureUnit(0, true);

		texnum ++;
		gltexture->Bind(Colormap.colormap,0,0,0,texnum);
		SetTextureUnit(texnum, true);
		SetTextureMode(texturemode);

		//Pass 1: Light64
		if(pass == GLPASS_PLAIN || pass == GLPASS_TEXTURE)
		{
			texnum ++;
			gltexture->Bind(Colormap.colormap,0,0,0,texnum);
			SetTextureUnit(texnum, true);
			if(!gl_fixedcolormap)
				SetLight64(lightlevel64);
			else
				SetLight64(0);
		}

		//Pass 2: Blend Psx if true
		if(pass == GLPASS_PLAIN || pass == GLPASS_TEXTURE)
		{
			if(psx_blend)
			{
				texnum ++;
				gltexture->Bind(Colormap.colormap,0,0,0,texnum);
				SetTextureUnit(texnum, true);
				SetBlend(flashcolor, GL_INTERPOLATE);
				if(APART(flashcolor) == 0)(SetBlend(flashcolor, GL_ADD));
			}
		}

		//Pass 3: SetColor
		if(pass == GLPASS_PLAIN || pass == GLPASS_TEXTURE || pass == GLPASS_BASE_MASKED)//|| (pass == GLPASS_BASE_MASKED && glset.lightmode == 16)
		{
			texnum ++;
			gltexture->Bind(Colormap.colormap,0,0,0,texnum);
			SetTextureUnit(texnum, true);
			SetCombineColor();
		}

		//Pass 4: Light Software
		if(pass == GLPASS_TEXTURE)
		{
			texnum ++;
			gltexture->Bind(Colormap.colormap,0,0,0,texnum);
			SetTextureUnit(texnum, true);
			SetColor(0xff, 0xff, 0xff, 0xff);
		}

		if(pass == GLPASS_PLAIN || pass == GLPASS_BASE_MASKED)
		if(glset.lightmode == 16)
		{
			texnum ++;
			SetTexNum = GL_TEXTURE0+texnum;
			Enable_texunit(texnum);
			int newlight = gl_CalcLightLevel(light, rellight, false);
			if(!gl_fixedcolormap)
			{
				//glBindTexture(GL_TEXTURE_2D, generate_softlight(newlight, 0));
				generate_softlightflat(newlight, 0);
				GetLightPSX();
				glMatrixMode(GL_TEXTURE);
				glLoadIdentity();
				glTranslatef(0.5,0.5,0.0);
				glRotatef(ANGLE_TO_FLOAT(viewangle)-90.0,0.0,0.0,1.0);
				glTranslatef(-0.5,-0.5,0.0);
				glMatrixMode(GL_PROJECTION);

				glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
				glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);   //Modulate RGB with RGB
				glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PREVIOUS);
				glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
				glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_TEXTURE0 + texnum);
				glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
			}
			else
			{
				SetTexNum = GL_TEXTURE0;
				gltexture->Bind(Colormap.colormap,0,0,0,texnum);
				SetTextureUnit(texnum, true);
				SetColor(0xff, 0xff, 0xff, 0xff);
			}
		}

		//Pass 5: Blend D64 if false
		if(pass == GLPASS_TEXTURE)
		{
			texnum ++;
			gltexture->Bind(Colormap.colormap,0,0,0,texnum);
			SetTextureUnit(texnum, true);
			SetColor(0xff, 0xff, 0xff, 0xff);
		}

		if(pass == GLPASS_PLAIN)
		{
			if(!psx_blend)
			{
				texnum ++;
				gltexture->Bind(Colormap.colormap,0,0,0,texnum);
				SetTextureUnit(texnum, true);
				SetBlend(flashcolor, GL_ADD);
			}
		}

		//Pass 6: SetSuperBright
		if(pass == GLPASS_PLAIN || pass == GLPASS_TEXTURE)
		{
			if(glset.lightmode == 16)
			{
				if (!gl_fixedcolormap)
				{
					texnum ++;
					gltexture->Bind(Colormap.colormap,0,0,0,texnum);
					SetTextureUnit(texnum, true);
					SetPSXSuperBright(255);
				}
			}
		}

		SetCombineAlpha();
		SetTextureUnit(0, true);
	}
}

//==========================================================================
//
// Sets the texture matrix according to the plane's texture positioning
// information
//
//==========================================================================

bool gl_SetPlaneTextureRotation(const GLSectorPlane * secplane, FMaterial * gltexture)
{
	// only manipulate the texture matrix if needed.
	if (secplane->xoffs != 0 || secplane->yoffs != 0 ||
		secplane->xscale != FRACUNIT || secplane->yscale != FRACUNIT ||
		secplane->angle != 0 || 
		gltexture->TextureWidth(GLUSE_TEXTURE) != 64 ||
		gltexture->TextureHeight(GLUSE_TEXTURE) != 64)
	{
		float uoffs=FIXED2FLOAT(secplane->xoffs)/gltexture->TextureWidth(GLUSE_TEXTURE);
		float voffs=FIXED2FLOAT(secplane->yoffs)/gltexture->TextureHeight(GLUSE_TEXTURE);

		float xscale1=FIXED2FLOAT(secplane->xscale);
		float yscale1=FIXED2FLOAT(secplane->yscale);
		if (gltexture->tex->bHasCanvas)
		{
			yscale1 = 0 - yscale1;
		}
		float angle=-ANGLE_TO_FLOAT(secplane->angle);

		float xscale2=64.f/gltexture->TextureWidth(GLUSE_TEXTURE);
		float yscale2=64.f/gltexture->TextureHeight(GLUSE_TEXTURE);

		glMatrixMode(GL_TEXTURE);
		glPushMatrix();
		glScalef(xscale1 ,yscale1,1.0f);
		glTranslatef(uoffs,voffs,0.0f);
		glScalef(xscale2 ,yscale2,1.0f);
		glRotatef(angle,0.0f,0.0f,1.0f);
		return true;
	}
	return false;
}


//==========================================================================
//
// Flats 
//
//==========================================================================

void GLFlat::DrawSubsectorLights(subsector_t * sub, int pass)
{
	Plane p;
	Vector nearPt, up, right, t1;
	float scale;
	unsigned int k;
	seg_t *v;

	FLightNode * node = sub->lighthead[pass==GLPASS_LIGHT_ADDITIVE];
	gl_RenderState.Apply();
	while (node)
	{
		ADynamicLight * light = node->lightsource;
		
		if (light->flags2&MF2_DORMANT)
		{
			node=node->nextLight;
			continue;
		}
		iter_dlightf++;

		// we must do the side check here because gl_SetupLight needs the correct plane orientation
		// which we don't have for Legacy-style 3D-floors
		fixed_t planeh = plane.plane.ZatPoint(light);
		if (gl_lights_checkside && ((planeh<light->Z() && ceiling) || (planeh>light->Z() && !ceiling)))
		{
			node=node->nextLight;
			continue;
		}

		p.Set(plane.plane);
		if (!gl_SetupLight(p, light, nearPt, up, right, scale, Colormap.colormap, false, foggy)) 
		{
			node=node->nextLight;
			continue;
		}
		draw_dlightf++;

		//[GEC]
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		// Render the light
		glBegin(GL_TRIANGLE_FAN);
		for(k = 0, v = sub->firstline; k < sub->numlines; k++, v++)
		{
			vertex_t *vt = v->v1;
			float zc = plane.plane.ZatPoint(vt->fx, vt->fy) + dz;

			t1.Set(vt->fx, zc, vt->fy);
			Vector nearToVert = t1 - nearPt;
			glTexCoord2f( (nearToVert.Dot(right) * scale) + 0.5f, (nearToVert.Dot(up) * scale) + 0.5f);

			glVertex3f(vt->fx, zc, vt->fy);
		}

		glEnd();
		node = node->nextLight;
	}
}


//==========================================================================
//
// Flats 
//
//==========================================================================
extern FDynLightData lightdata;

bool GLFlat::SetupSubsectorLights(bool lightsapplied, subsector_t * sub)
{
	Plane p;

	lightdata.Clear();
	for(int i=0;i<2;i++)
	{
		FLightNode * node = sub->lighthead[i];
		while (node)
		{
			ADynamicLight * light = node->lightsource;
			
			if (light->flags2&MF2_DORMANT)
			{
				node=node->nextLight;
				continue;
			}
			iter_dlightf++;

			// we must do the side check here because gl_SetupLight needs the correct plane orientation
			// which we don't have for Legacy-style 3D-floors
			fixed_t planeh = plane.plane.ZatPoint(light->X(), light->Y());
			if (gl_lights_checkside && ((planeh<light->Z() && ceiling) || (planeh>light->Z() && !ceiling)))
			{
				node=node->nextLight;
				continue;
			}

			p.Set(plane.plane);
			gl_GetLight(p, light, Colormap.colormap, false, false, lightdata);
			node = node->nextLight;
		}
	}

	int numlights[3];

	lightdata.Combine(numlights, gl.MaxLights());
	if (numlights[2] > 0)
	{
		draw_dlightf+=numlights[2]/2;
		gl_RenderState.EnableLight(true);
		gl_RenderState.SetLights(numlights, &lightdata.arrays[0][0]);
		gl_RenderState.Apply();
		return true;
	}
	if (lightsapplied) 
	{
		gl_RenderState.EnableLight(false);
		gl_RenderState.Apply();
	}
	return false;
}

//==========================================================================
//
//
//
//==========================================================================

void GLFlat::DrawSubsector(subsector_t * sub)
{
	/*glBegin(GL_TRIANGLE_FAN);

	for(unsigned int k=0; k<sub->numlines; k++)
	{
		vertex_t *vt = sub->firstline[k].v1;
		glTexCoord2f(vt->fx/64.f, -vt->fy/64.f);
		float zc = plane.plane.ZatPoint(vt->fx, vt->fy) + dz;
		if (renderflags & SSRF_WATER2) {zc += 0.5;}//[GEC]
		glVertex3f(vt->fx, zc, vt->fy);
	}
	glEnd();

	flatvertices += sub->numlines;
	flatprimitives++;*/

	glBegin(GL_TRIANGLE_FAN);

	for(unsigned int k=0; k<sub->numlines; k++)
	{
		vertex_t *vt = sub->firstline[k].v1;
		//glTexCoord2f(vt->fx/64.f, -vt->fy/64.f);
		float zc = plane.plane.ZatPoint(vt->fx, vt->fy) + dz;
		glMultiTexCoord2f(GL_TEXTURE0, vt->fx/64.f, -vt->fy/64.f);
		if (renderflags & SSRF_WATER2) {zc += 0.5;}//[GEC]

		if(SetTexNum != GL_TEXTURE0)
		{
			t1.Set(vt->fx, zc, vt->fy);
			Vector nearToVert = t1 - nearPt2;
			glMultiTexCoord2f(SetTexNum, (nearToVert.Dot(right) * scale) + 0.5f, (nearToVert.Dot(up) * scale) + 0.5f);
		}
		glVertex3f(vt->fx, zc, vt->fy);
	}
	glEnd();
}

//==========================================================================
//
//
//
//==========================================================================

void GLFlat::DrawSubsectors(int pass, bool istrans)
{
	bool lightsapplied = false;

	gl_RenderState.Apply();
	if (sub)
	{
		// This represents a single subsector
		if (pass == GLPASS_ALL) lightsapplied = SetupSubsectorLights(lightsapplied, sub);
		DrawSubsector(sub);
	}
	else
	{
		if (vboindex >= 0)
		{
			//glColor3f( 1.f,.5f,.5f);
			int index = vboindex;
			for (int i=0; i<sector->subsectorcount; i++)
			{
				subsector_t * sub = sector->subsectors[i];
				// This is just a quick hack to make translucent 3D floors and portals work.
				if (gl_drawinfo->ss_renderflags[sub-subsectors]&renderflags || istrans)
				{
					if (pass == GLPASS_ALL) lightsapplied = SetupSubsectorLights(lightsapplied, sub);
					glDrawArrays(GL_TRIANGLE_FAN, index, sub->numlines);
					flatvertices += sub->numlines;
					flatprimitives++;
				}
				index += sub->numlines;
			}
		}
		else
		{
			//glColor3f( .5f,1.f,.5f); // these are for testing the VBO stuff.
			// Draw the subsectors belonging to this sector
			for (int i=0; i<sector->subsectorcount; i++)
			{
				subsector_t * sub = sector->subsectors[i];
				if (gl_drawinfo->ss_renderflags[sub-subsectors]&renderflags || istrans)
				{
					if (pass == GLPASS_ALL) lightsapplied = SetupSubsectorLights(lightsapplied, sub);
					DrawSubsector(sub);
				}
			}
		}

		// Draw the subsectors assigned to it due to missing textures
		if (!(renderflags&SSRF_RENDER3DPLANES))
		{
			gl_subsectorrendernode * node = (renderflags&SSRF_RENDERFLOOR)?
				gl_drawinfo->GetOtherFloorPlanes(sector->sectornum) :
				gl_drawinfo->GetOtherCeilingPlanes(sector->sectornum);

			while (node)
			{
				if (pass == GLPASS_ALL) lightsapplied = SetupSubsectorLights(lightsapplied, node->sub);
				DrawSubsector(node->sub);
				node = node->next;
			}
		}
	}
	gl_RenderState.EnableLight(false);
}


//==========================================================================
//
//
//
//==========================================================================
void GLFlat::Draw(int pass)
{
	int i;
	int rel = getExtraLight();

	int texturemode;//[GEC]
	gl_RenderState.ResetSpecials();//[GEC]
	gl_RenderState.Apply();//[GEC]

#ifdef _DEBUG
	if (sector->sectornum == gl_breaksec)
	{
		int a = 0;
	}
#endif

	if (pass == GLPASS_PLAIN || pass == GLPASS_ALL || pass == GLPASS_TRANSLUCENT || pass == GLPASS_BASE_MASKED || pass == GLPASS_TEXTURE)
	{
		// water layer 1 //[GEC]
		if (renderflags & SSRF_WATER1) { plane.xoffs += (scrollfrac >> 6) * gltexture->TextureWidth(GLUSE_TEXTURE); }

		// water layer 2 //[GEC]
		if (renderflags & SSRF_WATER2) { plane.yoffs -= (scrollfrac >> 6) * gltexture->TextureHeight(GLUSE_TEXTURE); }
	}

	switch (pass)
	{
	case GLPASS_BASE:
		gl_SetColor(lightlevel, rel, &Colormap,1.0f);
		if (!foggy) gl_SetFog(lightlevel, rel, &Colormap, false);
		DrawSubsectors(pass, false);
		break;

	case GLPASS_PLAIN:			// Single-pass rendering
	case GLPASS_ALL:
	case GLPASS_BASE_MASKED:
		gl_SetColor(lightlevel, rel, &Colormap,1.0f);
		if (!foggy || pass != GLPASS_BASE_MASKED) gl_SetFog(lightlevel, rel, &Colormap, false);
		// fall through
	case GLPASS_TEXTURE:
	{
		//[GEC]
		
		//SetInitSpecials(texturemode, glset.lightmode, lightlevel, 0, sector->lightlevel_64, false, FragCol, DynCol);

		gltexture->Bind(Colormap.colormap);
		SetSpecials(pass, lightlevel, rel, sector->lightlevel_64);//[GEC]
		//SetNewSpecials(gltexture, Colormap.colormap, false, false, true, false, pass);//[GEC]

		bool pushed = gl_SetPlaneTextureRotation(&plane, gltexture);
		DrawSubsectors(pass, false);
		if (pushed) 
		{
			glPopMatrix();
			glMatrixMode(GL_MODELVIEW);
		}

		//[GEC] Reset default
		Disable_texunits();
		gl_RenderState.GetTextureMode(&texturemode);
		gl_RenderState.SetTextureMode(texturemode);
		break;
	}

	case GLPASS_TEXBlEND:
	{
		if (flashcolor != 0)
		{
			if (!psx_blend)
			{
				gl_RenderState.SetTextureMode(TM_MASK);
				gl_RenderState.ResetSpecials();//[GEC]
				if (gl.shadermodel < 4)//[GEC] NO usa Fog en Shader mode
				{
					gl_SetFog(lightlevel, rel, &Colormap, true);
				}

				PalEntry Flashcolor = flashcolor;
				if(glset.lightmode == 16)
				{
					Flashcolor.r = clamp(Flashcolor.r * 2, 0, 255);
					Flashcolor.g = clamp(Flashcolor.g * 2, 0, 255);
					Flashcolor.b = clamp(Flashcolor.b * 2, 0, 255);
					Flashcolor.a = clamp(Flashcolor.a * 2, 0, 255);
				}

				glColor4f((Flashcolor.r / 255.0f), (Flashcolor.g / 255.0f), (Flashcolor.b / 255.0f), 1.0);
				gltexture->Bind(Colormap.colormap);
				bool pushed = gl_SetPlaneTextureRotation(&plane, gltexture);
				DrawSubsectors(pass, false);
				if (pushed) 
				{
					glPopMatrix();
					glMatrixMode(GL_MODELVIEW);
				}
			}
		}
		break;
	}

	case GLPASS_LIGHT:
	case GLPASS_LIGHT_ADDITIVE:

		if (!foggy)	gl_SetFog((255+lightlevel)>>1, 0, &Colormap, false);
		else gl_SetFog(lightlevel, 0, &Colormap, true);	

		if (sub)
		{
			DrawSubsectorLights(sub, pass);
		}
		else
		{
			// Draw the subsectors belonging to this sector
			for (i=0; i<sector->subsectorcount; i++)
			{
				subsector_t * sub = sector->subsectors[i];

				if (gl_drawinfo->ss_renderflags[sub-subsectors]&renderflags)
				{
					DrawSubsectorLights(sub, pass);
				}
			}

			// Draw the subsectors assigned to it due to missing textures
			if (!(renderflags&SSRF_RENDER3DPLANES))
			{
				gl_subsectorrendernode * node = (renderflags&SSRF_RENDERFLOOR)?
					gl_drawinfo->GetOtherFloorPlanes(sector->sectornum) :
					gl_drawinfo->GetOtherCeilingPlanes(sector->sectornum);

				while (node)
				{
					DrawSubsectorLights(node->sub, pass);
					node = node->next;
				}
			}
		}
		break;

	case GLPASS_TRANSLUCENT:
		if (renderstyle==STYLE_Add) gl_RenderState.BlendFunc(GL_SRC_ALPHA, GL_ONE);
		gl_SetColor(lightlevel, rel, &Colormap, alpha);
		gl_SetFog(lightlevel, rel, &Colormap, false);
		gl_RenderState.AlphaFunc(GL_GEQUAL,gl_mask_threshold*(alpha));
		if (!gltexture)	
		{
			gl_RenderState.EnableTexture(false);
			DrawSubsectors(pass, true);
			gl_RenderState.EnableTexture(true);
		}
		else 
		{
			if (foggy) gl_RenderState.EnableBrightmap(false);
			gltexture->Bind(Colormap.colormap);
			SetSpecials(GLPASS_PLAIN, lightlevel, rel, sector->lightlevel_64);//[GEC]

			bool pushed = gl_SetPlaneTextureRotation(&plane, gltexture);
			DrawSubsectors(pass, true);
			gl_RenderState.EnableBrightmap(true);
			if (pushed)
			{
				glPopMatrix();
				glMatrixMode(GL_MODELVIEW);
			}

			//[GEC] Reset default
			Disable_texunits();
			gl_RenderState.GetTextureMode(&texturemode);
			gl_RenderState.SetTextureMode(texturemode);
		}
		if (renderstyle==STYLE_Add) gl_RenderState.BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		break;
	}
}


//==========================================================================
//
// GLFlat::PutFlat
//
// Checks texture, lighting and translucency settings and puts this
// plane in the appropriate render list.
//
//==========================================================================
inline void GLFlat::PutFlat(bool fog)
{
	int list;

	if (gl_fixedcolormap) 
	{
		Colormap.GetFixedColormap();
	}
	if (renderstyle!=STYLE_Translucent || alpha < 1.f - FLT_EPSILON || fog)
	{
		int list = (renderflags&SSRF_RENDER3DPLANES) ? GLDL_TRANSLUCENT : GLDL_TRANSLUCENTBORDER;
		gl_drawinfo->drawlists[list].AddFlat (this);
	}
	else if (gltexture != NULL)
	{
		static DrawListType list_indices[2][2][2]={
			{ { GLDL_PLAIN, GLDL_FOG      }, { GLDL_MASKED,      GLDL_FOGMASKED      } },
			{ { GLDL_LIGHT, GLDL_LIGHTFOG }, { GLDL_LIGHTMASKED, GLDL_LIGHTFOGMASKED } }
		};

		bool light = gl_forcemultipass;
		bool masked = gltexture->isMasked() && ((renderflags&SSRF_RENDER3DPLANES) || stack);

		if (!gl_fixedcolormap)
		{
			foggy = gl_CheckFog(&Colormap, lightlevel) || level.flags&LEVEL_HASFADETABLE;

			if (gl_lights && !gl_dynlight_shader && GLRenderer->mLightCount)	// Are lights touching this sector?
			{
				for(int i=0;i<sector->subsectorcount;i++) if (sector->subsectors[i]->lighthead[0] != NULL)
				{
					light=true;
				}
			}
		}
		else foggy = false;

		list = list_indices[light][masked][foggy];
		if (list == GLDL_LIGHT && gltexture->tex->gl_info.Brightmap && gl_BrightmapsActive()) list = GLDL_LIGHTBRIGHT;

		gl_drawinfo->drawlists[list].AddFlat (this);
	}
}

//==========================================================================
//
// This draws one flat 
// The passed sector does not indicate the area which is rendered. 
// It is only used as source for the plane data.
// The whichplane boolean indicates if the flat is a floor(false) or a ceiling(true)
//
//==========================================================================

void GLFlat::Process(sector_t * model, int whichplane, bool fog, int nexttexture)
{
	plane.GetFromSector(model, whichplane);

	if (!fog)
	{
		if (plane.texture==skyflatnum) return;

		gltexture=FMaterial::ValidateTexture(plane.texture + nexttexture, true);//[GEC]
		if (!gltexture) return;
		if (gltexture->tex->isFullbright()) 
		{
			Colormap.LightColor.r = Colormap.LightColor.g = Colormap.LightColor.b = 0xff;
			lightlevel=255;
		}
	}
	else 
	{
		gltexture = NULL;
		lightlevel = abs(lightlevel);
	}

	// get height from vplane
	if (whichplane == sector_t::floor && sector->transdoor) dz = -1;
	else dz = 0;

	z = plane.plane.ZatPoint(0.f, 0.f);
	
	PutFlat(fog);
	rendered_flats++;
}

//==========================================================================
//
// Sets 3D floor info. Common code for all 4 cases 
//
//==========================================================================

void GLFlat::SetFrom3DFloor(F3DFloor *rover, bool top, bool underside)
{
	F3DFloor::planeref & plane = top? rover->top : rover->bottom;

	// FF_FOG requires an inverted logic where to get the light from
	lightlist_t *light = P_GetPlaneLight(sector, plane.plane, underside);
	lightlevel = *light->p_lightlevel;
	
	if (rover->flags & FF_FOG) Colormap.LightColor = (light->extra_colormap)->Fade;
	else Colormap.CopyLightColor(light->extra_colormap);

	alpha = rover->alpha/255.0f;
	renderstyle = rover->flags&FF_ADDITIVETRANS? STYLE_Add : STYLE_Translucent;
	if (plane.model->VBOHeightcheck(plane.isceiling))
	{
		vboindex = plane.vindex;
	}
	else
	{
		vboindex = -1;
	}
}

//==========================================================================
//
// Process a sector's flats for rendering
// This function is only called once per sector.
// Subsequent subsectors are just quickly added to the ss_renderflags array
//
//==========================================================================

void GLFlat::ProcessSector(sector_t * frontsector)
{
	lightlist_t * light;

#ifdef _DEBUG
	if (frontsector->sectornum==gl_breaksec)
	{
		int a = 0;
	}
#endif

	// Get the real sector for this one.
	sector=&sectors[frontsector->sectornum];	
	extsector_t::xfloor &x = sector->e->XFloor;
	this->sub=NULL;

	byte &srf = gl_drawinfo->sectorrenderflags[sector->sectornum];

	//
	//
	//
	// do floors
	//
	//
	//
	if (frontsector->floorplane.ZatPoint(FIXED2FLOAT(viewx), FIXED2FLOAT(viewy)) <= FIXED2FLOAT(viewz))
	{
		// process the original floor first.

		srf |= SSRF_RENDERFLOOR;

		lightlevel = gl_ClampLight(frontsector->GetFloorLight());
		Colormap=frontsector->ColorMap;
		Colormap.LightColorMul(frontsector->SpecialColors[sector_t::floor]);//[GEC]

		if ((stack = (frontsector->portals[sector_t::floor] != NULL)))
		{
			gl_drawinfo->AddFloorStack(sector);
			alpha = frontsector->GetAlpha(sector_t::floor)/65536.0f;
		}
		else
		{
			alpha = 1.0f-frontsector->GetReflect(sector_t::floor);
		}
		if (frontsector->VBOHeightcheck(sector_t::floor))
		{
			vboindex = frontsector->vboindex[sector_t::floor];
		}
		else
		{
			vboindex = -1;
		}

		ceiling=false;
		renderflags=SSRF_RENDERFLOOR;

		if (x.ffloors.Size())
		{
			light = P_GetPlaneLight(sector, &frontsector->floorplane, false);
			if ((!(sector->GetFlags(sector_t::floor)&PLANEF_ABSLIGHTING) || !light->fromsector)	
				&& (light->p_lightlevel != &frontsector->lightlevel))
			{
				lightlevel = *light->p_lightlevel;
			}

			Colormap.CopyLightColor(light->extra_colormap);
		}
		renderstyle = STYLE_Translucent;

		if (frontsector->Flags & SECF_LIQUIDFLOOR)// [GEC]
		{
			renderflags |= SSRF_WATER1;
			Process(frontsector, sector_t::floor, false, 1);// [GEC] Encuentra la siguiente textura
			renderflags &= ~SSRF_WATER1;

			renderflags |= SSRF_WATER2;
			alpha = (0.627451 * alpha) / 1.0;//Equivale a 0xA0
			Process(frontsector, sector_t::floor, false);
			renderflags &= ~SSRF_WATER2;

		}
		else
		{
			if (alpha!=0.0f) Process(frontsector, false, false);
		}
	}
	
	//
	//
	//
	// do ceilings
	//
	//
	//
	if (frontsector->ceilingplane.ZatPoint(FIXED2FLOAT(viewx), FIXED2FLOAT(viewy)) >= FIXED2FLOAT(viewz))
	{
		// process the original ceiling first.

		srf |= SSRF_RENDERCEILING;

		lightlevel = gl_ClampLight(frontsector->GetCeilingLight());
		Colormap=frontsector->ColorMap;
		Colormap.LightColorMul(frontsector->SpecialColors[sector_t::ceiling]);//[GEC]

		if ((stack = (frontsector->portals[sector_t::ceiling] != NULL))) 
		{
			gl_drawinfo->AddCeilingStack(sector);
			alpha = frontsector->GetAlpha(sector_t::ceiling)/65536.0f;
		}
		else
		{
			alpha = 1.0f-frontsector->GetReflect(sector_t::ceiling);
		}

		if (frontsector->VBOHeightcheck(sector_t::ceiling))
		{
			vboindex = frontsector->vboindex[sector_t::ceiling];
		}
		else
		{
			vboindex = -1;
		}

		ceiling=true;
		renderflags=SSRF_RENDERCEILING;

		if (x.ffloors.Size())
		{
			light = P_GetPlaneLight(sector, &sector->ceilingplane, true);

			if ((!(sector->GetFlags(sector_t::ceiling)&PLANEF_ABSLIGHTING))
				&& (light->p_lightlevel != &frontsector->lightlevel))
			{
				lightlevel = *light->p_lightlevel;
			}
			Colormap.CopyLightColor(light->extra_colormap);
		}
		renderstyle = STYLE_Translucent;
		if (alpha!=0.0f) Process(frontsector, true, false);
	}

	//
	//
	//
	// do 3D floors
	//
	//
	//

	stack=false;
	if (x.ffloors.Size())
	{
		player_t * player=players[consoleplayer].camera->player;

		renderflags=SSRF_RENDER3DPLANES;
		srf |= SSRF_RENDER3DPLANES;
		// 3d-floors must not overlap!
		fixed_t lastceilingheight=sector->CenterCeiling();	// render only in the range of the
		fixed_t lastfloorheight=sector->CenterFloor();		// current sector part (if applicable)
		F3DFloor * rover;	
		int k;
		
		// floors are ordered now top to bottom so scanning the list for the best match
		// is no longer necessary.

		ceiling=true;
		for(k=0;k<(int)x.ffloors.Size();k++)
		{
			rover=x.ffloors[k];
			
			if ((rover->flags&(FF_EXISTS|FF_RENDERPLANES|FF_THISINSIDE))==(FF_EXISTS|FF_RENDERPLANES))
			{
				if (rover->flags&FF_FOG && gl_fixedcolormap) continue;
				if (!rover->top.copied && rover->flags&(FF_INVERTPLANES|FF_BOTHPLANES))
				{
					fixed_t ff_top=rover->top.plane->ZatPoint(CenterSpot(sector));
					if (ff_top<lastceilingheight)
					{
						if (FIXED2FLOAT(viewz) <= rover->top.plane->ZatPoint(FIXED2FLOAT(viewx), FIXED2FLOAT(viewy)))
						{
							SetFrom3DFloor(rover, true, !!(rover->flags&FF_FOG));
							Colormap.FadeColor=frontsector->ColorMap->Fade;
							Process(rover->top.model, rover->top.isceiling, !!(rover->flags&FF_FOG));
						}
						lastceilingheight=ff_top;
					}
				}
				if (!rover->bottom.copied && !(rover->flags&FF_INVERTPLANES))
				{
					fixed_t ff_bottom=rover->bottom.plane->ZatPoint(CenterSpot(sector));
					if (ff_bottom<lastceilingheight)
					{
						if (FIXED2FLOAT(viewz)<=rover->bottom.plane->ZatPoint(FIXED2FLOAT(viewx), FIXED2FLOAT(viewy)))
						{
							SetFrom3DFloor(rover, false, !(rover->flags&FF_FOG));
							Colormap.FadeColor=frontsector->ColorMap->Fade;
							Process(rover->bottom.model, rover->bottom.isceiling, !!(rover->flags&FF_FOG));
						}
						lastceilingheight=ff_bottom;
						if (rover->alpha<255) lastceilingheight++;
					}
				}
			}
		}
			  
		ceiling=false;
		for(k=x.ffloors.Size()-1;k>=0;k--)
		{
			rover=x.ffloors[k];
			
			if ((rover->flags&(FF_EXISTS|FF_RENDERPLANES|FF_THISINSIDE))==(FF_EXISTS|FF_RENDERPLANES))
			{
				if (rover->flags&FF_FOG && gl_fixedcolormap) continue;
				if (!rover->bottom.copied && rover->flags&(FF_INVERTPLANES|FF_BOTHPLANES))
				{
					fixed_t ff_bottom=rover->bottom.plane->ZatPoint(CenterSpot(sector));
					if (ff_bottom>lastfloorheight || (rover->flags&FF_FIX))
					{
						if (FIXED2FLOAT(viewz) >= rover->bottom.plane->ZatPoint(FIXED2FLOAT(viewx), FIXED2FLOAT(viewy)))
						{
							SetFrom3DFloor(rover, false, !(rover->flags&FF_FOG));
							Colormap.FadeColor=frontsector->ColorMap->Fade;

							if (rover->flags&FF_FIX)
							{
								lightlevel = gl_ClampLight(rover->model->lightlevel);
								Colormap = rover->GetColormap();
							}

							Process(rover->bottom.model, rover->bottom.isceiling, !!(rover->flags&FF_FOG));
						}
						lastfloorheight=ff_bottom;
					}
				}
				if (!rover->top.copied && !(rover->flags&FF_INVERTPLANES))
				{
					fixed_t ff_top=rover->top.plane->ZatPoint(CenterSpot(sector));
					if (ff_top>lastfloorheight)
					{
						if (FIXED2FLOAT(viewz) >= rover->top.plane->ZatPoint(FIXED2FLOAT(viewx), FIXED2FLOAT(viewy)))
						{
							SetFrom3DFloor(rover, true, !!(rover->flags&FF_FOG));
							Colormap.FadeColor=frontsector->ColorMap->Fade;
							Process(rover->top.model, rover->top.isceiling, !!(rover->flags&FF_FOG));
						}
						lastfloorheight=ff_top;
						if (rover->alpha<255) lastfloorheight--;
					}
				}
			}
		}
	}
}

