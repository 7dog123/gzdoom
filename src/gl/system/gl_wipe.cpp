/*
** gl_wipe.cpp
** Screen wipe stuff
** (This uses immediate mode and the fixed function pipeline 
**  even if the new renderer is active)
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
#include "f_wipe.h"
#include "m_random.h"
#include "w_wad.h"
#include "v_palette.h"
#include "templates.h"
#include "vectors.h"
#include "v_video.h"
#include "v_font.h"

#include "gl/system/gl_interface.h"
#include "gl/renderer/gl_renderer.h"
#include "gl/renderer/gl_renderstate.h"
#include "gl/system/gl_framebuffer.h"
#include "gl/textures/gl_translate.h"
#include "gl/textures/gl_material.h"
#include "gl/utility/gl_templates.h"

#ifndef _WIN32
struct POINT {
  SDWORD x; 
  SDWORD y; 
};
struct RECT {
  SDWORD left; 
  SDWORD top; 
  SDWORD right; 
  SDWORD bottom; 
}; 
#endif

//===========================================================================
// 
//	Screen wipes
//
//===========================================================================

// TYPES -------------------------------------------------------------------

int LoadTicks;
class OpenGLFrameBuffer::Wiper_LoadingScreen : public OpenGLFrameBuffer::Wiper//[GEC]
{
public:
	Wiper_LoadingScreen();
	bool Run(int ticks, OpenGLFrameBuffer *fb);

/*private:
	int fadetics;
	int wipeFadeAlpha;*/
};

class OpenGLFrameBuffer::Wiper_FadeScreen : public OpenGLFrameBuffer::Wiper//[GEC]
{
public:
	Wiper_FadeScreen();
	bool Run(int ticks, OpenGLFrameBuffer *fb);

private:
	int fadetics;
	int wipeFadeAlpha;
};

class OpenGLFrameBuffer::Wiper_Melt64 : public OpenGLFrameBuffer::Wiper//[GEC]
{
public:
	Wiper_Melt64();
	bool Run(int ticks, OpenGLFrameBuffer *fb);

private:
	int Clock;
	int fadetics;
	int wipeFadeAlpha;
	float Y;
};

class OpenGLFrameBuffer::Wiper_Crossfade : public OpenGLFrameBuffer::Wiper
{
public:
	Wiper_Crossfade();
	bool Run(int ticks, OpenGLFrameBuffer *fb);

private:
	int Clock;
};

class OpenGLFrameBuffer::Wiper_Melt : public OpenGLFrameBuffer::Wiper
{
public:
	Wiper_Melt();
	bool Run(int ticks, OpenGLFrameBuffer *fb);

private:
	static const int WIDTH = 320, HEIGHT = 200;
	int y[WIDTH];
};

class OpenGLFrameBuffer::Wiper_Burn : public OpenGLFrameBuffer::Wiper
{
public:
	Wiper_Burn();
	~Wiper_Burn();
	bool Run(int ticks, OpenGLFrameBuffer *fb);

private:
	static const int WIDTH = 64, HEIGHT = 64;
	BYTE BurnArray[WIDTH * (HEIGHT + 5)];
	FHardwareTexture *BurnTexture;
	int Density;
	int BurnTime;
};

//==========================================================================
//
// OpenGLFrameBuffer :: WipeStartScreen
//
// Called before the current screen has started rendering. This needs to
// save what was drawn the previous frame so that it can be animated into
// what gets drawn this frame.
//
//==========================================================================

bool OpenGLFrameBuffer::WipeStartScreen(int type)
{
	switch (type)
	{
	case wipe_Burn:
		ScreenWipe = new Wiper_Burn;
		break;

	case wipe_Fade:
		ScreenWipe = new Wiper_Crossfade;
		break;

	case wipe_Melt:
		ScreenWipe = new Wiper_Melt;
		break;

	case wipe_Melt64://[GEC]
		ScreenWipe = new Wiper_Melt64;
		break;

	case wipe_FadeScreen://[GEC]
		ScreenWipe = new Wiper_FadeScreen;
		break;

	case wipe_LoadingScreen://[GEC]
		ScreenWipe = new Wiper_LoadingScreen;
		break;

	default:
		return false;
	}

	wipestartscreen = new FHardwareTexture(Width, Height, false, false, false, true);
	wipestartscreen->CreateTexture(NULL, Width, Height, false, 0, CM_DEFAULT);
	glFinish();
	wipestartscreen->Bind(0, CM_DEFAULT);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, Width, Height);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	return true;
}

//==========================================================================
//
// OpenGLFrameBuffer :: WipeEndScreen
//
// The screen we want to animate to has just been drawn.
//
//==========================================================================

void OpenGLFrameBuffer::WipeEndScreen()
{
	wipeendscreen = new FHardwareTexture(Width, Height, false, false, false, true);
	wipeendscreen->CreateTexture(NULL, Width, Height, false, 0, CM_DEFAULT);
	glFlush();
	wipeendscreen->Bind(0, CM_DEFAULT);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, Width, Height);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	Unlock();
}

//==========================================================================
//
// OpenGLFrameBuffer :: WipeDo
//
// Perform the actual wipe animation. The number of tics since the last
// time this function was called is passed in. Returns true when the wipe
// is over. The first time this function has been called, the screen is
// still locked from before and EndScene() still has not been called.
// Successive times need to call BeginScene().
//
//==========================================================================

bool OpenGLFrameBuffer::WipeDo(int ticks)
{
	// Sanity checks.
	if (wipestartscreen == NULL || wipeendscreen == NULL)
	{
		return true;
	}

	Lock(true);
	
	gl_RenderState.EnableTexture(true);
	gl_RenderState.EnableFog(false);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(false);

	bool done = ScreenWipe->Run(ticks, this);
	glDepthMask(true);
	//DrawLetterbox();
	return done;
}

//==========================================================================
//
// OpenGLFrameBuffer :: WipeCleanup
//
// Release any resources that were specifically created for the wipe.
//
//==========================================================================

void OpenGLFrameBuffer::WipeCleanup()
{
	if (ScreenWipe != NULL)
	{
		delete ScreenWipe;
		ScreenWipe = NULL;
	}
	if (wipestartscreen != NULL)
	{
		delete wipestartscreen;
		wipestartscreen = NULL;
	}
	if (wipeendscreen != NULL)
	{
		delete wipeendscreen;
		wipeendscreen = NULL;
	}
}

//==========================================================================
//
// OpenGLFrameBuffer :: Wiper Constructor
//
//==========================================================================

OpenGLFrameBuffer::Wiper::~Wiper()
{
}

// WIPE: FADESCREEN ---------------------------------------------------------

//==========================================================================
//
// OpenGLFrameBuffer :: Wiper_Crossfade Constructor
//
//==========================================================================

OpenGLFrameBuffer::Wiper_FadeScreen::Wiper_FadeScreen()
{
	fadetics = 8;
	wipeFadeAlpha = 0xff;
}

//==========================================================================
//
// OpenGLFrameBuffer :: Wiper_Crossfade :: Run
//
//==========================================================================

bool OpenGLFrameBuffer::Wiper_FadeScreen::Run(int ticks, OpenGLFrameBuffer *fb)
{
	//[GEC] Recrea el FadeScreen de Doom64 EX con exactitud
	bool done = false;

	float ur = fb->GetWidth() / FHardwareTexture::GetTexDimension(fb->GetWidth());
	float vb = fb->GetHeight() / FHardwareTexture::GetTexDimension(fb->GetHeight());

	//if(wipeFadeAlpha > 0)// FadeScreen
	//{
		if(wipeFadeAlpha < 0) 
		{
            //wipeFadeAlpha = 0;
			done = true;
        }

		gl_RenderState.BlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
		gl_RenderState.SetTextureMode(TM_OPAQUE);
		gl_RenderState.EnableAlphaTest(false);
		gl_RenderState.Apply();
		fb->wipestartscreen->Bind(0, CM_DEFAULT);

		float color = (float)(wipeFadeAlpha/255.f);

		glColor4f(color, color, color, 1.0f);
		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(0, vb);
		glVertex2i(0, 0);
		glTexCoord2f(0, 0);
		glVertex2i(0, fb->Height);
		glTexCoord2f(ur, vb);
		glVertex2i(fb->Width, 0);
		glTexCoord2f(ur, 0);
		glVertex2i(fb->Width, fb->Height);
		glEnd();

		wipeFadeAlpha -= fadetics;

		gl_RenderState.EnableAlphaTest(true);
		gl_RenderState.SetTextureMode(TM_MODULATE);
	//}
	//else done = true;

	return done;
}

// WIPE: MELT64 ---------------------------------------------------------

//==========================================================================
//
// OpenGLFrameBuffer :: Wiper_Melt64 Constructor
//
//==========================================================================

OpenGLFrameBuffer::Wiper_Melt64::Wiper_Melt64()
: Clock(0)
{
	Y = 0.f;
	fadetics = 6;
	wipeFadeAlpha = 0xff;
}

//==========================================================================
//
// OpenGLFrameBuffer :: Wiper_Melt64 :: Run
//
//==========================================================================

bool OpenGLFrameBuffer::Wiper_Melt64::Run(int ticks, OpenGLFrameBuffer *fb)
{
	//[GEC] Recrea el Melt de Doom64 EX con exactitud
	Clock += 2;
	bool done = false;

	float ur = fb->GetWidth() / FHardwareTexture::GetTexDimension(fb->GetWidth());
	float vb = fb->GetHeight() / FHardwareTexture::GetTexDimension(fb->GetHeight());

	if(Clock < 160)
	{
		gl_RenderState.BlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
		gl_RenderState.SetTextureMode(TM_OPAQUE);
		gl_RenderState.EnableAlphaTest(false);
		gl_RenderState.Apply();
		fb->wipestartscreen->Bind(0, CM_DEFAULT);
		glColor4f(1.f, 1.f, 1.f, 1.0f);
		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(0, vb);
		glVertex2i(0, 0);
		glTexCoord2f(0, 0);
		glVertex2i(0, fb->Height);
		glTexCoord2f(ur, vb);
		glVertex2i(fb->Width, 0);
		glTexCoord2f(ur, 0);
		glVertex2i(fb->Width, fb->Height);
		glEnd();

		gl_RenderState.BlendFunc(GL_SRC_ALPHA,GL_ONE);
		gl_RenderState.SetTextureMode(TM_MASK);
		gl_RenderState.Apply();
		fb->wipestartscreen->Bind(0, CM_DEFAULT);
		glColor4f(1.0, 0.0, 0.0, 0.003125);
		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(0, vb);
		glVertex2i(0, 0);
		glTexCoord2f(0, 0);
		glVertex2i(0, fb->Height);
		glTexCoord2f(ur, vb);
		glVertex2i(fb->Width, 0);
		glTexCoord2f(ur, 0);
		glVertex2i(fb->Width, fb->Height);
		glEnd();

		gl_RenderState.BlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
		gl_RenderState.SetTextureMode(TM_OPAQUE);
		gl_RenderState.Apply();
		fb->wipestartscreen->Bind(0, CM_DEFAULT);
		glColor4f(1.f, 1.f, 1.f, (float)(0x10/255.f));
		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(0, vb);
		glVertex2f(0, 0+(Y));
		glTexCoord2f(0, 0);
		glVertex2f(0, (float)fb->Height+(Y));
		glTexCoord2f(ur, vb);
		glVertex2f((float)fb->Width, 0+(Y));
		glTexCoord2f(ur, 0);
		glVertex2f((float)fb->Width, (float)fb->Height+(Y));
		glEnd();

		//
        // Mueve la pantalla hacia abajo. Sin limpiar el marco, debemos
        // obtener un buen efecto de fusión con el efecto HOM
        //
		Y += 1.006f;

		//
        // Actualiza buffer de pantalla
        //
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, fb->Width, fb->Height);

		gl_RenderState.EnableAlphaTest(true);
		gl_RenderState.SetTextureMode(TM_MODULATE);
		gl_RenderState.BlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	}
	else //if(wipeFadeAlpha > 0)// FadeScreen
	{
		if(wipeFadeAlpha < 0) 
		{
            //wipeFadeAlpha = 0;
			done = true;
        }

		gl_RenderState.BlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
		gl_RenderState.SetTextureMode(TM_OPAQUE);
		gl_RenderState.EnableAlphaTest(false);
		gl_RenderState.Apply();
		fb->wipestartscreen->Bind(0, CM_DEFAULT);

		float color = (float)(wipeFadeAlpha/255.f);

		glColor4f(color, color, color, 1.0f);
		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(0, vb);
		glVertex2i(0, 0);
		glTexCoord2f(0, 0);
		glVertex2i(0, fb->Height);
		glTexCoord2f(ur, vb);
		glVertex2i(fb->Width, 0);
		glTexCoord2f(ur, 0);
		glVertex2i(fb->Width, fb->Height);
		glEnd();

		wipeFadeAlpha -= fadetics;

		gl_RenderState.EnableAlphaTest(true);
		gl_RenderState.SetTextureMode(TM_MODULATE);
	}
	//else done = true;

	return done;
}


// WIPE: LOADINGSCREEN ---------------------------------------------------------

//==========================================================================
//
// OpenGLFrameBuffer :: Wiper_LoadingScreen Constructor
//
//==========================================================================

FTexture *tex;
OpenGLFrameBuffer::Wiper_LoadingScreen::Wiper_LoadingScreen()
{
	LoadTicks = 32;
	const BITFIELD texflags = 1 | 64;
	tex = TexMan.FindTexture ("LOADING",FTexture::TEX_MiscPatch, texflags);
}

//==========================================================================
//
// OpenGLFrameBuffer :: Wiper_Crossfade :: Run
//
// Fades the old screen into the new one over 32 ticks.
//
//==========================================================================

bool OpenGLFrameBuffer::Wiper_LoadingScreen::Run(int ticks, OpenGLFrameBuffer *fb)
{
	if(LoadTicks > 0)
	{
		float ur = fb->GetWidth() / FHardwareTexture::GetTexDimension(fb->GetWidth());
		float vb = fb->GetHeight() / FHardwareTexture::GetTexDimension(fb->GetHeight());

		gl_RenderState.SetTextureMode(TM_OPAQUE);
		gl_RenderState.EnableAlphaTest(false);
		gl_RenderState.Apply();
		fb->wipestartscreen->Bind(0, CM_DEFAULT);
		glColor4f(1.f, 1.f, 1.f, 1.f);
		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(0, vb);
		glVertex2i(0, 0);
		glTexCoord2f(0, 0);
		glVertex2i(0, fb->Height);
		glTexCoord2f(ur, vb);
		glVertex2i(fb->Width, 0);
		glTexCoord2f(ur, 0);
		glVertex2i(fb->Width, fb->Height);
		glEnd();

		if(tex)
		{
			screen->DrawTexture(tex, 0, 0,
				DTA_VirtualWidthF, 256.0,
				DTA_VirtualHeightF, 240.0,
				//DTA_CleanNoMove, true,
				TAG_DONE);
		}
		else
		{
			EColorRange color = CR_UNTRANSLATED;
			screen->DrawText (BigFont, color, (256 - BigFont->StringWidth("Loading"))/2, 120.0, "Loading", DTA_ResWidthF, 256, DTA_ResHeightF, 240, TAG_DONE);
		}

		LoadTicks--;
		return false;
	}

	return true;
}

// WIPE: CROSSFADE ---------------------------------------------------------

//==========================================================================
//
// OpenGLFrameBuffer :: Wiper_Crossfade Constructor
//
//==========================================================================

OpenGLFrameBuffer::Wiper_Crossfade::Wiper_Crossfade()
: Clock(0)
{
}

//==========================================================================
//
// OpenGLFrameBuffer :: Wiper_Crossfade :: Run
//
// Fades the old screen into the new one over 32 ticks.
//
//==========================================================================
bool OpenGLFrameBuffer::Wiper_Crossfade::Run(int ticks, OpenGLFrameBuffer *fb)
{
	Clock += ticks;

	float ur = fb->GetWidth() / FHardwareTexture::GetTexDimension(fb->GetWidth());
	float vb = fb->GetHeight() / FHardwareTexture::GetTexDimension(fb->GetHeight());

	gl_RenderState.SetTextureMode(TM_OPAQUE);
	gl_RenderState.EnableAlphaTest(false);
	gl_RenderState.Apply();
	fb->wipestartscreen->Bind(0, CM_DEFAULT);
	glColor4f(1.f, 1.f, 1.f, 1.f);
	glBegin(GL_TRIANGLE_STRIP);
	glTexCoord2f(0, vb);
	glVertex2i(0, 0);
	glTexCoord2f(0, 0);
	glVertex2i(0, fb->Height);
	glTexCoord2f(ur, vb);
	glVertex2i(fb->Width, 0);
	glTexCoord2f(ur, 0);
	glVertex2i(fb->Width, fb->Height);
	glEnd();

	fb->wipeendscreen->Bind(0, CM_DEFAULT);
	glColor4f(1.f, 1.f, 1.f, clamp(Clock/32.f, 0.f, 1.f));
	glBegin(GL_TRIANGLE_STRIP);
	glTexCoord2f(0, vb);
	glVertex2i(0, 0);
	glTexCoord2f(0, 0);
	glVertex2i(0, fb->Height);
	glTexCoord2f(ur, vb);
	glVertex2i(fb->Width, 0);
	glTexCoord2f(ur, 0);
	glVertex2i(fb->Width, fb->Height);
	glEnd();
	gl_RenderState.EnableAlphaTest(true);
	gl_RenderState.SetTextureMode(TM_MODULATE);

	return Clock >= 32;
}

//==========================================================================
//
// OpenGLFrameBuffer :: Wiper_Melt Constructor
//
//==========================================================================

OpenGLFrameBuffer::Wiper_Melt::Wiper_Melt()
{
	int i, r;

	// setup initial column positions
	// (y<0 => not ready to scroll yet)
	y[0] = -(M_Random() & 15);
	for (i = 1; i < WIDTH; ++i)
	{
		r = (M_Random()%3) - 1;
		y[i] = clamp(y[i-1] + r, -15, 0);
	}
}

//==========================================================================
//
// OpenGLFrameBuffer :: Wiper_Melt :: Run
//
// Fades the old screen into the new one over 32 ticks.
//
//==========================================================================

bool OpenGLFrameBuffer::Wiper_Melt::Run(int ticks, OpenGLFrameBuffer *fb)
{
	float ur = fb->GetWidth() / FHardwareTexture::GetTexDimension(fb->GetWidth());
	float vb = fb->GetHeight() / FHardwareTexture::GetTexDimension(fb->GetHeight());

	// Draw the new screen on the bottom.
	gl_RenderState.SetTextureMode(TM_OPAQUE);
	gl_RenderState.Apply();
	fb->wipeendscreen->Bind(0, CM_DEFAULT);
	glColor4f(1.f, 1.f, 1.f, 1.f);
	glBegin(GL_TRIANGLE_STRIP);
	glTexCoord2f(0, vb);
	glVertex2i(0, 0);
	glTexCoord2f(0, 0);
	glVertex2i(0, fb->Height);
	glTexCoord2f(ur, vb);
	glVertex2i(fb->Width, 0);
	glTexCoord2f(ur, 0);
	glVertex2i(fb->Width, fb->Height);
	glEnd();

	int i, dy;
	bool done = false;

	fb->wipestartscreen->Bind(0, CM_DEFAULT);
	// Copy the old screen in vertical strips on top of the new one.
	while (ticks--)
	{
		done = true;
		for (i = 0; i < WIDTH; i++)
		{
			if (y[i] < 0)
			{
				y[i]++;
				done = false;
			} 
			else if (y[i] < HEIGHT)
			{
				dy = (y[i] < 16) ? y[i]+1 : 8;
				y[i] = MIN(y[i] + dy, HEIGHT);
				done = false;
			}
			if (ticks == 0)
			{ // Only draw for the final tick.
				RECT rect;
				POINT dpt;

				dpt.x = i * fb->Width / WIDTH;
				dpt.y = MAX(0, y[i] * fb->Height / HEIGHT);
				rect.left = dpt.x;
				rect.top = 0;
				rect.right = (i + 1) * fb->Width / WIDTH;
				rect.bottom = fb->Height - dpt.y;
				if (rect.bottom > rect.top)
				{
					float tw = (float)FHardwareTexture::GetTexDimension(fb->Width);
					float th = (float)FHardwareTexture::GetTexDimension(fb->Height);
					rect.bottom = fb->Height - rect.bottom;
					rect.top = fb->Height - rect.top;
					glColor4f(1.f, 1.f, 1.f, 1.f);
					glBegin(GL_TRIANGLE_STRIP);
					glTexCoord2f(rect.left / tw, rect.top / th);
					glVertex2i(rect.left, rect.bottom);
					glTexCoord2f(rect.left / tw, rect.bottom / th);
					glVertex2i(rect.left, rect.top);
					glTexCoord2f(rect.right / tw, rect.top / th);
					glVertex2i(rect.right, rect.bottom);
					glTexCoord2f(rect.right / tw, rect.bottom / th);
					glVertex2i(rect.right, rect.top);
					glEnd();
				}
			}
		}
	}
	gl_RenderState.SetTextureMode(TM_MODULATE);
	return done;
}

//==========================================================================
//
// OpenGLFrameBuffer :: Wiper_Burn Constructor
//
//==========================================================================

OpenGLFrameBuffer::Wiper_Burn::Wiper_Burn()
{
	Density = 4;
	BurnTime = 0;
	memset(BurnArray, 0, sizeof(BurnArray));
	BurnTexture = NULL;
}

//==========================================================================
//
// OpenGLFrameBuffer :: Wiper_Burn Destructor
//
//==========================================================================

OpenGLFrameBuffer::Wiper_Burn::~Wiper_Burn()
{
	if (BurnTexture != NULL)
	{
		delete BurnTexture;
		BurnTexture = NULL;
	}
}

//==========================================================================
//
// OpenGLFrameBuffer :: Wiper_Burn :: Run
//
//==========================================================================

bool OpenGLFrameBuffer::Wiper_Burn::Run(int ticks, OpenGLFrameBuffer *fb)
{
	bool done;

	BurnTime += ticks;
	ticks *= 2;

	// Make the fire burn
	done = false;
	while (!done && ticks--)
	{
		Density = wipe_CalcBurn(BurnArray, WIDTH, HEIGHT, Density);
		done = (Density < 0);
	}

	if (BurnTexture != NULL) delete BurnTexture;
	BurnTexture = new FHardwareTexture(WIDTH, HEIGHT, false, false, false, true);

	// Update the burn texture with the new burn data
	BYTE rgb_buffer[WIDTH*HEIGHT*4];

	const BYTE *src = BurnArray;
	DWORD *dest = (DWORD *)rgb_buffer;
	for (int y = HEIGHT; y != 0; --y)
	{
		for (int x = WIDTH; x != 0; --x)
		{
			BYTE s = clamp<int>((*src++)*2, 0, 255);
			*dest++ = MAKEARGB(s,255,255,255);
		}
	}

	float ur = fb->GetWidth() / FHardwareTexture::GetTexDimension(fb->GetWidth());
	float vb = fb->GetHeight() / FHardwareTexture::GetTexDimension(fb->GetHeight());


	// Put the initial screen back to the buffer.
	gl_RenderState.SetTextureMode(TM_OPAQUE);
	gl_RenderState.EnableAlphaTest(false);
	gl_RenderState.Apply();
	fb->wipestartscreen->Bind(0, CM_DEFAULT);
	glColor4f(1.f, 1.f, 1.f, 1.f);
	glBegin(GL_TRIANGLE_STRIP);
	glTexCoord2f(0, vb);
	glVertex2i(0, 0);
	glTexCoord2f(0, 0);
	glVertex2i(0, fb->Height);
	glTexCoord2f(ur, vb);
	glVertex2i(fb->Width, 0);
	glTexCoord2f(ur, 0);
	glVertex2i(fb->Width, fb->Height);
	glEnd();

	gl_RenderState.SetTextureMode(TM_MODULATE);
	gl_RenderState.Apply(true);
	glActiveTexture(GL_TEXTURE1);
	glEnable(GL_TEXTURE_2D);

	// mask out the alpha channel of the wipeendscreen.
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE1);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE); 
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);

	glActiveTexture(GL_TEXTURE0);

	// Burn the new screen on top of it.
	glColor4f(1.f, 1.f, 1.f, 1.f);
	fb->wipeendscreen->Bind(1, CM_DEFAULT);
	//BurnTexture->Bind(0, CM_DEFAULT);

	BurnTexture->CreateTexture(rgb_buffer, WIDTH, HEIGHT, false, 0, CM_DEFAULT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


	glBegin(GL_TRIANGLE_STRIP);
	glMultiTexCoord2f(GL_TEXTURE0, 0, 0);
	glMultiTexCoord2f(GL_TEXTURE1, 0, vb);
	glVertex2i(0, 0);
	glMultiTexCoord2f(GL_TEXTURE0, 0, 1);
	glMultiTexCoord2f(GL_TEXTURE1, 0, 0);
	glVertex2i(0, fb->Height);
	glMultiTexCoord2f(GL_TEXTURE0, 1, 0);
	glMultiTexCoord2f(GL_TEXTURE1, ur, vb);
	glVertex2i(fb->Width, 0);
	glMultiTexCoord2f(GL_TEXTURE0, 1, 1);
	glMultiTexCoord2f(GL_TEXTURE1, ur, 0);
	glVertex2i(fb->Width, fb->Height);
	glEnd();

	glActiveTexture(GL_TEXTURE1);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glDisable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);

	// The fire may not always stabilize, so the wipe is forced to end
	// after an arbitrary maximum time.
	return done || (BurnTime > 40);
}
