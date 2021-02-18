/*
** v_blend.cpp
** Screen blending stuff
**
**---------------------------------------------------------------------------
** Copyright 1998-2006 Randy Heit
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

#include <assert.h>

#include "templates.h"
#include "sbar.h"
#include "c_cvars.h"
#include "c_dispatch.h"
#include "c_console.h"
#include "v_video.h"
#include "m_swap.h"
#include "w_wad.h"
#include "v_text.h"
#include "s_sound.h"
#include "gi.h"
#include "doomstat.h"
#include "g_level.h"
#include "d_net.h"
#include "colormatcher.h"
#include "v_palette.h"
#include "d_player.h"
#include "farchive.h"
#include "a_hexenglobal.h"
#include "r_utility.h"//[GEC]

// Palette indices. //[GEC]
// For damage/bonus red-/gold-shifts
#define NUMREDPALS			8
#define NUMBONUSPALS		4
#define ST_MAXDMGCOUNT  160
#define ST_MAXSTRCOUNT  32
#define ST_MAXBONCOUNT  100

CVAR (Int,   blend_type,			0,			CVAR_ARCHIVE);
//EXTERN_CVAR(Bool, blend_type);//[GEC]

// [RH] Amount of red flash for up to 114 damage points. Calculated by hand
//		using a logarithmic scale and my trusty HP48G.
static BYTE DamageToAlpha[114] =
{
	  0,   8,  16,  23,  30,  36,  42,  47,  53,  58,  62,  67,  71,  75,  79,
	 83,  87,  90,  94,  97, 100, 103, 107, 109, 112, 115, 118, 120, 123, 125,
	128, 130, 133, 135, 137, 139, 141, 143, 145, 147, 149, 151, 153, 155, 157,
	159, 160, 162, 164, 165, 167, 169, 170, 172, 173, 175, 176, 178, 179, 181,
	182, 183, 185, 186, 187, 189, 190, 191, 192, 194, 195, 196, 197, 198, 200,
	201, 202, 203, 204, 205, 206, 207, 209, 210, 211, 212, 213, 214, 215, 216,
	217, 218, 219, 220, 221, 221, 222, 223, 224, 225, 226, 227, 228, 229, 229,
	230, 231, 232, 233, 234, 235, 235, 236, 237
};


/*
=============
SV_AddBlend
[RH] This is from Q2.
=============
*/
CVAR(Bool, blendcombine, true, CVAR_ARCHIVE);//[GEC] Shader

void V_AddBlend (float r, float g, float b, float a, float v_blend[4])
{
	float a2, a3;
	BYTE R, G, B, A;

	if (a <= 0)
		return;
	a2 = v_blend[3] + (1-v_blend[3])*a;	// new total alpha
	a3 = v_blend[3]/a2;		// fraction of color from old

	v_blend[0] = v_blend[0]*a3 + r*(1-a3);
	v_blend[1] = v_blend[1]*a3 + g*(1-a3);
	v_blend[2] = v_blend[2]*a3 + b*(1-a3);
	v_blend[3] = a2;

	//no combine
	if(!blendcombine)
	{
		v_blend[0] = r;
		v_blend[1] = g;
		v_blend[2] = b;
		v_blend[3] = a;
	}

	if(blend_type != 0)
	{
		if(psx_blend)//PSX DOOM
		{
			R = (BYTE)(v_blend[0] * 255);
			G = (BYTE)(v_blend[1] * 255);
			B = (BYTE)(v_blend[2] * 255);
			A = (BYTE)(v_blend[3] * 255);
		}
		else// DOOM 64
		{
			float R1 = (float)((v_blend[0] * v_blend[3]) + (0.0f * 1.0f));
			float G1 = (float)((v_blend[1] * v_blend[3]) + (0.0f * 1.0f));
			float B1 = (float)((v_blend[2] * v_blend[3]) + (0.0f * 1.0f));

			//R1 = clamp<float>(R1, 0.0, 1.0);
			//G1 = clamp<float>(R1, 0.0, 1.0);
			//B1 = clamp<float>(R1, 0.0, 1.0);

			R = (BYTE)(R1 * 255)& 0xff;
			G = (BYTE)(G1 * 255)& 0xff;
			B = (BYTE)(B1 * 255)& 0xff;
			A = (BYTE) 0xff;
		}

		if ((players[consoleplayer].camera->player != NULL))//[GEC]
		{
			flashcolor = PalEntry(A & 0xff, R & 0xff, G & 0xff, B & 0xff);//[GEC]
		}
	}
}

//---------------------------------------------------------------------------
//
// BlendView
//
//---------------------------------------------------------------------------

void V_AddPlayerBlend (player_t *CPlayer, float blend[4], float maxinvalpha, int maxpainblend)
{
	psx_blend = (blend_type == 1 ? true : false);//[GEC]
	flashcolor = PalEntry(0x00,0x00,0x00,0x00);//[GEC]
	int	bcnt = 0;

	int cnt;

	// [RH] All powerups can affect the screen blending now
	for (AInventory *item = CPlayer->mo->Inventory; item != NULL; item = item->Inventory)
	{
		if(gameinfo.playerblendsystem == 1 || gameinfo.playerblendsystem == 2)//[GEC] PsxDoom || Doom64
		{
			if (item->IsKindOf (RUNTIME_CLASS(APowerStrength))){break;}
			else if (item->IsKindOf (RUNTIME_CLASS(APowerIronFeet))){break;}
			else if (item->IsKindOf (RUNTIME_CLASS(APowerInvulnerable))){break;}
		}

		PalEntry color = item->GetBlend ();
		if (color.a != 0)
		{
			V_AddBlend (color.r/255.f, color.g/255.f, color.b/255.f, color.a/255.f, blend);
			if (color.a/255.f > maxinvalpha) maxinvalpha = color.a/255.f;
		}
	}

	if (CPlayer->bonuscount)
	{
		cnt = CPlayer->bonuscount << 3;

		float alpha = (cnt > 128 ? 0.5f : cnt / 255.f);

		if(gameinfo.playerblendsystem == 1)//[GEC] PsxDoom
		{
			int palette = ((CPlayer->bonuscount) + 7) >> 3;

			if (palette >= NUMBONUSPALS)
				palette = NUMBONUSPALS-1;

			alpha = (float)((1.0 * palette)/8);
		}
		else if(gameinfo.playerblendsystem == 2)//[GEC] doom64
		{
			/*int c1 = (CPlayer->bonuscount + 8) >> 3;
			int c2;

			if(c1 > ST_MAXBONCOUNT) {
				c1 = ST_MAXBONCOUNT;
			}*/
			//c2 = (((c1 << 2) + c1) << 1);

			int c1 = (CPlayer->bonuscount + 8) >> 1;
			alpha = (float)((c1 & 0xff)/255.f);
		}
		
		V_AddBlend (RPART(gameinfo.pickupcolor)/255.f, GPART(gameinfo.pickupcolor)/255.f, 
					BPART(gameinfo.pickupcolor)/255.f, alpha, blend);
	}

	if(gameinfo.playerblendsystem == 1 || gameinfo.playerblendsystem == 2)//[GEC] PsxDoom || Doom64
	{
		for (AInventory *item = CPlayer->mo->Inventory; item != NULL; item = item->Inventory)
		{
			if (item->IsKindOf (RUNTIME_CLASS(APowerIronFeet)))
			{
				PalEntry color = item->GetBlend ();

				if (color.a != 0)
				{
					V_AddBlend (color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f, blend);
					if (color.a/255.f > maxinvalpha) maxinvalpha = color.a/255.f;
				}
				break;
			}
		}
	}

	cnt = CPlayer->damagecount;

	if(gameinfo.playerblendsystem == 1 || gameinfo.playerblendsystem == 2)//[GEC] PsxDoom || Doom64
	{
		for (AInventory *item = CPlayer->mo->Inventory; item != NULL; item = item->Inventory)
		{
			if (item->IsKindOf (RUNTIME_CLASS(APowerStrength)))
			{
				PalEntry color = item->GetBlend ();

				bcnt = color.a;

				if(gameinfo.playerblendsystem == 1)
				{
					if ((cnt))
					{
						int palette = ((cnt)+7)>>3;
					
						if (palette >= NUMREDPALS)
							palette = NUMREDPALS-1;

						int alpha = (int)(1.0 * palette/9) * 255;
						if(alpha < bcnt)
						{
							cnt = 0;
						}
						else
						{
							break;
						}
					}
				}

				if (color.a != 0)
				{
					V_AddBlend (color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f, blend);
					if (color.a/255.f > maxinvalpha) maxinvalpha = color.a/255.f;
				}
				break;
			}
		}
	}

	PalEntry painFlash = CPlayer->mo->DamageFade;
	CPlayer->mo->GetClass()->ActorInfo->GetPainFlash(CPlayer->mo->DamageTypeReceived, &painFlash);

	if(gameinfo.playerblendsystem == 1 && (painFlash.a != 0))//[GEC] PsxDoom
	{
		if ((cnt))
		{
			int palette = ((cnt)+7)>>3;
		
			if (palette >= NUMREDPALS)
				palette = NUMREDPALS-1;

			//[GEC] DOOM STYLE
			float alpha = (float)(1.0 * palette/9);
			V_AddBlend (painFlash.r / 255.f, painFlash.g / 255.f, painFlash.b / 255.f, alpha, blend);
		}
	}
	else if(gameinfo.playerblendsystem == 2 && (painFlash.a != 0))//[GEC] Doom64
	{
		if(cnt)
		{
			if(cnt)
			{
				if(cnt > ST_MAXDMGCOUNT)
				{
					cnt = ST_MAXDMGCOUNT;
				}
			}

			// take priority based on value
			if(cnt > bcnt)
			{
				float alpha = (float)((cnt / 255.f));
				V_AddBlend (painFlash.r / 255.f, painFlash.g / 255.f, painFlash.b / 255.f, alpha, blend);
			}
		}
	}
	else if (painFlash.a != 0)//Zdoom
	{
		cnt = DamageToAlpha[MIN (113, CPlayer->damagecount * painFlash.a / 255)];
			
		if (cnt)
		{
			if (cnt > maxpainblend)
				cnt = maxpainblend;

			V_AddBlend (painFlash.r / 255.f, painFlash.g / 255.f, painFlash.b / 255.f, cnt / 255.f, blend);
		}
	}

	// Unlike Doom, I did not have any utility source to look at to find the
	// exact numbers to use here, so I've had to guess by looking at how they
	// affect the white color in Hexen's palette and picking an alpha value
	// that seems reasonable.
	// [Gez] The exact values could be obtained by looking how they affect
	// each color channel in Hexen's palette.

	if (CPlayer->poisoncount)
	{
		cnt = MIN (CPlayer->poisoncount, 64);
		if (paletteflash & PF_POISON)
		{
			V_AddBlend(44/255.f, 92/255.f, 36/255.f, ((cnt + 7) >> 3) * 0.1f, blend);
		}
		else
		{
			V_AddBlend (0.04f, 0.2571f, 0.f, cnt/93.2571428571f, blend);
		}

	}

	if (CPlayer->hazardcount)
	{
		if (paletteflash & PF_HAZARD)
		{
			if (CPlayer->hazardcount > 16*TICRATE || (CPlayer->hazardcount & 8))
			{
				V_AddBlend (0.f, 1.f, 0.f, 0.125f, blend);
			}
		}
		else
		{
			cnt= MIN(CPlayer->hazardcount/8, 64);
			V_AddBlend (0.f, 0.2571f, 0.f, cnt/93.2571428571f, blend);
		}
	}

	if (CPlayer->mo->DamageType == NAME_Ice)
	{
		if (paletteflash & PF_ICE)
		{
			V_AddBlend(0.f, 0.f, 224/255.f, 0.5f, blend);
		}
		else
		{
			V_AddBlend (0.25f, 0.25f, 0.853f, 0.4f, blend);
		}		
	}

	if (CPlayer->bfgticdown)//[GEC]
	{
		PalEntry BFGFlash = CPlayer->bfgcolor;
		V_AddBlend(BFGFlash.r / 255.f, BFGFlash.g/ 255.f,
			BFGFlash.b/ 255.f, CPlayer->bfgticdown/ 255.f, blend);
	}

	if(gameinfo.playerblendsystem == 1 || gameinfo.playerblendsystem == 2)//[GEC] PsxDoom || Doom64
	{
		for (AInventory *item = CPlayer->mo->Inventory; item != NULL; item = item->Inventory)
		{
			if (item->IsKindOf (RUNTIME_CLASS(APowerInvulnerable)))
			{
				PalEntry color = item->GetBlend ();

				if (color.a != 0)
				{
					V_AddBlend (color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f, blend);
					if (color.a/255.f > maxinvalpha) maxinvalpha = color.a/255.f;
				}
				else
				{
					if(!blendcombine)
					{
						flashcolor = PalEntry(0x00,0x00,0x00,0x00);//[GEC]
					}
				}
				break;
			}
		}
	}
	
	// cap opacity if desired
	if (blend[3] > maxinvalpha) blend[3] = maxinvalpha;
}
