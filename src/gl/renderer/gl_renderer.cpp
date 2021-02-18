/*
** gl1_renderer.cpp
** Renderer interface
**
**---------------------------------------------------------------------------
** Copyright 2008 Christoph Oelckers
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
#include "files.h"
#include "m_swap.h"
#include "v_video.h"
#include "r_data/r_translate.h"
#include "m_png.h"
#include "m_crc32.h"
#include "w_wad.h"
//#include "gl/gl_intern.h"
#include "gl/gl_functions.h"
#include "vectors.h"

#include "gl/system/gl_interface.h"
#include "gl/system/gl_framebuffer.h"
#include "gl/renderer/gl_renderer.h"
#include "gl/renderer/gl_lightdata.h"
#include "gl/renderer/gl_renderstate.h"
#include "gl/data/gl_data.h"
#include "gl/data/gl_vertexbuffer.h"
#include "gl/dynlights/gl_lightbuffer.h"
#include "gl/scene/gl_drawinfo.h"
#include "gl/shaders/gl_shader.h"
#include "gl/textures/gl_texture.h"
#include "gl/textures/gl_translate.h"
#include "gl/textures/gl_material.h"
#include "gl/utility/gl_clock.h"
#include "gl/utility/gl_templates.h"
#include "gl/models/gl_models.h"

#include "gl/scene/gl_skynew.h"//[GEC]

//===========================================================================
// 
// Renderer interface
//
//===========================================================================

//-----------------------------------------------------------------------------
//
// Initialize
//
//-----------------------------------------------------------------------------

FGLRenderer::FGLRenderer(OpenGLFrameBuffer *fb) 
{
	framebuffer = fb;
	mCurrentPortal = NULL;
	mMirrorCount = 0;
	mPlaneMirrorCount = 0;
	mLightCount = 0;
	mAngles = FRotator(0,0,0);
	mViewVector = FVector2(0,0);
	mVBO = NULL;
	gl_spriteindex = 0;
	mShaderManager = NULL;
	glpart2 = glpart = gllight = mirrortexture = NULL;
	psx_light_wall = NULL;//[GEC] Nuevo
	psx_light_plane = NULL;//[GEC] Nuevo
}

void FGLRenderer::Initialize()
{
	glpart2 = FTexture::CreateTexture(Wads.GetNumForFullName("glstuff/glpart2.png"), FTexture::TEX_MiscPatch);
	glpart = FTexture::CreateTexture(Wads.GetNumForFullName("glstuff/glpart.png"), FTexture::TEX_MiscPatch);
	mirrortexture = FTexture::CreateTexture(Wads.GetNumForFullName("glstuff/mirror.png"), FTexture::TEX_MiscPatch);
	gllight = FTexture::CreateTexture(Wads.GetNumForFullName("glstuff/gllight.png"), FTexture::TEX_MiscPatch);
	psx_light_wall = FTexture::CreateTexture(Wads.GetNumForFullName("glstuff/psx_light_wall.png"), FTexture::TEX_MiscPatch);//[GEC] Nuevo
	psx_light_plane = FTexture::CreateTexture(Wads.GetNumForFullName("glstuff/psx_light_plane.png"), FTexture::TEX_MiscPatch);//[GEC] Nuevo

	mVBO = new FFlatVertexBuffer;
	mFBID = 0;
	mOldFBID = 0;
	SetupLevel();
	mShaderManager = new FShaderManager;
}

FGLRenderer::~FGLRenderer() 
{
	gl_CleanModelData();
	gl_DeleteAllAttachedLights();
	FMaterial::FlushAll();
	if (mShaderManager != NULL) delete mShaderManager;
	if (mVBO != NULL) delete mVBO;
	if (glpart2) delete glpart2;
	if (glpart) delete glpart;
	if (mirrortexture) delete mirrortexture;
	if (gllight) delete gllight;
	if (psx_light_wall) delete psx_light_wall;//[GEC] Nuevo
	if (psx_light_plane) delete psx_light_plane;//[GEC] Nuevo
	if (mFBID != 0) glDeleteFramebuffersEXT(1, &mFBID);
}

//===========================================================================
// 
//
//
//===========================================================================

void FGLRenderer::SetupLevel()
{
	mVBO->CreateVBO();
}

void FGLRenderer::Begin2D()
{
	gl_RenderState.EnableFog(false);
	gl_RenderState.Set2DMode(true);
}

//===========================================================================
// 
//
//
//===========================================================================

void FGLRenderer::ProcessLowerMiniseg(seg_t *seg, sector_t * frontsector, sector_t * backsector)
{
	GLWall wall;
	wall.ProcessLowerMiniseg(seg, frontsector, backsector);
	rendered_lines++;
}

//===========================================================================
// 
//
//
//===========================================================================

void FGLRenderer::ProcessSprite(AActor *thing, sector_t *sector)
{
	GLSprite glsprite;
	glsprite.Process(thing, sector);
}

//===========================================================================
// 
//
//
//===========================================================================

void FGLRenderer::ProcessParticle(particle_t *part, sector_t *sector)
{
	GLSprite glsprite;
	glsprite.ProcessParticle(part, sector);//, 0, 0);
}

//===========================================================================
// 
//
//
//===========================================================================

void FGLRenderer::ProcessSector(sector_t *fakesector)
{
	GLFlat glflat;
	glflat.ProcessSector(fakesector);
}

//===========================================================================
// 
//
//
//===========================================================================

void FGLRenderer::FlushTextures()
{
	FMaterial::FlushAll();
}

//===========================================================================
// 
//
//
//===========================================================================

bool FGLRenderer::StartOffscreen()
{
	if (gl.flags & RFL_FRAMEBUFFER)
	{
		if (mFBID == 0) glGenFramebuffersEXT(1, &mFBID);
		glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &mOldFBID);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, mFBID);
		return true;
	}
	return false;
}

//===========================================================================
// 
//
//
//===========================================================================

void FGLRenderer::EndOffscreen()
{
	if (gl.flags & RFL_FRAMEBUFFER)
	{
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, mOldFBID);
	}
}

//===========================================================================
// 
//
//
//===========================================================================

unsigned char *FGLRenderer::GetTextureBuffer(FTexture *tex, int &w, int &h)
{
	FMaterial * gltex = FMaterial::ValidateTexture(tex);
	if (gltex)
	{
		return gltex->CreateTexBuffer(CM_DEFAULT, 0, w, h);
	}
	return NULL;
}

//===========================================================================
// 
//
//
//===========================================================================

void FGLRenderer::ClearBorders()
{
	OpenGLFrameBuffer *glscreen = static_cast<OpenGLFrameBuffer*>(screen);

	// Letterbox time! Draw black top and bottom borders.
	int width = glscreen->GetWidth();
	int height = glscreen->GetHeight();
	int trueHeight = glscreen->GetTrueHeight();

	int borderHeight = (trueHeight - height) / 2;

	glViewport(0, 0, width, trueHeight);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, width * 1.0, 0.0, trueHeight, -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glColor3f(0.f, 0.f, 0.f);
	gl_RenderState.Set2DMode(true);
	gl_RenderState.EnableTexture(false);
	gl_RenderState.Apply(true);

	glBegin(GL_QUADS);
	// upper quad
	glVertex2i(0, borderHeight);
	glVertex2i(0, 0);
	glVertex2i(width, 0);
	glVertex2i(width, borderHeight);

	// lower quad
	glVertex2i(0, trueHeight);
	glVertex2i(0, trueHeight - borderHeight);
	glVertex2i(width, trueHeight - borderHeight);
	glVertex2i(width, trueHeight);
	glEnd();

	gl_RenderState.EnableTexture(true);

	glViewport(0, (trueHeight - height) / 2, width, height); 
}

//==========================================================================
//
// Draws a texture
//
//==========================================================================

void FGLRenderer::DrawTexture(FTexture *img, DCanvas::DrawParms &parms)
{
	double xscale = parms.destwidth / parms.texwidth;
	double yscale = parms.destheight / parms.texheight;
	double x = parms.x - parms.left * xscale;
	double y = parms.y - parms.top * yscale;
	double w = parms.destwidth;
	double h = parms.destheight;
	float u1, v1, u2, v2, r, g, b;
	float light = 1.f;

	FMaterial * gltex = FMaterial::ValidateTexture(img);

	if (parms.colorOverlay && (parms.colorOverlay & 0xffffff) == 0)
	{
		// Right now there's only black. Should be implemented properly later
		light = 1.f - APART(parms.colorOverlay)/255.f;
		parms.colorOverlay = 0;
	}

	if (!img->bHasCanvas)
	{
		if (!parms.alphaChannel) 
		{
			int translation = 0;
			if (parms.remap != NULL && !parms.remap->Inactive)
			{
				GLTranslationPalette * pal = static_cast<GLTranslationPalette*>(parms.remap->GetNative());
				if (pal) translation = -pal->GetIndex();
			}
			gltex->BindPatch(CM_DEFAULT, translation);
		}
		else 
		{
			// This is an alpha texture
			gltex->BindPatch(CM_SHADE, 0);
		}

		u1 = gltex->GetUL();
		v1 = gltex->GetVT();
		u2 = gltex->GetUR();
		v2 = gltex->GetVB();
	}
	else
	{
		gltex->Bind(CM_DEFAULT, 0, 0);
		u2=1.f;
		v2=-1.f;
		u1 = v1 = 0.f;
		gl_RenderState.SetTextureMode(TM_OPAQUE);
	}
	
	if (parms.flipX)
	{
		float temp = u1;
		u1 = u2;
		u2 = temp;
	}
	

	if (parms.windowleft > 0 || parms.windowright < parms.texwidth)
	{
		x += parms.windowleft * xscale;
		w -= (parms.texwidth - parms.windowright + parms.windowleft) * xscale;

		u1 = float(u1 + parms.windowleft / parms.texwidth);
		u2 = float(u2 - (parms.texwidth - parms.windowright) / parms.texwidth);
	}

	if (parms.style.Flags & STYLEF_ColorIsFixed)
	{
		r = RPART(parms.fillcolor)/255.0f;
		g = GPART(parms.fillcolor)/255.0f;
		b = BPART(parms.fillcolor)/255.0f;
	}
	else
	{
		r = g = b = light;
	}

	if(parms.color != -1)//[GEC]
	{
		r = RPART(parms.color)/255.0f;
		g = GPART(parms.color)/255.0f;
		b = BPART(parms.color)/255.0f;
	}
	
	// scissor test doesn't use the current viewport for the coordinates, so use real screen coordinates
	int btm = (SCREENHEIGHT - screen->GetHeight()) / 2;
	btm = SCREENHEIGHT - btm;

	glEnable(GL_SCISSOR_TEST);
	int space = (static_cast<OpenGLFrameBuffer*>(screen)->GetTrueHeight()-screen->GetHeight())/2;
	glScissor(parms.lclip, btm - parms.dclip + space, parms.rclip - parms.lclip, parms.dclip - parms.uclip);
	
	gl_SetRenderStyle(parms.style, !parms.masked, false);
	if (img->bHasCanvas)
	{
		gl_RenderState.SetTextureMode(TM_OPAQUE);
	}

	glColor4f(r, g, b, FIXED2FLOAT(parms.alpha));
	
	gl_RenderState.EnableAlphaTest(false);
	gl_RenderState.Apply();
	glBegin(GL_TRIANGLE_STRIP);
	glTexCoord2f(u1, v1);
	glVertex2d(x, y);
	glTexCoord2f(u1, v2);
	glVertex2d(x, y + h);
	glTexCoord2f(u2, v1);
	glVertex2d(x + w, y);
	glTexCoord2f(u2, v2);
	glVertex2d(x + w, y + h);
	glEnd();

	if (parms.colorOverlay)
	{
		gl_RenderState.SetTextureMode(TM_MASK);
		gl_RenderState.BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		gl_RenderState.BlendEquation(GL_FUNC_ADD);
		gl_RenderState.Apply();
		glColor4ub(RPART(parms.colorOverlay),GPART(parms.colorOverlay),BPART(parms.colorOverlay),APART(parms.colorOverlay));
		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(u1, v1);
		glVertex2d(x, y);
		glTexCoord2f(u1, v2);
		glVertex2d(x, y + h);
		glTexCoord2f(u2, v1);
		glVertex2d(x + w, y);
		glTexCoord2f(u2, v2);
		glVertex2d(x + w, y + h);
		glEnd();
	}

	gl_RenderState.EnableAlphaTest(true);
	
	glScissor(0, 0, screen->GetWidth(), screen->GetHeight());
	glDisable(GL_SCISSOR_TEST);
	gl_RenderState.SetTextureMode(TM_MODULATE);
	gl_RenderState.BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	gl_RenderState.BlendEquation(GL_FUNC_ADD);
}

//==========================================================================
//
//
//
//==========================================================================
void FGLRenderer::DrawLine(int x1, int y1, int x2, int y2, int palcolor, uint32 color)
{
	PalEntry p = color? (PalEntry)color : GPalette.BaseColors[palcolor];
	gl_RenderState.EnableTexture(false);
	gl_RenderState.Apply(true);
	glColor3ub(p.r, p.g, p.b);
	glBegin(GL_LINES);
	glVertex2i(x1, y1);
	glVertex2i(x2, y2);
	glEnd();
	gl_RenderState.EnableTexture(true);
}

//==========================================================================
//
//
//
//==========================================================================
void FGLRenderer::DrawPixel(int x1, int y1, int palcolor, uint32 color)
{
	PalEntry p = color? (PalEntry)color : GPalette.BaseColors[palcolor];
	gl_RenderState.EnableTexture(false);
	gl_RenderState.Apply(true);
	glColor3ub(p.r, p.g, p.b);
	glBegin(GL_POINTS);
	glVertex2i(x1, y1);
	glEnd();
	gl_RenderState.EnableTexture(true);
}

//===========================================================================
// 
//
//
//===========================================================================

void FGLRenderer::Dim(PalEntry color, float damount, int x1, int y1, int w, int h)
{
	float r, g, b;
	
	gl_RenderState.EnableTexture(false);
	gl_RenderState.BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	gl_RenderState.AlphaFunc(GL_GREATER,0);
	gl_RenderState.Apply(true);
	
	r = color.r/255.0f;
	g = color.g/255.0f;
	b = color.b/255.0f;
	
	glBegin(GL_TRIANGLE_FAN);
	glColor4f(r, g, b, damount);
	glVertex2i(x1, y1);
	glVertex2i(x1, y1 + h);
	glVertex2i(x1 + w, y1 + h);
	glVertex2i(x1 + w, y1);
	glEnd();
	
	gl_RenderState.EnableTexture(true);
}

//==========================================================================
//
//
//
//==========================================================================
void FGLRenderer::FlatFill (int left, int top, int right, int bottom, FTexture *src, bool local_origin)
{
	float fU1,fU2,fV1,fV2;

	FMaterial *gltexture=FMaterial::ValidateTexture(src);
	
	if (!gltexture) return;

	gltexture->Bind(CM_DEFAULT, 0, 0);
	
	// scaling is not used here.
	if (!local_origin)
	{
		fU1 = float(left) / src->GetWidth();
		fV1 = float(top) / src->GetHeight();
		fU2 = float(right) / src->GetWidth();
		fV2 = float(bottom) / src->GetHeight();
	}
	else
	{		
		fU1 = 0;
		fV1 = 0;
		fU2 = float(right-left) / src->GetWidth();
		fV2 = float(bottom-top) / src->GetHeight();
	}
	gl_RenderState.Apply();
	glBegin(GL_TRIANGLE_STRIP);
	glColor4f(1, 1, 1, 1);
	glTexCoord2f(fU1, fV1); glVertex2f(left, top);
	glTexCoord2f(fU1, fV2); glVertex2f(left, bottom);
	glTexCoord2f(fU2, fV1); glVertex2f(right, top);
	glTexCoord2f(fU2, fV2); glVertex2f(right, bottom);
	glEnd();
}

//==========================================================================
//
//
//
//==========================================================================
void FGLRenderer::Clear(int left, int top, int right, int bottom, int palcolor, uint32 color)
{
	int rt;
	int offY = 0;
	PalEntry p = palcolor==-1 || color != 0? (PalEntry)color : GPalette.BaseColors[palcolor];
	int width = right-left;
	int height= bottom-top;
	
	
	rt = screen->GetHeight() - top;
	
	int space = (static_cast<OpenGLFrameBuffer*>(screen)->GetTrueHeight()-screen->GetHeight())/2;	// ugh...
	rt += space;
	/*
	if (!m_windowed && (m_trueHeight != m_height))
	{
		offY = (m_trueHeight - m_height) / 2;
		rt += offY;
	}
	*/
	
	glEnable(GL_SCISSOR_TEST);
	glScissor(left, rt - height, width, height);
	
	glClearColor(p.r/255.0f, p.g/255.0f, p.b/255.0f, 0.f);
	glClear(GL_COLOR_BUFFER_BIT);
	glClearColor(0.f, 0.f, 0.f, 0.f);
	
	glDisable(GL_SCISSOR_TEST);
}

//==========================================================================
//
// D3DFB :: FillSimplePoly
//
// Here, "simple" means that a simple triangle fan can draw it.
//
//==========================================================================

void FGLRenderer::FillSimplePoly(FTexture *texture, FVector2 *points, int npoints,
	double originx, double originy, double scalex, double scaley,
	angle_t rotation, FDynamicColormap *colormap, int lightlevel)
{
	if (npoints < 3)
	{ // This is no polygon.
		return;
	}

	FMaterial *gltexture = FMaterial::ValidateTexture(texture);

	if (gltexture == NULL)
	{
		return;
	}

	FColormap cm;
	cm = colormap;

	lightlevel = gl_CalcLightLevel(lightlevel, 0, true);
	PalEntry pe = gl_CalcLightColor(lightlevel, cm.LightColor, cm.blendfactor, true);
	glColor3ub(pe.r, pe.g, pe.b);

	gltexture->Bind(cm.colormap);

	int i;
	float rot = float(rotation * M_PI / float(1u << 31));
	bool dorotate = rot != 0;

	float cosrot = cos(rot);
	float sinrot = sin(rot);

	//float yoffs = GatheringWipeScreen ? 0 : LBOffset;
	float uscale = float(1.f / (texture->GetScaledWidth() * scalex));
	float vscale = float(1.f / (texture->GetScaledHeight() * scaley));
	if (gltexture->tex->bHasCanvas)
	{
		vscale = 0 - vscale;
	}
	float ox = float(originx);
	float oy = float(originy);

	gl_RenderState.Apply();
	glBegin(GL_TRIANGLE_FAN);
	for (i = 0; i < npoints; ++i)
	{
		float u = points[i].X - 0.5f - ox;
		float v = points[i].Y - 0.5f - oy;
		if (dorotate)
		{
			float t = u;
			u = t * cosrot - v * sinrot;
			v = v * cosrot + t * sinrot;
		}
		glTexCoord2f(u * uscale, v * vscale);
		glVertex3f(points[i].X, points[i].Y /* + yoffs */, 0);
	}
	glEnd();
}



//==========================================================================
//
// Draws a fire texture
//
//==========================================================================

typedef struct
{
    byte r;
    byte g;
    byte b;
    byte a;

	void SetRGBA(byte rr, byte gg, byte bb, byte aa) 
	{ 
		r = bb;
		g = gg;
		b = rr;
		a = aa;
	}
} dPalette_t2;

int firetex[64 * 128];
unsigned char fireBuf[8192];
dPalette_t2  firePal[256];

int FIRESKYWIDTH = 64;
int FIRESKYHEIGHT = 128;

//
// R_SpreadFirePSX
//

static void R_SpreadFirePSX(byte* src1, byte* src2, int pixel, int counter, int* rand)
{
    int randIdx = 0;
    int randIdx2 = 0;
    byte *tmpSrc;
    
    if(pixel != 0) 
    {
		//[GEC] Like PsxDoom
        randIdx = (rndtable[*rand]);
        randIdx2 = (rndtable[*rand+1]);
        *rand = ((*rand+2) & 0xff);
		fndindex = *rand;
        
        tmpSrc = (src1 + (((counter - (randIdx & 3)) + 1) & (FIRESKYWIDTH-1)));
        
        *(byte*)(tmpSrc - FIRESKYWIDTH) = pixel - ((randIdx2 & 1));
    }
    else
    {
        *(byte*)(src2 - FIRESKYWIDTH) = 0;                                                                                                                      
    }
}

//
// Fire_Out
//

void Fire_Out(void)
{
	int i;
    for(i = ((FIRESKYWIDTH * FIRESKYHEIGHT) - FIRESKYWIDTH); i < (FIRESKYWIDTH*FIRESKYHEIGHT); i++) 
    {
        if(fireBuf[i] != 0){fireBuf[i] = (fireBuf[i] - 0x01);}
    }
}

//
// R_FirePSX
//

int Fire_Frame = 0;

static void R_FirePSX(byte *buffer, bool fireout)
{
	if(Fire_Frame > 200 && fireout)
    {
        if(Fire_Frame > 248){Fire_Out();}
        else if((Fire_Frame & 1) == 1){Fire_Out();}
    }

    int counter = 0;
    int rand = 0;
    int step = 0;
    int pixel = 0;
    byte *src;
    byte *srcoffset;

    rand = 0;
    step = 0;
    pixel = 0;

	Fire_Frame++;
    rand = fndindex;
    src = buffer;
    counter = 0;
    src += FIRESKYWIDTH;
    do 
    {  // width
        srcoffset = (src + counter);
        pixel = *(byte*)srcoffset;

        step = 2;

        R_SpreadFirePSX(src, srcoffset, pixel, counter, &rand);

        src += FIRESKYWIDTH;
        srcoffset += FIRESKYWIDTH;

        do 
        {  // height
            pixel = *(byte*)srcoffset;
            step += 2;

            R_SpreadFirePSX(src, srcoffset, pixel, counter, &rand);

            pixel = *(byte*)(srcoffset + FIRESKYWIDTH);
            src += FIRESKYWIDTH;
            srcoffset += FIRESKYWIDTH;

            R_SpreadFirePSX(src, srcoffset, pixel, counter, &rand);

            pixel = *(byte*)(srcoffset + FIRESKYWIDTH);
            src += FIRESKYWIDTH;
            srcoffset += FIRESKYWIDTH;
        }
        while(step < FIRESKYHEIGHT);

        
        counter++;
        src -= ((FIRESKYWIDTH*FIRESKYHEIGHT)-FIRESKYWIDTH);

    }
    while(counter < FIRESKYWIDTH);
}

//
// R_InitFirePSX
//

void FGLRenderer::R_InitFirePSX_()
{
    int i;

    memset(&firePal, 0, sizeof(dPalette_t2)*256);

	firePal[0x00].SetRGBA(0x00,0x00,0x00,0x00);
	firePal[0x01].SetRGBA(0x18,0x00,0x00,0xFF);
	firePal[0x02].SetRGBA(0x29,0x08,0x00,0xFF);
	firePal[0x03].SetRGBA(0x42,0x08,0x00,0xFF);
	firePal[0x04].SetRGBA(0x51,0x10,0x00,0xFF);
	firePal[0x05].SetRGBA(0x63,0x18,0x00,0xFF);
	firePal[0x06].SetRGBA(0x73,0x18,0x00,0xFF);
	firePal[0x07].SetRGBA(0x8C,0x21,0x00,0xFF);
	firePal[0x08].SetRGBA(0x9C,0x29,0x00,0xFF);
	firePal[0x09].SetRGBA(0xAD,0x39,0x00,0xFF);
	firePal[0x0A].SetRGBA(0xBD,0x42,0x00,0xFF);
	firePal[0x0B].SetRGBA(0xC6,0x42,0x00,0xFF);
	firePal[0x0C].SetRGBA(0xDE,0x4A,0x00,0xFF);
	firePal[0x0D].SetRGBA(0xDE,0x52,0x00,0xFF);
	firePal[0x0E].SetRGBA(0xDE,0x52,0x00,0xFF);
	firePal[0x0F].SetRGBA(0xD6,0x5A,0x00,0xFF);
	firePal[0x10].SetRGBA(0xD6,0x5A,0x00,0xFF);
	firePal[0x11].SetRGBA(0xD6,0x63,0x08,0xFF);
	firePal[0x12].SetRGBA(0xCE,0x6B,0x08,0xFF);
	firePal[0x13].SetRGBA(0xCE,0x73,0x08,0xFF);
	firePal[0x14].SetRGBA(0xCE,0x7B,0x08,0xFF);
	firePal[0x15].SetRGBA(0xCE,0x84,0x10,0xFF);
	firePal[0x16].SetRGBA(0xC6,0x84,0x10,0xFF);
	firePal[0x17].SetRGBA(0xC6,0x8C,0x10,0xFF);
	firePal[0x18].SetRGBA(0xC6,0x94,0x18,0xFF);
	firePal[0x19].SetRGBA(0xBD,0x9C,0x18,0xFF);
	firePal[0x1A].SetRGBA(0xBD,0x9C,0x18,0xFF);
	firePal[0x1B].SetRGBA(0xBD,0xA5,0x21,0xFF);
	firePal[0x1C].SetRGBA(0xBD,0xA5,0x21,0xFF);
	firePal[0x1D].SetRGBA(0xBD,0xAD,0x29,0xFF);
	firePal[0x1E].SetRGBA(0xB5,0xAD,0x29,0xFF);
	firePal[0x1F].SetRGBA(0xB5,0xB5,0x29,0xFF);
	firePal[0x20].SetRGBA(0xB5,0xB5,0x31,0xFF);
	firePal[0x21].SetRGBA(0xCE,0xCE,0x6B,0xFF);
	firePal[0x22].SetRGBA(0xDE,0xDE,0x9C,0xFF);
	firePal[0x23].SetRGBA(0xEF,0xEF,0xC6,0xFF);
	firePal[0x24].SetRGBA(0xFF,0xFF,0xFF,0xFF);

	for(i = 0x25; i <= 0xFE; i++) 
	{
	firePal[i].SetRGBA(0x00,0x00,0x00,0xFF);
    }
	firePal[0xFF].SetRGBA(0xFF,0xFF,0xFF,0xFF);

	for(i = 0; i < (64*128); i++) 
    {
        if(i == 0)
        fireBuf[i] = 0x01;      
        else if(i < 8128)
        fireBuf[i] = 0x00;
        else
        fireBuf[i] = 0x24;
    }

	Fire_Frame = 0;
}

//
// R_FirePSXTicker
//

void FGLRenderer::R_FirePSXTicker_(bool fireout)
{
	R_FirePSX(fireBuf,fireout);
}

void FGLRenderer::DrawFireTexture(FTexture *img, DCanvas::DrawParms &parms)
{
	double xscale = parms.destwidth / parms.texwidth;
	double yscale = parms.destheight / parms.texheight;
	double x = parms.x - parms.left * xscale;
	double y = parms.y - parms.top * yscale;
	double w = parms.destwidth;
	double h = parms.destheight;
	float u1, v1, u2, v2, r, g, b;
	float light = 1.f;

	FMaterial * gltex = FMaterial::ValidateTexture(img);

	if (parms.colorOverlay && (parms.colorOverlay & 0xffffff) == 0)
	{
		// Right now there's only black. Should be implemented properly later
		light = 1.f - APART(parms.colorOverlay)/255.f;
		parms.colorOverlay = 0;
	}

	if (!img->bHasCanvas)
	{
		if (!parms.alphaChannel) 
		{
			int translation = 0;
			if (parms.remap != NULL && !parms.remap->Inactive)
			{
				GLTranslationPalette * pal = static_cast<GLTranslationPalette*>(parms.remap->GetNative());
				if (pal) translation = -pal->GetIndex();
			}
			gltex->BindPatch(CM_DEFAULT, translation);
		}
		else 
		{
			// This is an alpha texture
			gltex->BindPatch(CM_SHADE, 0);
		}

		u1 = gltex->GetUL();
		v1 = gltex->GetVT();
		u2 = gltex->GetUR();
		v2 = gltex->GetVB();
	}
	else
	{
		gltex->Bind(CM_DEFAULT, 0, 0);
		u2=1.f;
		v2=-1.f;
		u1 = v1 = 0.f;
		gl_RenderState.SetTextureMode(TM_OPAQUE);
	}
	
	if (parms.flipX)
	{
		float temp = u1;
		u1 = u2;
		u2 = temp;
	}
	

	if (parms.windowleft > 0 || parms.windowright < parms.texwidth)
	{
		x += parms.windowleft * xscale;
		w -= (parms.texwidth - parms.windowright + parms.windowleft) * xscale;

		u1 = float(u1 + parms.windowleft / parms.texwidth);
		u2 = float(u2 - (parms.texwidth - parms.windowright) / parms.texwidth);
	}

	if (parms.style.Flags & STYLEF_ColorIsFixed)
	{
		r = RPART(parms.fillcolor)/255.0f;
		g = GPART(parms.fillcolor)/255.0f;
		b = BPART(parms.fillcolor)/255.0f;
	}
	else
	{
		r = g = b = light;
	}

	if(parms.color != -1)//[GEC]
	{
		r = RPART(parms.color)/255.0f;
		g = GPART(parms.color)/255.0f;
		b = BPART(parms.color)/255.0f;
	}
	
	//
    // copy fire pixel data to texture data array
    //
    memset(firetex, 0, sizeof(int) * FIRESKYWIDTH * FIRESKYHEIGHT);
    for(int i = 0; i < FIRESKYWIDTH * FIRESKYHEIGHT; i++)
	{
        byte rgb[4];

        rgb[0] = firePal[fireBuf[i]].r;
        rgb[1] = firePal[fireBuf[i]].g;
        rgb[2] = firePal[fireBuf[i]].b;
		rgb[3] = firePal[fireBuf[i]].a;

		firetex[i] =  MAKEARGB(rgb[3], rgb[0], rgb[1], rgb[2]);
    }

	FHardwareTexture *firetex2;
	firetex2 = new FHardwareTexture(64, 128, false, false, false, true);
	firetex2->CreateTexture(NULL, 64, 128, false, 0, CM_DEFAULT);
	glFlush();
	firetex2->Bind(0, CM_DEFAULT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexSubImage2D(GL_TEXTURE_2D,0,0,0,FIRESKYWIDTH,FIRESKYHEIGHT,GL_RGBA,GL_UNSIGNED_BYTE,firetex);

	// scissor test doesn't use the current viewport for the coordinates, so use real screen coordinates
	int btm = (SCREENHEIGHT - screen->GetHeight()) / 2;
	btm = SCREENHEIGHT - btm;

	glEnable(GL_SCISSOR_TEST);
	int space = (static_cast<OpenGLFrameBuffer*>(screen)->GetTrueHeight()-screen->GetHeight())/2;
	glScissor(parms.lclip, btm - parms.dclip + space, parms.rclip - parms.lclip, parms.dclip - parms.uclip);
	
	gl_SetRenderStyle(parms.style, !parms.masked, false);
	if (img->bHasCanvas)
	{
		gl_RenderState.SetTextureMode(TM_OPAQUE);
	}

	glColor4f(r, g, b, FIXED2FLOAT(parms.alpha));
	
	gl_RenderState.EnableAlphaTest(true);

	gl_RenderState.Apply();
	glBegin(GL_TRIANGLE_STRIP);
	glTexCoord2f(u1, v1);
	glVertex2d(x, y);
	glTexCoord2f(u1, v2);
	glVertex2d(x, y + h);
	glTexCoord2f(u2, v1);
	glVertex2d(x + w, y);
	glTexCoord2f(u2, v2);
	glVertex2d(x + w, y + h);
	glEnd();

	gl_RenderState.EnableAlphaTest(true);
	
	glScissor(0, 0, screen->GetWidth(), screen->GetHeight());
	glDisable(GL_SCISSOR_TEST);
	gl_RenderState.SetTextureMode(TM_MODULATE);
	gl_RenderState.BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	gl_RenderState.BlendEquation(GL_FUNC_ADD);

	firetex2->Clean(true);
}