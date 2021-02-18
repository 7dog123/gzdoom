/*
** gl_walls_draw.cpp
** Wall rendering
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
#include "p_local.h"
#include "p_lnspec.h"
#include "a_sharedglobal.h"
#include "gl/gl_functions.h"

#include "gl/system/gl_interface.h"
#include "gl/system/gl_cvars.h"
#include "gl/renderer/gl_lightdata.h"
#include "gl/renderer/gl_renderstate.h"
#include "gl/data/gl_data.h"
#include "gl/dynlights/gl_dynlight.h"
#include "gl/dynlights/gl_glow.h"
#include "gl/scene/gl_drawinfo.h"
#include "gl/scene/gl_portal.h"
#include "gl/shaders/gl_shader.h"
#include "gl/textures/gl_material.h"
#include "gl/utility/gl_clock.h"
#include "gl/utility/gl_templates.h"
#include "gl/textures/gl_combiners.h"//[GEC]

EXTERN_CVAR(Bool, gl_seamless)

//==========================================================================
//
// [GEC] Create LightPSX Texture
//
//==========================================================================

int SetTexNum2 = GL_TEXTURE0;
static int LastFactor;
texcoord tcs2[4];

struct FSoftWall
{
	GLuint Softwall;
}SoftWalls[16384];

int GLWall::GetLightPSX(int light, int count)
{
	//Get MinLevel
	int MinLevel = PSXDoomLightingEquation(light/255.0, 200, 0);

	float zc = FIXED2FLOAT(seg->frontsector->GetPlaneTexZ(sector_t::ceiling));
	float zf = FIXED2FLOAT(seg->frontsector->GetPlaneTexZ(sector_t::floor));

	float vtx[]={glseg.x1,zf,glseg.y1, glseg.x1,zc,glseg.y1, glseg.x2,zc,glseg.y2, glseg.x2,zf,glseg.y2};
	Plane p;

	p.Init(vtx,4);

	if (!p.ValidNormal()) return MinLevel;

	float dist = fabsf(p.DistToPoint(FIXED2FLOAT(viewx), FIXED2FLOAT(viewz), FIXED2FLOAT(viewy)));
	float radius = (434);
	if (radius <= 0.f)   return MinLevel;
	if (dist > radius)   return MinLevel;

	///
	tcs2[0].u=0.0; tcs2[0].v=0.0;
	tcs2[1].u=0.0; tcs2[1].v=1.0;
	tcs2[2].u=1.0; tcs2[2].v=1.0;
	tcs2[3].u=1.0; tcs2[3].v=0.0;

	int Distance = (int)Dist2(glseg.x1,glseg.y1, glseg.x2,glseg.y2);

	//Funcion que da valores en pares "2"
	int i14 = 0;
	Distance = (i14 = Distance) % 8 != 0 ? i14 / 8 + 1 : i14 / 8;
    Distance *= 8;

	Distance = clamp(Distance,4,4096);//Min And Max Distance

	int W = Distance;
	int H = 1;
	byte *buffer;
	int k = 0;

	float xcamera = 0;
	float ycamera = 0;
	float factor = 0;
	float Dist = 0;

	bool CreateTextrue = false;

	double dx=(glseg.x2 - glseg.x1);
	double dy=(glseg.y2 - glseg.y1);
	double steps;
	float xInc,yInc, x, y;
	steps = (float)Distance;
	xInc=dx/(float)steps;
	yInc=dy/(float)steps;

	//Create Buffer
	buffer = (byte *)calloc(sizeof(byte),(W*H));
	memset(buffer, MinLevel, sizeof(byte) * (W*H));

	x = glseg.x1;
	y = glseg.y1;

	//Set New Buffer
	for(k = 0; k < Distance;k++)
	{
		x += xInc;
		y += yInc;

		xcamera = FIXED2FLOAT(viewx);
		ycamera = FIXED2FLOAT(viewy);

		Dist = Dist2(xcamera,ycamera, x, y);
		factor = fabs(-92.0f * (Dist / 200.f));

		buffer[k] = PSXDoomLightingEquation(light / 255.0, factor, 0);
	}

	//Check MinLevel
	for(k = 0; k < Distance;k++)
	{
		if(buffer[k] == MinLevel)
		{
			continue;
		}
		else
		{
			CreateTextrue = true;
			break;
		}
	}

	/*if(LastFactor == factor)
	{
		glBindTexture( GL_TEXTURE_2D, SoftWall);
		free(buffer);
		return -1;
	}

	//Save LastFactor
	LastFactor = factor;*/

	if(CreateTextrue)
	{
		if(LastFactor == factor)
		{
			//glBindTexture( GL_TEXTURE_2D, SoftWall);
			glBindTexture( GL_TEXTURE_2D, SoftWalls[count].Softwall);
			free(buffer);
			return -1;
		}

		glDeleteTextures(1, &SoftWalls[count].Softwall);

		//Save LastFactor
		LastFactor = factor;

		//if(SoftWall == 0)
			//glGenTextures(1, &SoftWall);

		//glBindTexture( GL_TEXTURE_2D, SoftWall);

		if(SoftWalls[count].Softwall == 0)
			glGenTextures(1, &SoftWalls[count].Softwall);

		glBindTexture( GL_TEXTURE_2D, SoftWalls[count].Softwall);

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		// update texture data
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, W, H, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, buffer);
		free(buffer);
		return -1;
	}

	/*if (SoftWall != 0) 
	{
		glDeleteTextures(1, &SoftWall);
		SoftWall = 0;
	}*/

	free(buffer);
	return MinLevel;
}

//==========================================================================
//
// [GEC] Aply blends and PSX LIGHT
//
//==========================================================================

void GLWall::SetSpecials(int pass, int light, int rellight, int lightlevel64)//[GEC]
{
	//return;
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
			gl_RenderState.SetLightMode(1);//[GEC]
	}

	//Pass 5: Blend D64 if false
	if(pass == GLPASS_ALL || pass == GLPASS_PLAIN)
	{
		if(!psx_blend)
		{
			gl_RenderState.SetBlendColor(flashcolor, 1);
		}
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
		if(pass == GLPASS_PLAIN || pass == GLPASS_TEXTURE || pass == GLPASS_BASE_MASKED)
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
		{
			if(glset.lightmode == 16)
			{
				texnum ++;
				int count = GLWall::seg->linedef-lines; //Printf("count %d\n",count);
				int newlight = gl_CalcLightLevel(light, rellight, false);
				Enable_texunit(texnum);
				int minlevel = (!gl_fixedcolormap) ? GetLightPSX(newlight, count) : 0xff;
				glMatrixMode(GL_TEXTURE);
				glLoadIdentity();
				glRotatef(0.0,0.0,0.0,1.0);
				glMatrixMode(GL_PROJECTION);
				if(minlevel == -1)
				{
					SetTexNum2 = GL_TEXTURE0+texnum;
					glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
					glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);   //Modulate RGB with RGB
					glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PREVIOUS);
					glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
					glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_TEXTURE0 + texnum);
					glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
				}
				else
				{
					SetTexNum2 = GL_TEXTURE0;
					gltexture->Bind(Colormap.colormap,0,0,0,texnum);
					SetTextureUnit(texnum, true);
					SetColor(minlevel, minlevel, minlevel, 0xff);
				}
			}
		}

		//Pass 5: Blend D64 if false
		if(pass == GLPASS_TEXTURE)
		{
			if(!psx_blend)
			{
				texnum ++;
				gltexture->Bind(Colormap.colormap,0,0,0,texnum);
				SetTextureUnit(texnum, true);
				SetColor(0xff, 0xff, 0xff, 0xff);
			}
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
// Sets up the texture coordinates for one light to be rendered
//
//==========================================================================
bool GLWall::PrepareLight(texcoord * tcs, ADynamicLight * light)
{
	float vtx[]={glseg.x1,zbottom[0],glseg.y1, glseg.x1,ztop[0],glseg.y1, glseg.x2,ztop[1],glseg.y2, glseg.x2,zbottom[1],glseg.y2};
	Plane p;
	Vector nearPt, up, right;
	float scale;

	p.Init(vtx,4);

	if (!p.ValidNormal()) 
	{
		return false;
	}

	if (!gl_SetupLight(p, light, nearPt, up, right, scale, Colormap.colormap, true, !!(flags&GLWF_FOGGY))) 
	{
		return false;
	}

	if (tcs != NULL)
	{
		Vector t1;
		int outcnt[4]={0,0,0,0};

		for(int i=0;i<4;i++)
		{
			t1.Set(&vtx[i*3]);
			Vector nearToVert = t1 - nearPt;
			tcs[i].u = (nearToVert.Dot(right) * scale) + 0.5f;
			tcs[i].v = (nearToVert.Dot(up) * scale) + 0.5f;

			// quick check whether the light touches this polygon
			if (tcs[i].u<0) outcnt[0]++;
			if (tcs[i].u>1) outcnt[1]++;
			if (tcs[i].v<0) outcnt[2]++;
			if (tcs[i].v>1) outcnt[3]++;

		}
		// The light doesn't touch this polygon
		if (outcnt[0]==4 || outcnt[1]==4 || outcnt[2]==4 || outcnt[3]==4) return false;
	}

	draw_dlight++;
	return true;
}

//==========================================================================
//
// Collect lights for shader
//
//==========================================================================
FDynLightData lightdata;

void GLWall::SetupLights()
{
	float vtx[]={glseg.x1,zbottom[0],glseg.y1, glseg.x1,ztop[0],glseg.y1, glseg.x2,ztop[1],glseg.y2, glseg.x2,zbottom[1],glseg.y2};
	Plane p;

	lightdata.Clear();
	p.Init(vtx,4);

	if (!p.ValidNormal()) 
	{
		return;
	}
	for(int i=0;i<2;i++)
	{
		FLightNode *node;
		if (seg->sidedef == NULL)
		{
			node = NULL;
		}
		else if (!(seg->sidedef->Flags & WALLF_POLYOBJ))
		{
			node = seg->sidedef->lighthead[i];
		}
		else if (sub)
		{
			// Polobject segs cannot be checked per sidedef so use the subsector instead.
			node = sub->lighthead[i];
		}
		else node = NULL;

		// Iterate through all dynamic lights which touch this wall and render them
		while (node)
		{
			if (!(node->lightsource->flags2&MF2_DORMANT))
			{
				iter_dlight++;

				Vector fn, pos;

				float x = FIXED2FLOAT(node->lightsource->X());
				float y = FIXED2FLOAT(node->lightsource->Y());
				float z = FIXED2FLOAT(node->lightsource->Z());
				float dist = fabsf(p.DistToPoint(x, z, y));
				float radius = (node->lightsource->GetRadius() * gl_lights_size);
				float scale = 1.0f / ((2.f * radius) - dist);

				if (radius > 0.f && dist < radius)
				{
					Vector nearPt, up, right;

					pos.Set(x,z,y);
					fn=p.Normal();
					fn.GetRightUp(right, up);

					Vector tmpVec = fn * dist;
					nearPt = pos + tmpVec;

					Vector t1;
					int outcnt[4]={0,0,0,0};
					texcoord tcs[4];

					// do a quick check whether the light touches this polygon
					for(int i=0;i<4;i++)
					{
						t1.Set(&vtx[i*3]);
						Vector nearToVert = t1 - nearPt;
						tcs[i].u = (nearToVert.Dot(right) * scale) + 0.5f;
						tcs[i].v = (nearToVert.Dot(up) * scale) + 0.5f;

						if (tcs[i].u<0) outcnt[0]++;
						if (tcs[i].u>1) outcnt[1]++;
						if (tcs[i].v<0) outcnt[2]++;
						if (tcs[i].v>1) outcnt[3]++;

					}
					if (outcnt[0]!=4 && outcnt[1]!=4 && outcnt[2]!=4 && outcnt[3]!=4) 
					{
						gl_GetLight(p, node->lightsource, Colormap.colormap, true, false, lightdata);
					}
				}
			}
			node = node->nextLight;
		}
	}
	int numlights[3];

	lightdata.Combine(numlights, gl.MaxLights());
	if (numlights[2] > 0)
	{
		draw_dlight+=numlights[2]/2;
		gl_RenderState.EnableLight(true);
		gl_RenderState.SetLights(numlights, &lightdata.arrays[0][0]);
	}
}

//==========================================================================
//
// General purpose wall rendering function
// everything goes through here
//
//==========================================================================

//void GLWall::RenderWall(int textured, float * color2, ADynamicLight * light)
void GLWall::RenderWall(int textured, float * color2, ADynamicLight * light, bool color, float alpha, int extra)
{
	float FragR,FragG,FragB,FragA;//[GEC]
	vertcolor vrgb[4];//[GEC]

	texcoord tcs[4];
	bool split = (gl_seamless && !(textured&4) && seg->sidedef != NULL && !(seg->sidedef->Flags & WALLF_POLYOBJ));

	if (!light)
	{
		tcs[0]=lolft;
		tcs[1]=uplft;
		tcs[2]=uprgt;
		tcs[3]=lorgt;
		if (!!(flags&GLWF_GLOW) && (textured & 2))
		{
			gl_RenderState.SetGlowPlanes(topplane, bottomplane);
			gl_RenderState.SetGlowParams(topglowcolor, bottomglowcolor);
		}

	}
	else
	{
		if (!PrepareLight(tcs, light)) return;
	}


	gl_RenderState.Apply();

	/*// the rest of the code is identical for textured rendering and lights

	glBegin(GL_TRIANGLE_FAN);

	// lower left corner
	if (textured&1) glTexCoord2f(tcs[0].u,tcs[0].v);
	glVertex3f(glseg.x1,zbottom[0],glseg.y1);

	if (split && glseg.fracleft==0) SplitLeftEdge(tcs);

	// upper left corner
	if (textured&1) glTexCoord2f(tcs[1].u,tcs[1].v);
	glVertex3f(glseg.x1,ztop[0],glseg.y1);

	if (split && !(flags & GLWF_NOSPLITUPPER)) SplitUpperEdge(tcs);

	// color for right side
	if (color2) glColor4fv(color2);

	// upper right corner
	if (textured&1) glTexCoord2f(tcs[2].u,tcs[2].v);
	glVertex3f(glseg.x2,ztop[1],glseg.y2);

	if (split && glseg.fracright==1) SplitRightEdge(tcs);

	// lower right corner
	if (textured&1) glTexCoord2f(tcs[3].u,tcs[3].v); 
	glVertex3f(glseg.x2,zbottom[1],glseg.y2);

	if (split && !(flags & GLWF_NOSPLITLOWER)) SplitLowerEdge(tcs);

	glEnd();

	vertexcount+=4;*/

	if (!color)
	{
		// the rest of the code is identical for textured rendering and lights

		glBegin(GL_TRIANGLE_FAN);

		// lower left corner
		if (textured&1) //glTexCoord2f(tcs[0].u,tcs[0].v);
		{
			glMultiTexCoord2f(GL_TEXTURE0, tcs[0].u,tcs[0].v);
			if(SetTexNum2 != GL_TEXTURE0)
				glMultiTexCoord2f(SetTexNum2, tcs2[0].u,tcs2[0].v);
		}
		glVertex3f(glseg.x1,zbottom[0],glseg.y1);

		if (split && glseg.fracleft==0) SplitLeftEdge(tcs, tcs2, SetTexNum2, NULL);

		// upper left corner
		if (textured&1) //glTexCoord2f(tcs[1].u,tcs[1].v);
		{
			glMultiTexCoord2f(GL_TEXTURE0, tcs[1].u,tcs[1].v);
			if(SetTexNum2 != GL_TEXTURE0)
				glMultiTexCoord2f(SetTexNum2, tcs2[1].u,tcs2[1].v);
		}
		glVertex3f(glseg.x1,ztop[0],glseg.y1);

		if (split && !(flags & GLWF_NOSPLITUPPER)) SplitUpperEdge(tcs, tcs2, SetTexNum2, NULL);

		// color for right side
		if (color2)
		{
			glColor4fv(color2);
		}

		// upper right corner
		if (textured&1) //glTexCoord2f(tcs[2].u,tcs[2].v);
		{
			glMultiTexCoord2f(GL_TEXTURE0, tcs[2].u,tcs[2].v);
			if(SetTexNum2 != GL_TEXTURE0)
			glMultiTexCoord2f(SetTexNum2, tcs2[2].u,tcs2[2].v);
		}
		glVertex3f(glseg.x2,ztop[1],glseg.y2);

		if (split && glseg.fracright==1) SplitRightEdge(tcs, tcs2, SetTexNum2, NULL);

		// lower right corner
		if (textured&1) //glTexCoord2f(tcs[3].u,tcs[3].v); 
		{
			glMultiTexCoord2f(GL_TEXTURE0, tcs[3].u,tcs[3].v);
			if(SetTexNum2 != GL_TEXTURE0)
				glMultiTexCoord2f(SetTexNum2, tcs2[3].u,tcs2[3].v);
		}
		glVertex3f(glseg.x2,zbottom[1],glseg.y2);

		if (split && !(flags & GLWF_NOSPLITLOWER)) SplitLowerEdge(tcs, tcs2, SetTexNum2, NULL);

		glEnd();

		vertexcount+=4;
	}
	else
	{
		FColormap cmc = Colormap;

		// lower left corner
		cmc.LightColorMul(Wallcolor[0]);//[GEC]
		gl_SetColor(lightlevel, extra, &cmc, alpha);
		gl_RenderState.GetFragColor(&FragR, &FragG, &FragB, &FragA);//Get FragColor
		vrgb[0].r = FragR; vrgb[0].g = FragG; vrgb[0].b = FragB; vrgb[0].a = FragA;

		// upper left corner
		cmc = Colormap;
		cmc.LightColorMul(Wallcolor[1]);//[GEC]
		gl_SetColor(lightlevel, extra, &cmc, alpha);
		gl_RenderState.GetFragColor(&FragR, &FragG, &FragB, &FragA);//Get FragColor
		vrgb[1].r = FragR; vrgb[1].g = FragG; vrgb[1].b = FragB; vrgb[1].a = FragA;

		// upper right corner
		cmc = Colormap;
		cmc.LightColorMul(Wallcolor[2]);//[GEC]
		gl_SetColor(lightlevel, extra, &cmc, alpha);
		gl_RenderState.GetFragColor(&FragR, &FragG, &FragB, &FragA);//Get FragColor
		vrgb[2].r = FragR; vrgb[2].g = FragG; vrgb[2].b = FragB; vrgb[2].a = FragA;

		// lower right corner
		cmc = Colormap;
		cmc.LightColorMul(Wallcolor[3]);//[GEC]
		gl_SetColor(lightlevel, extra, &cmc, alpha);
		gl_RenderState.GetFragColor(&FragR, &FragG, &FragB, &FragA);//Get FragColor
		vrgb[3].r = FragR; vrgb[3].g = FragG; vrgb[3].b = FragB; vrgb[3].a = FragA;

		// the rest of the code is identical for textured rendering and lights
		glBegin(GL_TRIANGLE_FAN);

		// lower left corner
		glColor4f(vrgb[0].r, vrgb[0].g, vrgb[0].b, vrgb[0].a);

		if (textured&1) //glTexCoord2f(tcs[0].u,tcs[0].v);
		{
			glMultiTexCoord2f(GL_TEXTURE0, tcs[0].u,tcs[0].v);
			if(SetTexNum2 != GL_TEXTURE0)
				glMultiTexCoord2f(SetTexNum2, tcs2[0].u,tcs2[0].v);
		}
		glVertex3f(glseg.x1,zbottom[0],glseg.y1);

		if (split && glseg.fracleft==0) SplitLeftEdge(tcs, tcs2, SetTexNum2, vrgb);

		// upper left corner
		glColor4f(vrgb[1].r, vrgb[1].g, vrgb[1].b, vrgb[1].a);

		if (textured&1) //glTexCoord2f(tcs[1].u,tcs[1].v);
		{
			glMultiTexCoord2f(GL_TEXTURE0, tcs[1].u,tcs[1].v);
			if(SetTexNum2 != GL_TEXTURE0)
				glMultiTexCoord2f(SetTexNum2, tcs2[1].u,tcs2[1].v);
		}
		glVertex3f(glseg.x1,ztop[0],glseg.y1);

		if (split && !(flags & GLWF_NOSPLITUPPER)) SplitUpperEdge(tcs, tcs2, SetTexNum2, vrgb);

		// color for right side
		if (color2) glColor4fv(color2);

		// upper right corner
		glColor4f(vrgb[2].r, vrgb[2].g, vrgb[2].b, vrgb[2].a);

		if (textured&1) //glTexCoord2f(tcs[2].u,tcs[2].v);
		{
			glMultiTexCoord2f(GL_TEXTURE0, tcs[2].u,tcs[2].v);
			if(SetTexNum2 != GL_TEXTURE0)
				glMultiTexCoord2f(SetTexNum2, tcs2[2].u,tcs2[2].v);
		}
		glVertex3f(glseg.x2,ztop[1],glseg.y2);

		if (split && glseg.fracright==1) SplitRightEdge(tcs, tcs2, SetTexNum2, vrgb);

		// lower right corner
		glColor4f(vrgb[3].r, vrgb[3].g, vrgb[3].b, vrgb[3].a);

		if (textured&1) //glTexCoord2f(tcs[3].u,tcs[3].v); 
		{
			glMultiTexCoord2f(GL_TEXTURE0, tcs[3].u,tcs[3].v);
			if(SetTexNum2 != GL_TEXTURE0)
				glMultiTexCoord2f(SetTexNum2, tcs2[3].u,tcs2[3].v);
		}
		glVertex3f(glseg.x2,zbottom[1],glseg.y2);

		if (split && !(flags & GLWF_NOSPLITLOWER)) SplitLowerEdge(tcs, tcs2, SetTexNum2, vrgb);

		glEnd();

		vertexcount+=4;
	}

}

//==========================================================================
//
// 
//
//==========================================================================

void GLWall::RenderFogBoundary()
{
	if (gl_fogmode && gl_fixedcolormap == 0)
	{
		// with shaders this can be done properly
		if (gl.shadermodel == 4 || (gl.shadermodel == 3 && gl_fog_shader))
		{
			int rel = rellight + getExtraLight();
			gl_SetFog(lightlevel, rel, &Colormap, false);
			gl_RenderState.SetEffect(EFF_FOGBOUNDARY);
			gl_RenderState.EnableAlphaTest(false);
			RenderWall(0, NULL);
			gl_RenderState.EnableAlphaTest(true);
			gl_RenderState.SetEffect(EFF_NONE);
		}
		else
		{
			// otherwise some approximation is needed. This won't look as good
			// as the shader version but it's an acceptable compromise.
			float fogdensity=gl_GetFogDensity(lightlevel, Colormap.FadeColor);

			float xcamera=FIXED2FLOAT(viewx);
			float ycamera=FIXED2FLOAT(viewy);

			float dist1=Dist2(xcamera,ycamera, glseg.x1,glseg.y1);
			float dist2=Dist2(xcamera,ycamera, glseg.x2,glseg.y2);


			// these values were determined by trial and error and are scale dependent!
			float fogd1=(0.95f-exp(-fogdensity*dist1/62500.f)) * 1.05f;
			float fogd2=(0.95f-exp(-fogdensity*dist2/62500.f)) * 1.05f;

			gl_ModifyColor(Colormap.FadeColor.r, Colormap.FadeColor.g, Colormap.FadeColor.b, Colormap.colormap);
			float fc[4]={Colormap.FadeColor.r/255.0f,Colormap.FadeColor.g/255.0f,Colormap.FadeColor.b/255.0f,fogd2};

			gl_RenderState.EnableTexture(false);
			gl_RenderState.EnableFog(false);
			gl_RenderState.AlphaFunc(GL_GREATER,0);
			glDepthFunc(GL_LEQUAL);
			glColor4f(fc[0],fc[1],fc[2], fogd1);
			gl_RenderState.SetFragColor(fc[0], fc[1], fc[2], fogd1);//[GEC]
			if (glset.lightmode == 8) glVertexAttrib1f(VATTR_LIGHTLEVEL, 1.0); // Korshun.
			if (glset.lightmode == 16) glVertexAttrib1f(VATTR_LIGHTLEVEL, 1.0); // [GEC]

			flags &= ~GLWF_GLOW;
			RenderWall(4,fc);

			glDepthFunc(GL_LESS);
			gl_RenderState.EnableFog(true);
			gl_RenderState.AlphaFunc(GL_GEQUAL,0.5f);
			gl_RenderState.EnableTexture(true);
		}
	}
}


//==========================================================================
//
// 
//
//==========================================================================
void GLWall::RenderMirrorSurface()
{
	if (GLRenderer->mirrortexture == NULL) return;

	// For the sphere map effect we need a normal of the mirror surface,
	Vector v(glseg.y2-glseg.y1, 0 ,-glseg.x2+glseg.x1);
	v.Normalize();
	glNormal3fv(&v[0]);

	// Use sphere mapping for this
	gl_RenderState.SetEffect(EFF_SPHEREMAP);

	gl_SetColor(lightlevel, 0, &Colormap ,0.1f);
	gl_RenderState.BlendFunc(GL_SRC_ALPHA,GL_ONE);
	gl_RenderState.AlphaFunc(GL_GREATER,0);
	glDepthFunc(GL_LEQUAL);
	gl_SetFog(lightlevel, getExtraLight(), &Colormap, true);

	FMaterial * pat=FMaterial::ValidateTexture(GLRenderer->mirrortexture);
	pat->BindPatch(Colormap.colormap, 0);

	flags &= ~GLWF_GLOW;
	//flags |= GLWF_NOSHADER;
	RenderWall(0,NULL);

	gl_RenderState.SetEffect(EFF_NONE);

	// Restore the defaults for the translucent pass
	gl_RenderState.BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	gl_RenderState.AlphaFunc(GL_GEQUAL,0.5f*gl_mask_sprite_threshold);
	glDepthFunc(GL_LESS);

	// This is drawn in the translucent pass which is done after the decal pass
	// As a result the decals have to be drawn here.
	if (seg->sidedef->AttachedDecals)
	{
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(-1.0f, -128.0f);
		glDepthMask(false);
		DoDrawDecals();
		glDepthMask(true);
		glPolygonOffset(0.0f, 0.0f);
		glDisable(GL_POLYGON_OFFSET_FILL);
		gl_RenderState.SetTextureMode(TM_MODULATE);
		gl_RenderState.BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
}

//==========================================================================
//
// [GEC] Set HV Mirror Textures
//
//==========================================================================

void GLWall::SetHVMirror()
{
	int WrapS = 0;
	int WrapT = 0;

	if ((seg->linedef->gecflags & ML_UV_WRAP_H_MIRROR)) { glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT); WrapS = 1;}
	else { glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);}

	if ((seg->linedef->gecflags & ML_UV_WRAP_V_MIRROR)) { glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT); WrapT = 1;}
	else { glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);}

	gl_RenderState.SetWarpTexture(WrapS, WrapT);
}

//==========================================================================
//
// 
//
//==========================================================================

void GLWall::RenderTranslucentWall()
{
	int texturemode;//[GEC]
	gl_RenderState.GetTextureMode(&texturemode);//[GEC]
	gl_RenderState.ResetSpecials();//[GEC]

	bool transparent = gltexture? gltexture->GetTransparent() : false;
	
	// currently the only modes possible are solid, additive or translucent
	// and until that changes I won't fix this code for the new blending modes!
	bool isadditive = RenderStyle == STYLE_Add;

	if (!transparent) gl_RenderState.AlphaFunc(GL_GEQUAL,gl_mask_threshold*fabs(alpha));
	else gl_RenderState.EnableAlphaTest(false);
	if (isadditive) gl_RenderState.BlendFunc(GL_SRC_ALPHA,GL_ONE);

	int extra;
	if (gltexture) 
	{
		if (flags&GLWF_FOGGY) gl_RenderState.EnableBrightmap(false);
		gl_RenderState.EnableGlow(!!(flags & GLWF_GLOW));

		//[GEC]

		SetHVMirror();//[GEC]
		gltexture->Bind(Colormap.colormap, flags, 0);
		extra = getExtraLight();

		SetSpecials(GLPASS_PLAIN, lightlevel, extra ,seg->frontsector->lightlevel_64);// [GEC]
	}
	else 
	{
		gl_RenderState.EnableTexture(false);
		extra = 0;
	}

	gl_SetColor(lightlevel, extra, &Colormap, fabsf(alpha));
	if (type!=RENDERWALL_M2SNF) gl_SetFog(lightlevel, extra, &Colormap, isadditive);
	else gl_SetFog(255, 0, NULL, false);

	//RenderWall(5,NULL);
	RenderWall(5, NULL, 0, true, fabsf(alpha), extra);

	// restore default settings
	if (isadditive) gl_RenderState.BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	if (transparent) gl_RenderState.EnableAlphaTest(true);

	if (!gltexture)	
	{
		gl_RenderState.EnableTexture(true);
	}
	gl_RenderState.EnableBrightmap(true);
	gl_RenderState.EnableGlow(false);

	//[GEC] Reset default
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	//[GEC] Reset default
	Disable_texunits();
	gl_RenderState.SetTextureMode(texturemode);
}

//==========================================================================
//
// 
//
//==========================================================================
void GLWall::Draw(int pass)
{
	FLightNode * node;
	int rel;

	int texturemode;//[GEC]
	gl_RenderState.GetTextureMode(&texturemode);//[GEC]
	gl_RenderState.ResetSpecials();//[GEC]
	gl_RenderState.Apply();//[GEC]

#ifdef _DEBUG
	if (seg->linedef-lines==879)
	{
		int a = 0;
	}
#endif


	// This allows mid textures to be drawn on lines that might overlap a sky wall
	if ((flags&GLWF_SKYHACK && type==RENDERWALL_M2S) || type == RENDERWALL_COLORLAYER)
	{
		if (pass != GLPASS_DECALS)
		{
			glEnable(GL_POLYGON_OFFSET_FILL);
			glPolygonOffset(-1.0f, -128.0f);
		}
	}

	switch (pass)
	{
	case GLPASS_ALL:			// Single-pass rendering
		SetupLights();
		// fall through
	case GLPASS_PLAIN:			// Single-pass rendering
		rel = rellight + getExtraLight();
		gl_SetColor(lightlevel, rel, &Colormap,1.0f);
		if (type!=RENDERWALL_M2SNF) gl_SetFog(lightlevel, rel, &Colormap, false);
		else gl_SetFog(255, 0, NULL, false);

		gl_RenderState.EnableGlow(!!(flags & GLWF_GLOW));
		gltexture->Bind(Colormap.colormap, flags, 0);

		SetSpecials(pass, lightlevel, rel ,seg->frontsector->lightlevel_64);// [GEC]

		SetHVMirror();//[GEC]
		//RenderWall(3, NULL);
		RenderWall(3, NULL, 0, true, 1.0f, rel);

		gl_RenderState.EnableGlow(false);
		gl_RenderState.EnableLight(false);

		//[GEC] Reset default
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		//[GEC] Reset default
		Disable_texunits();
		gl_RenderState.SetTextureMode(texturemode);
		break;

	case GLPASS_BASE:			// Base pass for non-masked polygons (all opaque geometry)
	case GLPASS_BASE_MASKED:	// Base pass for masked polygons (2sided mid-textures and transparent 3D floors)
		rel = rellight + getExtraLight();
		gl_SetColor(lightlevel, rel, &Colormap,1.0f);
		if (!(flags&GLWF_FOGGY)) 
		{
			if (type!=RENDERWALL_M2SNF) gl_SetFog(lightlevel, rel, &Colormap, false);
			else gl_SetFog(255, 0, NULL, false);
		}
		gl_RenderState.EnableGlow(!!(flags & GLWF_GLOW));
		// fall through

		if (pass != GLPASS_BASE)
		{
			gltexture->Bind(Colormap.colormap, flags, 0);

			SetSpecials(pass, lightlevel, rel ,seg->frontsector->lightlevel_64);// [GEC]
		}

		SetHVMirror();//[GEC]
		//RenderWall(pass == GLPASS_BASE? 2:3, NULL);
		RenderWall(pass == GLPASS_BASE? 2:3, NULL, 0, true, 1.0f, rel);

		gl_RenderState.EnableGlow(false);
		gl_RenderState.EnableLight(false);

		//[GEC] Reset default
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		//[GEC] Reset default
		Disable_texunits();
		gl_RenderState.SetTextureMode(texturemode);
		break;

	case GLPASS_TEXTURE:		// modulated texture
		//[GEC]
		//gl_RenderState.GetTextureMode(&texturemode);//Get TextureMode
		//SetInitSpecials(texturemode, glset.lightmode, lightlevel, 0, seg->frontsector->lightlevel_64, false, FragCol, DynCol);

		gltexture->Bind(Colormap.colormap, flags, 0);

		SetSpecials(pass, 0, 0 ,seg->frontsector->lightlevel_64);// [GEC]

		//SetNewSpecials(gltexture, Colormap.colormap, false, false, false, true, pass);//[GEC]

		SetHVMirror();//[GEC]
		RenderWall(1, NULL);

		//[GEC] Reset default
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		//[GEC] Reset default
		Disable_texunits();
		gl_RenderState.SetTextureMode(texturemode);
		break;

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
					rel = rellight + getExtraLight();
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
				gltexture->Bind(Colormap.colormap, flags, 0);
				SetHVMirror();//[GEC]
				RenderWall(1, NULL);

				//[GEC] Reset default
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			}
		}
		break;
	}

	case GLPASS_LIGHT:
	case GLPASS_LIGHT_ADDITIVE:
		// black fog is diminishing light and should affect lights less than the rest!
		if (!(flags&GLWF_FOGGY)) gl_SetFog((255+lightlevel)>>1, 0, NULL, false);
		else gl_SetFog(lightlevel, 0, &Colormap, true);	

		if (seg->sidedef == NULL)
		{
			node = NULL;
		}
		else if (!(seg->sidedef->Flags & WALLF_POLYOBJ))
		{
			// Iterate through all dynamic lights which touch this wall and render them
			node = seg->sidedef->lighthead[pass==GLPASS_LIGHT_ADDITIVE];
		}
		else if (sub)
		{
			// To avoid constant rechecking for polyobjects use the subsector's lightlist instead
			node = sub->lighthead[pass==GLPASS_LIGHT_ADDITIVE];
		}
		else node = NULL;
		while (node)
		{
			if (!(node->lightsource->flags2&MF2_DORMANT))
			{
				iter_dlight++;

				//[GEC]
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

				RenderWall(1, NULL, node->lightsource);
			}
			node = node->nextLight;
		}
		break;

	case GLPASS_DECALS:
	case GLPASS_DECALS_NOFOG:
		if (seg->sidedef && seg->sidedef->AttachedDecals)
		{
			if (pass==GLPASS_DECALS) 
			{
				gl_SetFog(lightlevel, rellight + getExtraLight(), &Colormap, false);
			}
			DoDrawDecals();
		}
		break;

	case GLPASS_TRANSLUCENT:
		switch (type)
		{
		case RENDERWALL_MIRRORSURFACE:
			RenderMirrorSurface();
			break;

		case RENDERWALL_FOGBOUNDARY:
			RenderFogBoundary();
			break;

		default:
			RenderTranslucentWall();
			break;
		}
	}

	if ((flags&GLWF_SKYHACK && type==RENDERWALL_M2S) || type == RENDERWALL_COLORLAYER)
	{
		if (pass!=GLPASS_DECALS)
		{
			glDisable(GL_POLYGON_OFFSET_FILL);
			glPolygonOffset(0, 0);
		}
	}

	//[GEC] Reset default
	Disable_texunits();
	gl_RenderState.SetTextureMode(texturemode);
}
