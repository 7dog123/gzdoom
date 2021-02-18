/*
** intermission.cpp
** Framework for intermissions (text screens, slideshows, etc)
**
**---------------------------------------------------------------------------
** Copyright 2010 Christoph Oelckers
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

#include "doomtype.h"
#include "doomstat.h"
#include "d_event.h"
#include "w_wad.h"
#include "gi.h"
#include "v_video.h"
#include "v_palette.h"
#include "d_main.h"
#include "gstrings.h"
#include "intermission/intermission.h"
#include "actor.h"
#include "d_player.h"
#include "r_state.h"
#include "r_data/r_translate.h"
#include "c_bind.h"
#include "g_level.h"
#include "p_conversation.h"
#include "menu/menu.h"
#include "d_net.h"

FIntermissionDescriptorList IntermissionDescriptors;

IMPLEMENT_CLASS(DIntermissionScreen)
IMPLEMENT_CLASS(DIntermissionScreenFader)
IMPLEMENT_CLASS(DIntermissionScreenText)
IMPLEMENT_CLASS(DIntermissionScreenCast)
IMPLEMENT_CLASS(DIntermissionScreenScroller)
IMPLEMENT_POINTY_CLASS(DIntermissionController)
	DECLARE_POINTER(mScreen)
END_POINTERS

extern int		NoWipe;

//==========================================================================
//
//
//
//==========================================================================

static int mPicResW = 0;	//[GEC] 
static int mPicResH = 0;	//[GEC]
static int mPicX = 0;		//[GEC] 
static int mPicY = 0;		//[GEC]
static gamestate_t	mWipeOut = GS_FINALE;//[GEC]
FString NextMusic = "";//[GEC]

void DIntermissionScreen::Init(FIntermissionAction *desc, bool first)
{
	int lumpnum;

	if (desc->mCdTrack == 0 || !S_ChangeCDMusic (desc->mCdTrack, desc->mCdId))
	{
		if (desc->mMusic.IsEmpty())
		{
			// only start the default music if this is the first action in an intermission
			if (first) S_ChangeMusic (gameinfo.finaleMusic, gameinfo.finaleOrder, desc->mMusicLooping);
		}
		else
		{
			bool MusicLooping = desc->mMusicLooping;
			if (desc->mNextMusic.IsNotEmpty())//[GEC]
				MusicLooping = false;

			S_ChangeMusic (desc->mMusic, desc->mMusicOrder, MusicLooping);
			
		}
	}
	mDuration = desc->mDuration;

	if(desc->mNextMusic.IsNotEmpty())//[GEC]
		NextMusic = desc->mNextMusic;

	const char *texname = desc->mBackground;
	if (*texname == '@')
	{
		char *pp;
		unsigned int v = strtoul(texname+1, &pp, 10) - 1;
		if (*pp == 0 && v < gameinfo.finalePages.Size())
		{
			texname = gameinfo.finalePages[v].GetChars();
		}
		else if (gameinfo.finalePages.Size() > 0)
		{
			texname = gameinfo.finalePages[0].GetChars();
		}
		else
		{
			texname = gameinfo.TitlePage.GetChars();
		}
	}
	else if (*texname == '$')
	{
		texname = GStrings[texname+1];
	}
	if (texname[0] != 0)
	{
		mBackground = TexMan.CheckForTexture(texname, FTexture::TEX_MiscPatch);
		mFlatfill = desc->mFlatfill;
	}
	S_Sound (CHAN_VOICE | CHAN_UI, desc->mSound, 1.0f, ATTN_NONE);
	if (desc->mPalette.IsNotEmpty() && (lumpnum = Wads.CheckNumForFullName(desc->mPalette, true)) > 0)
	{
		PalEntry *palette;
		const BYTE *orgpal;
		FMemLump lump;
		int i;

		lump = Wads.ReadLump (lumpnum);
		orgpal = (BYTE *)lump.GetMem();
		palette = screen->GetPalette ();
		for (i = 256; i > 0; i--, orgpal += 3)
		{
			*palette++ = PalEntry (orgpal[0], orgpal[1], orgpal[2]);
		}
		screen->UpdatePalette ();
		mPaletteChanged = true;
		NoWipe = 1;
		M_EnableMenu(false);
	}
	mOverlays.Resize(desc->mOverlays.Size());
	for (unsigned i=0; i < mOverlays.Size(); i++)
	{
		mOverlays[i].x = desc->mOverlays[i].x;
		mOverlays[i].y = desc->mOverlays[i].y;
		mOverlays[i].mCondition = desc->mOverlays[i].mCondition;
		mOverlays[i].mPic = TexMan.CheckForTexture(desc->mOverlays[i].mName, FTexture::TEX_MiscPatch);
	}
	mTicker = 0;
	Run_mScrollTicker = false;	//[GEC] Run_mScrollTicker
	mScrollTicker = 0;			//[GEC] mScrollTicker
	Run_mAlphaTicker = false;	//[GEC] Run_mAlphaTicker
	mAlphaTicker = 0;			//[GEC] mAlphaTicker
	mAlpha = 0;					//[GEC] mAlpha Colector
}


int DIntermissionScreen::Responder (event_t *ev)
{
	if (ev->type == EV_KeyDown)
	{
		/*if(mWipeOut != GS_FINALE)//[GEC]
			wipegamestate = mWipeOut;
		else
			NoWipe = 30;*/
		return -1;
	}
	return 0;
}

int DIntermissionScreen::Ticker ()
{
	//Printf("mNextMusic %s\n", mNextMusic.GetChars());
	//Printf("mDuration %d\n", mDuration);

	if (++mTicker >= mDuration && mDuration > 0) return -1;
	if(Run_mScrollTicker)//[GEC] mScrollTicker
	{
		mScrollTicker++;
	}
	if(Run_mAlphaTicker)//[GEC] mAlphaTicker
	{
		if(mAlphaTicker >= FLOAT2FIXED(0xFF))
		{
			mAlphaTicker = FLOAT2FIXED(0xFF);
		}
		else
		{
			mAlphaTicker += FLOAT2FIXED(mAlpha);
		}
	}
	return 0;
}

bool DIntermissionScreen::CheckOverlay(int i)
{
	if (mOverlays[i].mCondition == NAME_Multiplayer && !multiplayer) return false;
	else if (mOverlays[i].mCondition != NAME_None)
	{
		if (multiplayer || players[0].mo == NULL) return false;
		const PClass *cls = PClass::FindClass(mOverlays[i].mCondition);
		if (cls == NULL) return false;
		if (!players[0].mo->IsKindOf(cls)) return false;
	}
	return true;
}

void DIntermissionScreen::Drawer ()
{
	if (NextMusic.IsNotEmpty())
	{
		//Printf("mNextMusic %s play %d\n", NextMusic.GetChars(),S_CheckMusicPlaying());
		if(!S_CheckMusicPlaying())
		{
			S_ChangeMusic (NextMusic, 0, true);
			NextMusic = NULL;
		}
	}

	if (mBackground.isValid())
	{
		if (!mFlatfill)
		{
			//Printf("mPicResW %d mPicResH %d\n",mPicResW,mPicResH);
			if(mPicResW == 0 && mPicResH == 0)//[GEC]
			{
				screen->DrawTexture (TexMan[mBackground], mPicX, mPicX, DTA_Fullscreen, true, TAG_DONE);//[GEC]
			}
			else
			{
				screen->Clear (0, 0, SCREENWIDTH, SCREENHEIGHT, 0, 0);//[GEC] Refresh view

				double rx = (double)mPicX;
				double ry = (double)mPicY;
				double rw = TexMan[mBackground]->GetScaledWidthDouble();
				double rh = TexMan[mBackground]->GetScaledHeightDouble();

				screen->VirtualToRealCoords(rx, ry, rw, rh, mPicResW, mPicResH, true);

				screen->DrawTexture (TexMan[mBackground], rx, ry,
					DTA_DestWidthF, rw, DTA_DestHeightF, rh, TAG_DONE);
			}
		}
		else
		{
			screen->FlatFill (0,0, SCREENWIDTH, SCREENHEIGHT, TexMan[mBackground]);
		}
	}
	else
	{
		screen->Clear (0, 0, SCREENWIDTH, SCREENHEIGHT, 0, 0);
	}
	for (unsigned i=0; i < mOverlays.Size(); i++)
	{
		if (CheckOverlay(i))
			screen->DrawTexture (TexMan[mOverlays[i].mPic], mOverlays[i].x, mOverlays[i].y, DTA_320x200, true, TAG_DONE);
	}
	if (!mFlatfill) screen->FillBorder (NULL);
}

void DIntermissionScreen::Destroy()
{
	if (mPaletteChanged)
	{
		PalEntry *palette;
		int i;

		palette = screen->GetPalette ();
		for (i = 0; i < 256; ++i)
		{
			palette[i] = GPalette.BaseColors[i];
		}
		screen->UpdatePalette ();
		NoWipe = 5;
		mPaletteChanged = false;
		M_EnableMenu(true);
	}
	S_StopSound(CHAN_VOICE);
	Super::Destroy();
}

//==========================================================================
//
//
//
//==========================================================================

void DIntermissionScreenFader::Init(FIntermissionAction *desc, bool first)
{
	Super::Init(desc, first);
	mType = static_cast<FIntermissionActionFader*>(desc)->mFadeType;
}

//===========================================================================
//
// FadePic
//
//===========================================================================

int DIntermissionScreenFader::Responder (event_t *ev)
{
	if (ev->type == EV_KeyDown)
	{
		V_SetBlend(0,0,0,0);
		return -1;
	}
	return Super::Responder(ev);
}

int DIntermissionScreenFader::Ticker ()
{
	if (mFlatfill || !mBackground.isValid()) return -1;
	return Super::Ticker();
}

void DIntermissionScreenFader::Drawer ()
{
	if (!mFlatfill && mBackground.isValid())
	{
		double factor = clamp(double(mTicker) / mDuration, 0., 1.);
		if (mType == FADE_In) factor = 1.0 - factor;
		int color = MAKEARGB(xs_RoundToInt(factor*255), 0,0,0);

		if (screen->Begin2D(false))
		{
			screen->DrawTexture (TexMan[mBackground], 0, 0, DTA_Fullscreen, true, DTA_ColorOverlay, color, TAG_DONE);
			for (unsigned i=0; i < mOverlays.Size(); i++)
			{
				if (CheckOverlay(i))
					screen->DrawTexture (TexMan[mOverlays[i].mPic], mOverlays[i].x, mOverlays[i].y, DTA_320x200, true, DTA_ColorOverlay, color, TAG_DONE);
			}
			screen->FillBorder (NULL);
		}
		else
		{
			V_SetBlend (0,0,0,int(256*factor));
			Super::Drawer();
		}
	}
}

//==========================================================================
//
//
//
//==========================================================================

void DIntermissionScreenText::Init(FIntermissionAction *desc, bool first)
{
	Super::Init(desc, first);
	mText = static_cast<FIntermissionActionTextscreen*>(desc)->mText;
	if (mText[0] == '$') mText = GStrings(&mText[1]);
	mTextSpeed = static_cast<FIntermissionActionTextscreen*>(desc)->mTextSpeed;
	mTextX = static_cast<FIntermissionActionTextscreen*>(desc)->mTextX;
	if (mTextX < 0) mTextX =gameinfo.TextScreenX;
	mTextY = static_cast<FIntermissionActionTextscreen*>(desc)->mTextY;
	if (mTextY < 0) mTextY =gameinfo.TextScreenY;
	mTextLen = (int)strlen(mText);
	mTextDelay = static_cast<FIntermissionActionTextscreen*>(desc)->mTextDelay;
	mTextColor = static_cast<FIntermissionActionTextscreen*>(desc)->mTextColor;
	// For text screens, the duration only counts when the text is complete.
	if (mDuration > 0) mDuration += mTextDelay + mTextSpeed * mTextLen;

	//----------[GEC] Stuff
	mCount = 0;

	// [GEC] Count number of Xrows in this text
	int c;
	for (mNumRows = 0, c = 0; mText[c] != '\0'; ++c)
	{
		mNumRows += (mText[c] == '\n');
	}
	mNumRows += 1;
	
	// [GEC] Set Size on Xrows
	mAlpharows = new fixed_t[mNumRows];
	mTextCount = new int[mNumRows];
	mTextCount2 = new int[mNumRows];
	for (int k = 0; k < mNumRows; k++)
	{
		mAlpharows[k] = 0;
		mTextCount[k] = 0;
		mTextCount2[k] = 0;

		if(mFadeText)
			mAlpharows[k] = 0;
		else
			mAlpharows[k] = FLOAT2FIXED(0xFF);
	}

	mFontName = static_cast<FIntermissionActionTextscreen*>(desc)->m_FontName;
	mTextAdd = static_cast<FIntermissionActionTextscreen*>(desc)->m_TextAdd;
	mTextResW = static_cast<FIntermissionActionTextscreen*>(desc)->m_TextResW;
	mTextResH = static_cast<FIntermissionActionTextscreen*>(desc)->m_TextResH;

	int TextSpeed = static_cast<FIntermissionActionTextscreen*>(desc)->m_TextSpeed;
	int TextDelay = static_cast<FIntermissionActionTextscreen*>(desc)->m_TextDelay;
	if(TextSpeed != -1) mTextSpeed  = TextSpeed;
	if(TextDelay != -1) mTextDelay  = TextDelay;

	int TextX = static_cast<FIntermissionActionTextscreen*>(desc)->m_TexX;
	int TextY = static_cast<FIntermissionActionTextscreen*>(desc)->m_TexY;
	if(TextX != 0) mTextX  = TextX;
	if(TextY != 0) mTextY  = TextY;
	
	mCenterText = static_cast<FIntermissionActionTextscreen*>(desc)->m_CenterText;
	mFadeText = static_cast<FIntermissionActionTextscreen*>(desc)->m_FadeText;
	mPlainText = static_cast<FIntermissionActionTextscreen*>(desc)->m_PlainText;
	mPlainSync = static_cast<FIntermissionActionTextscreen*>(desc)->m_PlainSync;
	mScrollText = static_cast<FIntermissionActionTextscreen*>(desc)->m_ScrollText;
	mScrollTextSpeed = static_cast<FIntermissionActionTextscreen*>(desc)->m_ScrollTextSpeed;
	mScrollTextDirection = static_cast<FIntermissionActionTextscreen*>(desc)->m_ScrollTextDirection;
	mScrollTextTime = static_cast<FIntermissionActionTextscreen*>(desc)->m_ScrollTextTime;
	if(mPlainSync) mWait_Aplha = true;
	else mWait_Aplha = false;
	mRowPadding = static_cast<FIntermissionActionTextscreen*>(desc)->m_RowPadding;

	mPicResW = static_cast<FIntermissionActionTextscreen*>(desc)->m_PicResW;
	mPicResH = static_cast<FIntermissionActionTextscreen*>(desc)->m_PicResH;
	mPicX = static_cast<FIntermissionActionTextscreen*>(desc)->m_PicX;
	mPicY = static_cast<FIntermissionActionTextscreen*>(desc)->m_PicY;
	mWipeOut = static_cast<FIntermissionActionTextscreen*>(desc)->m_WipeOut;

	mWaitScroll = true;			//Default true
	mButtonPressed = false;		//Default false
	mskip = false;				//Default false

	mNoSkip = static_cast<FIntermissionActionTextscreen*>(desc)->m_NoSkip;
	mAutoSkip = static_cast<FIntermissionActionTextscreen*>(desc)->m_AutoSkip;

	if(mFadeText)
	{
		mAlphaTicker = 0; // [GEC] Reset mAlphaTicker
		Run_mAlphaTicker = true;
	}

	if(mScrollText)
	{
		mScrollTicker = 0;			//[GEC] Reset mScrollTicker
		Run_mScrollTicker = false;	//[GEC]
	}

	
	//mNoSkip = true;
	//Printf("mRowPadding %d\n",mRowPadding);

	//m_ScrollTextTime = 100;
	

	//mTextSpeed = 4;
	//mTextDelay = 7;
	/*Printf("mTextSpeed %d\n",mTextSpeed);
	Printf("mTextDelay %d\n",mTextDelay);
	Printf("mTextW %d\n",mTextW);
	Printf("mTextH %d\n",mTextH);*/
}

int DIntermissionScreenText::Responder (event_t *ev)
{
	if (ev->type == EV_KeyDown)
	{
		if(!mNoSkip)
		{
			if (mTicker < mTextDelay + (mTextLen * mTextSpeed))
			{
				mskip = true;
				mCount = mNumRows-1;
				mTicker = mTextDelay + (mTextLen * mTextSpeed);
				return 1;
			}
		}
		else if(mNoSkip)return 0;
	}
	return Super::Responder(ev);
}

float AlphaShift(float shift, float steps) // Funcion Modificada de ColorShiftPalette
{
		return ((float)((1.0 *(float)shift)/(float)steps));
}

//[GEC] Nuevo sistema para ScreenText con posibilidad de centrar el texto al estilo de PSXDoom & DOOM64
void DIntermissionScreenText::Drawer ()
{
	Super::Drawer();
	if (mTicker >= mTextDelay)
	{
		//Printf("mskip %d\n",mskip);

		FFont * Font = SmallFont;
		//Set New Font
		const char *fontname = mFontName.GetChars();
		Font = V_GetFont (fontname);
		if (Font == NULL)
			Font = SmallFont;

		FTexture *pic;
		int w;
		size_t count;
		int c;
		int index;
		const FRemapTable *range;
		const char *ch = mText;
		const int kerning = Font->GetDefaultKerning();

		const char *buffer = (const char *) malloc (mTextLen + 1);
		memset((char *)buffer, 0, mTextLen + 1);

		int alpha_index = 0;
		int scroll = 0;
		int scrollx = 0, scrolly = 0;
		double rx ,ry, rw, rh;

		// Count number of rows in this text. Since it does not word-wrap, we just count
		// line feed characters.
		int numrows = 1;
		int charcount = 0;
		int charcount2 = 0;

		for (c = 0; c <= mTextLen; c++)
		{
			if(mPlainText)
			{
				if((ch[c] == '\0')){mTextCount[numrows-1] = charcount+1;}
				if((ch[c] == '\n')){mTextCount[numrows-1] = charcount+1;}
				charcount++;
			}

			if(mFadeText)
			{
				if((ch[c] == '\0')){mTextCount2[numrows-1] = charcount2; charcount2 = 0;}
				if((ch[c] == '\n')){mTextCount2[numrows-1] = charcount2; charcount2 = 0;}
				else charcount2++;
			}

			numrows += (ch[c] == '\n');
		}

		int rowheight = Font->GetHeight() * CleanYfac;
		int rowpadding = (gameinfo.gametype & (GAME_DoomStrifeChex) ? 3 : -1) * CleanYfac;

		int cx = (mTextX - 160)*CleanXfac + screen->GetWidth() / 2;
		int cy = (mTextY - 100)*CleanYfac + screen->GetHeight() / 2;
		int startx = cx;

		if(mTextResW != 0 && mTextResH != 0)
		{
			rowheight = Font->GetHeight();
			if(mRowPadding != FRACUNIT)
				rowpadding = mRowPadding;
		}
		else
		{
			if(mRowPadding != FRACUNIT)
				rowpadding = mRowPadding * CleanYfac;
		}

		if(mTextResW != 0 && mTextResH != 0)//[GEC]
		{
			cx = (mTextX);
			cy = (mTextY);
			startx = cx;
		}

		if(mTextResW == 0 && mTextResH == 0)//[GEC]
		{
			// Does this text fall off the end of the screen? If so, try to eliminate some margins first.
			while (rowpadding > 0 && cy + numrows * (rowheight + rowpadding) - rowpadding > screen->GetHeight())
			{
				rowpadding--;
			}
			// If it's still off the bottom, try to center it vertically.
			if (cy + numrows * (rowheight + rowpadding) - rowpadding > screen->GetHeight())
			{
				cy = (screen->GetHeight() - (numrows * (rowheight + rowpadding) - rowpadding)) / 2;
				// If it's off the top now, you're screwed. It's too tall to fit.
				if (cy < 0)
				{
					cy = 0;
				}
			}
		}
		rowheight += rowpadding;

		// draw some of the text onto the screen
		count = (mTicker - mTextDelay) / mTextSpeed;
		range = Font->GetColorTranslation (mTextColor);

		if(!mskip)
		{
			if(!mPlainText)
			{
				if(mFadeText)//[GEC] mFade
				{
					mAlpha = (float)((float)255/(float)mTextCount2[mCount])/(float)mTextSpeed;
					if(FLOAT2FIXED(mAlpha) == 0)
					{
						mAlpha = 255.0;
						mCount++;
					}
					mAlpharows[mCount] = mAlphaTicker;

					if((ch[count] == '\n') && !mButtonPressed)
					{
						mButtonPressed = true;
						mAlpharows[mCount] = FLOAT2FIXED(0xFF);
						mAlphaTicker = 0; // [GEC] Reset mAlphaTicker
						mCount++;
					}

					if(!(ch[count] == '\n') && mButtonPressed)
					{mButtonPressed = false;}

					mCount = clamp<int>(mCount, 0, numrows);
					if((mCount == (numrows)) && !mButtonPressed) // [GEC] Stop mAlphaTicker
					{
						mAlphaTicker = 0; // [GEC] Reset mAlphaTicker
						Run_mAlphaTicker = false;
						mskip = true;
					}
				}
				else
				{
					if((int)count > mTextLen) // [GEC] Stop mAlphaTicker
					{
						mAlphaTicker = 0; // [GEC] Reset mAlphaTicker
						Run_mAlphaTicker = false;
						mskip = true;
					}
				}
			}
			else if(mPlainText && !mPlainSync)
			{
				if(mFadeText)//[GEC] mFade
				{
					mAlpha = (float)((float)255/(float)mTextCount2[mCount])/(float)mTextSpeed;
					mAlpharows[mCount] = mAlphaTicker;
				}
				else
				{
					mAlpharows[mCount] = FLOAT2FIXED(0xFF);
				}

				//Printf("mCount %d numrows %d count %d \nmAlpha %f\n",mCount, numrows, count, mAlpha);

				if((count == mTextCount[mCount]) && !mButtonPressed)
				{
					mButtonPressed = true;
					mAlpharows[mCount] = FLOAT2FIXED(0xFF);
					mAlpharows[mCount+1] = FLOAT2FIXED(0);
					mAlphaTicker = 0; // [GEC] Reset mAlphaTicker
					mCount++;
				}

				if(!(count == mTextCount[mCount]) && mButtonPressed)
				{mButtonPressed = false;}

				mCount = clamp<int>(mCount, 0, numrows);
				if((mCount == (numrows)) && !mButtonPressed) // [GEC] Stop mAlphaTicker
				{
					mAlphaTicker = 0; // [GEC] Reset mAlphaTicker
					Run_mAlphaTicker = false;
					mCount++;
					mskip = true;
				}
			}
			else if(mPlainText && mPlainSync)
			{
				//[GEC] En Doom64 Ex es un valor de 6 Aquí es 12 Para ser ajustable con la velocidad
				mAlpha = AlphaShift((float)12,(float)mTextSpeed); 

				if(mFadeText)//[GEC] mFade
					mAlpharows[mCount] = mAlphaTicker;
				else
					mAlpharows[mCount] = FLOAT2FIXED(0xFF);

				/*if(mCount == numrows-1)
				{
					mAlphaTicker = 0; // [GEC] Reset mAlphaTicker
					Run_mAlphaTicker = false;
					mskip = true;
				}*/

				if((mAlphaTicker == FLOAT2FIXED(0xFF)) && !mButtonPressed)
				{
					mButtonPressed = (mAlphaTicker == FLOAT2FIXED(0xFF));
					mAlpharows[mCount] = FLOAT2FIXED(0xFF);
					mAlpharows[mCount+1] = FLOAT2FIXED(0);
					mAlphaTicker = 0; // [GEC] Reset mAlphaTicker
					mCount++;
				}

				if(!(mAlphaTicker == FLOAT2FIXED(0xFF)) && mButtonPressed)
				{mButtonPressed = false;}

				if((mCount == numrows) && !mButtonPressed) // [GEC] Stop mAlphaTicker
				{
					mAlphaTicker = 0; // [GEC] Reset mAlphaTicker
					Run_mAlphaTicker = false;
					mskip = true;
					mCount--;
					mTicker = mTextDelay + (mTextLen * mTextSpeed);
				}
				else
				{
					Run_mAlphaTicker = true;
				}
			}
		}
		else
		{
			for (index = 0; index < numrows; index++)
			mAlpharows[index] = FLOAT2FIXED(0xFF);

			mWaitScroll = false;
			//mCount = numrows-2;
			mAlphaTicker = 0; // [GEC] Reset mAlphaTicker
			Run_mAlphaTicker = false;
		}
		//Printf("mCount %d mTextCount[mCount]%d\n",mCount,mTextCount[mCount]);

		if(mScrollText && !mWaitScroll)
		{
			Run_mScrollTicker = true;
			if(mTextResW != 0 && mTextResH != 0)
			{
				scroll +=(mScrollTicker / mScrollTextSpeed);
			}
			else
			{
				scroll +=(mScrollTicker / mScrollTextSpeed)* CleanYfac;
			}
		}

		for(index = 0; index < (int)(mPlainText ? mTextCount[mCount] : count); index++)
		{
			if (index >= mTextLen)
			{
				if(mskip)
				{
					if (mScrollTicker >= mScrollTextTime)
					{
						mNoSkip = false;
						Run_mScrollTicker = false;	//[GEC]
						mAlphaTicker = 0; // [GEC] Reset mAlphaTicker
						Run_mAlphaTicker = false;
						//auto skip
						if(mAutoSkip)
						{
							F_AdvanceIntermission();
						}
						
					}
					else if(mScrollTextTime == 0)
					{
						//F_AdvanceIntermission();
						mNoSkip = false;
						Run_mScrollTicker = false;	//[GEC]
						mAlphaTicker = 0; // [GEC] Reset mAlphaTicker
						Run_mAlphaTicker = false;
					}
				}

				//mskip = true;
				memcpy((char *)buffer, ch, mTextLen);
				break;
			}

			memcpy((char *)buffer, ch, index);
		}

		if(mCenterText)
		{
			if(mTextResW != 0 && mTextResH != 0)
			{
				cx = (mTextResW - Font->StringWidth((const BYTE *)buffer,true))/2;//center
			}
			else
			{
				cx = ((320 - Font->StringWidth((const BYTE *)buffer,true))/2) * CleanXfac;//center
			}
		}

		//for ( ; count > 0 ; count-- )
		while((c = *buffer++))
		{
			/*c = *ch++;
			if (!c)
				break;*/
			if (c == '\n')
			{
				cx = startx;
				cy += rowheight;

				
				if(mCenterText)
				{
					if(mTextResW != 0 && mTextResH != 0)
					{
						cx = (mTextResW - Font->StringWidth((const BYTE *)buffer,true))/2;//center
					}
					else
					{
						cx = ((320 - Font->StringWidth((const BYTE *)buffer,true))/2) * CleanXfac;//center
					}
				}

				if(mFadeText) alpha_index++;
				continue;
			}

			pic = Font->GetChar (c, &w);
			w += kerning;
			if(mTextResW == 0 && mTextResH == 0){w *= CleanXfac;}

			if ((cx + w > SCREENWIDTH) && !mCenterText)
				continue;

			if (pic != NULL)
			{
				if(mScrollText)
				{
					switch (mScrollTextDirection)
					{
					case SCROLL_Up:
						scrolly = -scroll;
						break;

					case SCROLL_Down:
						scrolly = +scroll;
						break;

					case SCROLL_Left:
						scrollx = -scroll;
						break;

					case SCROLL_Right:
						scrollx = +scroll;
						break;
					}
				}

				if(mTextResW != 0 && mTextResH != 0)
				{
					rx = (cx + scrollx);
					ry = (cy + scrolly);
					rw = pic->GetScaledWidthDouble();
					rh = pic->GetScaledHeightDouble();

					screen->VirtualToRealCoords(rx, ry, rw, rh, mTextResW, mTextResH, true);

					screen->DrawTexture (pic,
					rx,
					ry,
					DTA_DestWidthF, rw,
					DTA_DestHeightF, rh, 
					DTA_Translation, range,
					DTA_Alpha, FLOAT2FIXED(FIXED2FLOAT(mAlpharows[alpha_index])/255.f),
					DTA_RenderStyle, LegacyRenderStyles[mTextAdd ? STYLE_Add : STYLE_Translucent],
					TAG_DONE);
				}
				else
				{
					screen->DrawTexture (pic,
					cx + scrollx,
					cy + scrolly,
					DTA_Translation, range,
					DTA_CleanNoMove, true,
					DTA_Alpha, FLOAT2FIXED(FIXED2FLOAT(mAlpharows[alpha_index])/255.f),
					DTA_RenderStyle, LegacyRenderStyles[mTextAdd ? STYLE_Add : STYLE_Translucent],
					TAG_DONE);
				}
			}
			cx += w;
		}
	}
}
// [GEC] El Viejo Codigo
/*
void DIntermissionScreenText::Drawer ()
{
	Super::Drawer();
	if (mTicker >= mTextDelay)
	{
		FTexture *pic;
		int w;
		size_t count;
		int c;
		const FRemapTable *range;
		const char *ch = mText;
		const int kerning = SmallFont->GetDefaultKerning();

		// Count number of rows in this text. Since it does not word-wrap, we just count
		// line feed characters.
		int numrows;

		for (numrows = 1, c = 0; ch[c] != '\0'; ++c)
		{
			numrows += (ch[c] == '\n');
		}

		int rowheight = SmallFont->GetHeight() * CleanYfac;
		int rowpadding = (gameinfo.gametype & (GAME_DoomStrifeChex) ? 3 : -1) * CleanYfac;

		int cx = (mTextX - 160)*CleanXfac + screen->GetWidth() / 2;
		int cy = (mTextY - 100)*CleanYfac + screen->GetHeight() / 2;
		int startx = cx;

		// Does this text fall off the end of the screen? If so, try to eliminate some margins first.
		while (rowpadding > 0 && cy + numrows * (rowheight + rowpadding) - rowpadding > screen->GetHeight())
		{
			rowpadding--;
		}
		// If it's still off the bottom, try to center it vertically.
		if (cy + numrows * (rowheight + rowpadding) - rowpadding > screen->GetHeight())
		{
			cy = (screen->GetHeight() - (numrows * (rowheight + rowpadding) - rowpadding)) / 2;
			// If it's off the top now, you're screwed. It's too tall to fit.
			if (cy < 0)
			{
				cy = 0;
			}
		}
		rowheight += rowpadding;

		// draw some of the text onto the screen
		count = (mTicker - mTextDelay) / mTextSpeed;
		range = SmallFont->GetColorTranslation (mTextColor);

		for ( ; count > 0 ; count-- )
		{
			c = *ch++;
			if (!c)
				break;
			if (c == '\n')
			{
				cx = startx;
				cy += rowheight;
				continue;
			}

			pic = SmallFont->GetChar (c, &w);
			w += kerning;
			w *= CleanXfac;
			if (cx + w > SCREENWIDTH)
				continue;
			if (pic != NULL)
			{
				screen->DrawTexture (pic,
					cx,
					cy,
					DTA_Translation, range,
					DTA_CleanNoMove, true,
					TAG_DONE);
			}
			cx += w;
		}
	}
}*/

//==========================================================================
//
//
//
//==========================================================================

void DIntermissionScreenCast::Init(FIntermissionAction *desc, bool first)
{
	Super::Init(desc, first);
	mName = static_cast<FIntermissionActionCast*>(desc)->mName;

	mFontName = static_cast<FIntermissionActionCast*>(desc)->mFontName;//[GEC]
	mYpos = static_cast<FIntermissionActionCast*>(desc)->mYpos;//[GEC]
	mResW = static_cast<FIntermissionActionCast*>(desc)->mResW;//[GEC]
	mResH = static_cast<FIntermissionActionCast*>(desc)->mResH;//[GEC]
	mText = static_cast<FIntermissionActionCast*>(desc)->mText;//[GEC]
	mYpos2 = static_cast<FIntermissionActionCast*>(desc)->mYpos2;//[GEC]
	mXpos2 = static_cast<FIntermissionActionCast*>(desc)->mXpos2;//[GEC]
	mShotSound = static_cast<FIntermissionActionCast*>(desc)->mShotSound;//[GEC]
	mTextColor = static_cast<FIntermissionActionCast*>(desc)->mTextColor;//[GEC]

	mcastrotationenable = static_cast<FIntermissionActionCast*>(desc)->mcastrotation;//[GEC]
	mlightplus = static_cast<FIntermissionActionCast*>(desc)->mlightplus;//[GEC]
	mlightmax = static_cast<FIntermissionActionCast*>(desc)->mlightmax;//[GEC]

	mYposSprite = static_cast<FIntermissionActionCast*>(desc)->mYposSprite;//[GEC]
	mCenterXSprite = static_cast<FIntermissionActionCast*>(desc)->mCenterXSprite;//[GEC]

	mcastrotation = 0;//[GEC]
	mcastlight = (mlightmax != 0 && mlightplus != 0) ? 0 : 255;//[GEC]

	mClass = PClass::FindClass(static_cast<FIntermissionActionCast*>(desc)->mCastClass);
	if (mClass != NULL) mDefaults = GetDefaultByType(mClass);
	else
	{
		mDefaults = NULL;
		caststate = NULL;
		return;
	}

	mCastSounds.Resize(static_cast<FIntermissionActionCast*>(desc)->mCastSounds.Size());
	for (unsigned i=0; i < mCastSounds.Size(); i++)
	{
		mCastSounds[i].mSequence = static_cast<FIntermissionActionCast*>(desc)->mCastSounds[i].mSequence;
		mCastSounds[i].mIndex = static_cast<FIntermissionActionCast*>(desc)->mCastSounds[i].mIndex;
		mCastSounds[i].mSound = static_cast<FIntermissionActionCast*>(desc)->mCastSounds[i].mSound;
	}
	caststate = mDefaults->SeeState;
	if (mClass->IsDescendantOf(RUNTIME_CLASS(APlayerPawn)))
	{
		advplayerstate = mDefaults->MissileState;
		casttranslation = translationtables[TRANSLATION_Players][consoleplayer];
	}
	else
	{
		advplayerstate = NULL;
		casttranslation = NULL;
		if (mDefaults->Translation != 0)
		{
			casttranslation = translationtables[GetTranslationType(mDefaults->Translation)]
												[GetTranslationIndex(mDefaults->Translation)];
		}
	}
	castdeath = false;
	castframes = 0;
	castonmelee = 0;
	castattacking = false;
	if (mDefaults->SeeSound)
	{
		S_Sound (CHAN_VOICE | CHAN_UI, mDefaults->SeeSound, 1, ATTN_NONE);
	}
}

int DIntermissionScreenCast::Responder (event_t *ev)
{
	if(ev->type == EV_KeyDown && mcastrotationenable)
	{
		int ch = ev->data1;
		switch (ch)
		{
		case KEY_PAD_DPAD_LEFT:
		case KEY_PAD_LTHUMB_LEFT:
		case KEY_JOYAXIS2MINUS:
		case KEY_JOYPOV1_LEFT:
		case KEY_LEFTARROW:
			mcastrotation = mcastrotation+2 & 15;
			return 0;
			break;

		case KEY_PAD_DPAD_RIGHT:
		case KEY_PAD_LTHUMB_RIGHT:
		case KEY_JOYAXIS2PLUS:
		case KEY_JOYPOV1_RIGHT:
		case KEY_RIGHTARROW:
			mcastrotation = mcastrotation-2 & 15;
			return 0;
			break;
		}
		//Printf("castrotation %d\n",castrotation);
	}

	if (ev->type != EV_KeyDown) return 0;

	if (castdeath)
		return 1;					// already in dying frames

	castdeath = true;

	if (mClass != NULL)
	{
		if(mShotSound)//[GEC]
		{
			S_Sound (CHAN_WEAPON | CHAN_UI, mShotSound, 1, ATTN_NONE);
		}

		FName label[] = {NAME_Death, NAME_Cast};
		caststate = mClass->ActorInfo->FindState(2, label);
		if (caststate == NULL) return -1;

		casttics = caststate->GetTics();
		castframes = 0;
		castattacking = false;

		if (mClass->IsDescendantOf(RUNTIME_CLASS(APlayerPawn)))
		{
			int snd = S_FindSkinnedSound(players[consoleplayer].mo, "*death");
			if (snd != 0) S_Sound (CHAN_VOICE | CHAN_UI, snd, 1, ATTN_NONE);
		}
		else if (mDefaults->DeathSound)
		{
			S_Sound (CHAN_VOICE | CHAN_UI, mDefaults->DeathSound, 1, ATTN_NONE);
		}
	}
	return true;
}

void DIntermissionScreenCast::PlayAttackSound()
{
	// sound hacks....
	if (caststate != NULL && castattacking)
	{
		for (unsigned i = 0; i < mCastSounds.Size(); i++)
		{
			if ((!!mCastSounds[i].mSequence) == (basestate != mDefaults->MissileState) &&
				(caststate == basestate + mCastSounds[i].mIndex))
			{
				S_StopAllChannels ();
				S_Sound (CHAN_WEAPON | CHAN_UI, mCastSounds[i].mSound, 1, ATTN_NONE);
				return;
			}
		}
	}

}

int DIntermissionScreenCast::Ticker ()
{
	Super::Ticker();

	if(mlightmax != 0 && mlightplus != 0)//[GEC]
	{
		mcastlight = mcastlight + mlightplus;

		if(mcastlight >= mlightmax)
		{
			mcastlight = mlightmax;
			mlightmax = 0;
			mlightplus = 0;
		}
	}

	if (--casttics > 0 && caststate != NULL)
		return 0; 				// not time to change state yet

	if (caststate == NULL || caststate->GetTics() == -1 || caststate->GetNextState() == NULL ||
		(caststate->GetNextState() == caststate && castdeath))
	{
		return -1;
	}
	else
	{
		// just advance to next state in animation
		if (caststate == advplayerstate)
			goto stopattack;	// Oh, gross hack!

		caststate = caststate->GetNextState();

		PlayAttackSound();
		castframes++;
	}

	if (castframes == 12 && !castdeath)
	{
		// go into attack frame
		castattacking = true;
		if (!mClass->IsDescendantOf(RUNTIME_CLASS(APlayerPawn)))
		{
			if (castonmelee)
				basestate = caststate = mDefaults->MeleeState;
			else
				basestate = caststate = mDefaults->MissileState;
			castonmelee ^= 1;
			if (caststate == NULL)
			{
				if (castonmelee)
					basestate = caststate = mDefaults->MeleeState;
				else
					basestate = caststate = mDefaults->MissileState;
			}
		}
		else
		{
			// The players use the melee state differently so it can't be used here
			basestate = caststate = mDefaults->MissileState;
		}
		PlayAttackSound();
	}

	if (castattacking)
	{
		if (castframes == 24 || caststate == mDefaults->SeeState )
		{
		  stopattack:
			castattacking = false;
			castframes = 0;
			caststate = mDefaults->SeeState;
		}
	}

	casttics = caststate->GetTics();
	if (casttics == -1)
		casttics = 15;
	return 0;
}

void DIntermissionScreenCast::Drawer ()
{
	spriteframe_t*		sprframe;
	FTexture*			pic;

	FFont * Font = SmallFont;//[GEC]
	//Set New Font
	const char *fontname = mFontName.GetChars();
	Font = V_GetFont (fontname);
	if (Font == NULL)
		Font = SmallFont;

	Super::Drawer();

	const char *name = mName;
	if (name != NULL)
	{
		if (*name == '$') name = GStrings(name+1);

		int X = (SCREENWIDTH - Font->StringWidth (name) * CleanXfac)/2;//[GEC]
		int Y = (SCREENHEIGHT * 180) / 200;//[GEC]

		if(mYpos != FRACUNIT)//[GEC]
		{
			if(mResW != 0 && mResH != 0 )
			{
				X = (mResW - Font->StringWidth (name))/2;
				Y = mYpos;
			}
			else
			{
				Y = mYpos * CleanYfac;
			}
		}

		if(mResW != 0 && mResH != 0 )//[GEC]
		{
			screen->DrawText (Font, mTextColor,
				X,Y, name, DTA_ResWidthF, mResW, DTA_ResHeightF, mResH, TAG_DONE);
		}
		else
		{
			screen->DrawText (Font, mTextColor,
				X,Y,name,DTA_CleanNoMove, true, TAG_DONE);
		}
	}

	name = mText;
	if (name != NULL)
	{
		if (*name == '$') name = GStrings(name+1);

		int X = mXpos2 * CleanXfac;//[GEC]
		int Y = mYpos2 * CleanYfac;//[GEC]

		if(mResW != 0 && mResH != 0 )
		{
			X = mXpos2;
			Y = mYpos2;
		}

		if(mResW != 0 && mResH != 0 )//[GEC]
		{
			screen->DrawText (Font, CR_UNTRANSLATED,
				X,Y, name, DTA_ResWidthF, mResW, DTA_ResHeightF, mResH, TAG_DONE);
		}
		else
		{
			screen->DrawText (Font, CR_UNTRANSLATED,
				X,Y,name,DTA_CleanNoMove, true, TAG_DONE);
		}
	}

	// draw the current frame in the middle of the screen
	if (caststate != NULL)
	{
		double castscalex = FIXED2DBL(mDefaults->scaleX);
		double castscaley = FIXED2DBL(mDefaults->scaleY);
		double rx, ry, rw, rh, offsetx;//[GEC]

		int castsprite = caststate->sprite;

		if (!(mDefaults->flags4 & MF4_NOSKIN) &&
			mDefaults->SpawnState != NULL && caststate->sprite == mDefaults->SpawnState->sprite &&
			mClass->IsDescendantOf(RUNTIME_CLASS(APlayerPawn)) &&
			skins != NULL)
		{
			// Only use the skin sprite if this class has not been removed from the
			// PlayerClasses list.
			for (unsigned i = 0; i < PlayerClasses.Size(); ++i)
			{
				if (PlayerClasses[i].Type == mClass)
				{
					FPlayerSkin *skin = &skins[players[consoleplayer].userinfo.GetSkin()];
					castsprite = skin->sprite;

					if (!(mDefaults->flags4 & MF4_NOSKIN))
					{
						castscaley = FIXED2DBL(skin->ScaleY);
						castscalex = FIXED2DBL(skin->ScaleX);
					}
				}
			}
		}

		sprframe = &SpriteFrames[sprites[castsprite].spriteframes + caststate->GetFrame()];
		pic = TexMan(sprframe->Texture[mcastrotation]);

		int Flip = (int)(sprframe->Flip & (1 << mcastrotation));//[GEC]
		int X = 160;//[GEC]
		int Y = 170;//[GEC]

		if(mYposSprite != FRACUNIT)//[GEC]
		{
			if(mResW != 0 && mResH != 0 )
			{
				X = (mResW)/2;
				Y = mYposSprite;
			}
			else
			{
				Y = mYpos;
			}
		}

		if(mResW != 0 && mResH != 0 )//[GEC]
		{
			rx = (double) X;
			ry = (double) Y;
			rw = (double) pic->GetScaledWidthDouble() * castscalex;
			rh = (double) pic->GetScaledHeightDouble() * castscaley;
			offsetx = (double) pic->GetScaledLeftOffsetDouble() * castscalex;
			if(mCenterXSprite) rx = (double) Flip ? (X + offsetx) - rw : X - offsetx;

			screen->VirtualToRealCoords(rx, ry, rw, rh, mResW, mResH, false);

			screen->DrawTexture (pic,
				rx,
				ry,
				DTA_DestWidthF, rw,
				DTA_DestHeightF, rh, 
				DTA_FlipX, Flip,
				DTA_LeftOffsetF, mCenterXSprite ? 0 : offsetx,
				DTA_RenderStyle, mDefaults->RenderStyle,
				DTA_Alpha, mDefaults->alpha,
				DTA_Translation, casttranslation,
				DTA_GLCOLOR,MAKERGB(mcastlight,mcastlight,mcastlight),
				TAG_DONE);
		}
		else
		{
			rx = (double) X;
			rw = (double) pic->GetScaledWidthDouble() * castscalex;
			offsetx = (double) pic->GetScaledLeftOffsetDouble() * castscalex;
			if(mCenterXSprite) rx = (double) Flip ? (X + offsetx) - rw : X - offsetx;

			screen->DrawTexture (pic, rx, Y,
				DTA_320x200, true,
				DTA_FlipX, Flip,
				DTA_LeftOffsetF, mCenterXSprite ? 0 : offsetx,//[GEC]
				DTA_DestHeightF, pic->GetScaledHeightDouble() * castscaley,
				DTA_DestWidthF, pic->GetScaledWidthDouble() * castscalex,
				DTA_RenderStyle, mDefaults->RenderStyle,
				DTA_Alpha, mDefaults->alpha,
				DTA_Translation, casttranslation,
				DTA_GLCOLOR,MAKERGB(mcastlight,mcastlight,mcastlight),
				TAG_DONE);
		}
	}
}

//==========================================================================
//
//
//
//==========================================================================

void DIntermissionScreenScroller::Init(FIntermissionAction *desc, bool first)
{
	Super::Init(desc, first);
	mFirstPic = mBackground;
	mSecondPic = TexMan.CheckForTexture(static_cast<FIntermissionActionScroller*>(desc)->mSecondPic, FTexture::TEX_MiscPatch);
	mScrollDelay = static_cast<FIntermissionActionScroller*>(desc)->mScrollDelay;
	mScrollTime = static_cast<FIntermissionActionScroller*>(desc)->mScrollTime;
	mScrollDir = static_cast<FIntermissionActionScroller*>(desc)->mScrollDir;
}

int DIntermissionScreenScroller::Responder (event_t *ev)
{
	int res = Super::Responder(ev);
	if (res == -1)
	{
		mBackground = mSecondPic;
		mTicker = mScrollDelay + mScrollTime;
	}
	return res;
}

void DIntermissionScreenScroller::Drawer ()
{
	FTexture *tex = TexMan[mFirstPic];
	FTexture *tex2 = TexMan[mSecondPic];
	if (mTicker >= mScrollDelay && mTicker < mScrollDelay + mScrollTime && tex != NULL && tex2 != NULL)
	{

		int fwidth = tex->GetScaledWidth();
		int fheight = tex->GetScaledHeight();

		double xpos1 = 0, ypos1 = 0, xpos2 = 0, ypos2 = 0;

		switch (mScrollDir)
		{
		case SCROLL_Up:
			ypos1 = double(mTicker - mScrollDelay) * fheight / mScrollTime;
			ypos2 = ypos1 - fheight;
			break;

		case SCROLL_Down:
			ypos1 = -double(mTicker - mScrollDelay) * fheight / mScrollTime;
			ypos2 = ypos1 + fheight;
			break;

		case SCROLL_Left:
		default:
			xpos1 = double(mTicker - mScrollDelay) * fwidth / mScrollTime;
			xpos2 = xpos1 - fwidth;
			break;

		case SCROLL_Right:
			xpos1 = -double(mTicker - mScrollDelay) * fwidth / mScrollTime;
			xpos2 = xpos1 + fwidth;
			break;
		}

		screen->DrawTexture (tex, xpos1, ypos1,
			DTA_VirtualWidth, fwidth,
			DTA_VirtualHeight, fheight,
			DTA_Masked, false,
			TAG_DONE);
		screen->DrawTexture (tex2, xpos2, ypos2,
			DTA_VirtualWidth, fwidth,
			DTA_VirtualHeight, fheight,
			DTA_Masked, false,
			TAG_DONE);

		screen->FillBorder (NULL);
		mBackground = mSecondPic;
	}
	else 
	{
		Super::Drawer();
	}
}


//==========================================================================
//
//
//
//==========================================================================

DIntermissionController *DIntermissionController::CurrentIntermission;

DIntermissionController::DIntermissionController(FIntermissionDescriptor *Desc, bool DeleteDesc, BYTE state)
{
	mDesc = Desc;
	mDeleteDesc = DeleteDesc;
	mIndex = 0;
	mAdvance = false;
	mSentAdvance = false;
	mScreen = NULL;
	mFirst = true;
	mGameState = state;

	// If the intermission finishes straight away then cancel the wipe.
	if(!NextPage())
		wipegamestate = GS_FINALE;
}

bool DIntermissionController::NextPage ()
{
	mPicResW = 0;//[GEC]
	mPicResH = 0;//[GEC]
	mPicX = 0;//[GEC]
	mPicY = 0;//[GEC]
	mWipeOut = GS_FINALE;//[GEC]

	FTextureID bg;
	bool fill = false;

	if (mIndex == (int)mDesc->mActions.Size() && mDesc->mLink == NAME_None)
	{
		// last page
		return false;
	}

	if (mScreen != NULL)
	{
		bg = mScreen->GetBackground(&fill);
		mScreen->Destroy();
	}
again:
	while ((unsigned)mIndex < mDesc->mActions.Size())
	{
		FIntermissionAction *action = mDesc->mActions[mIndex++];

		if (action->mClass == WIPER_ID)
		{
			wipegamestate = static_cast<FIntermissionActionWiper*>(action)->mWipeType;
		}
		else if (action->mClass == TITLE_ID)
		{
			Destroy();
			D_StartTitle (true);//[GEC]
			return false;
		}
		else
		{
			// create page here
			mScreen = (DIntermissionScreen*)action->mClass->CreateNew();
			mScreen->SetBackground(bg, fill);	// copy last screen's background before initializing
			mScreen->Init(action, mFirst);
			mFirst = false;
			return true;
		}
	}
	if (mDesc->mLink != NAME_None)
	{
		FIntermissionDescriptor **pDesc = IntermissionDescriptors.CheckKey(mDesc->mLink);
		if (pDesc != NULL)
		{
			if (mDeleteDesc) delete mDesc;
			mDeleteDesc = false;
			mIndex = 0;
			mDesc = *pDesc;
			goto again;
		}
	}
	return false;
}

bool DIntermissionController::Responder (event_t *ev)
{
	if (mScreen != NULL)
	{
		if (!mScreen->mPaletteChanged && ev->type == EV_KeyDown)
		{
			const char *cmd = Bindings.GetBind (ev->data1);

			if (cmd != NULL &&
				(!stricmp(cmd, "toggleconsole") ||
				 !stricmp(cmd, "screenshot")))
			{
				return false;
			}
		}

		if (mScreen->mTicker < 2) return false;	// prevent some leftover events from auto-advancing
		int res = mScreen->Responder(ev);
		if (res == -1 && !mSentAdvance)
		{
			Net_WriteByte(DEM_ADVANCEINTER);
			mSentAdvance = true;
		}
		return !!res;
	}
	return false;
}

void DIntermissionController::Ticker ()
{
	if (mAdvance)
	{
		mSentAdvance = false;
	}
	if (mScreen != NULL)
	{
		mAdvance |= (mScreen->Ticker() == -1);
	}
	if (mAdvance)
	{
		mAdvance = false;
		if (!NextPage())
		{
			switch (mGameState)
			{
			case FSTATE_InLevel:
				if (level.cdtrack == 0 || !S_ChangeCDMusic (level.cdtrack, level.cdid))
					S_ChangeMusic (level.Music, level.musicorder);
				gamestate = GS_LEVEL;
				wipegamestate = GS_LEVEL;
				P_ResumeConversation ();
				viewactive = true;
				Destroy();
				break;

			case FSTATE_ChangingLevel:
				gameaction = ga_worlddone;
				Destroy();
				break;

			default:
				break;
			}
		}
	}
}

void DIntermissionController::Drawer ()
{
	if (mScreen != NULL)
	{
		mScreen->Drawer();
	}
}

void DIntermissionController::Destroy ()
{
	mPicResW = 0;//[GEC]
	mPicResH = 0;//[GEC]
	mPicX = 0;//[GEC]
	mPicY = 0;//[GEC]
	mWipeOut = GS_FINALE;//[GEC]

	Super::Destroy();
	if (mScreen != NULL) mScreen->Destroy();
	if (mDeleteDesc) delete mDesc;
	mDesc = NULL;
	if (CurrentIntermission == this) CurrentIntermission = NULL;
}


//==========================================================================
//
// starts a new intermission
//
//==========================================================================

void F_StartIntermission(FIntermissionDescriptor *desc, bool deleteme, BYTE state)
{
	if (DIntermissionController::CurrentIntermission != NULL)
	{
		DIntermissionController::CurrentIntermission->Destroy();
	}

	mPicResW = 0;//[GEC]
	mPicResH = 0;//[GEC]
	mPicX = 0;//[GEC]
	mPicY = 0;//[GEC]

	V_SetBlend (0,0,0,0);
	S_StopAllChannels ();
	gameaction = ga_nothing;
	gamestate = GS_FINALE;
	if (state == FSTATE_InLevel) wipegamestate = GS_FINALE;	// don't wipe when within a level.
	viewactive = false;
	automapactive = false;
	DIntermissionController::CurrentIntermission = new DIntermissionController(desc, deleteme, state);
	GC::WriteBarrier(DIntermissionController::CurrentIntermission);
}


//==========================================================================
//
// starts a new intermission
//
//==========================================================================

void F_StartIntermission(FName seq, BYTE state)
{
	FIntermissionDescriptor **pdesc = IntermissionDescriptors.CheckKey(seq);
	if (pdesc != NULL)
	{
		F_StartIntermission(*pdesc, false, state);
	}
}


//==========================================================================
//
// Called by main loop.
//
//==========================================================================

bool F_Responder (event_t* ev)
{
	if (DIntermissionController::CurrentIntermission != NULL)
	{
		return DIntermissionController::CurrentIntermission->Responder(ev);
	}
	return false;
}

//==========================================================================
//
// Called by main loop.
//
//==========================================================================

void F_Ticker ()
{
	if (DIntermissionController::CurrentIntermission != NULL)
	{
		DIntermissionController::CurrentIntermission->Ticker();
	}
}

//==========================================================================
//
// Called by main loop.
//
//==========================================================================

void F_Drawer ()
{
	if (DIntermissionController::CurrentIntermission != NULL)
	{
		DIntermissionController::CurrentIntermission->Drawer();
	}
}


//==========================================================================
//
// Called by main loop.
//
//==========================================================================

void F_EndFinale ()
{
	if (DIntermissionController::CurrentIntermission != NULL)
	{
		DIntermissionController::CurrentIntermission->Destroy();
		DIntermissionController::CurrentIntermission = NULL;
	}
}

//==========================================================================
//
// Called by net loop.
//
//==========================================================================

void F_AdvanceIntermission()
{
	if (DIntermissionController::CurrentIntermission != NULL)
	{
		DIntermissionController::CurrentIntermission->mAdvance = true;
	}
}

