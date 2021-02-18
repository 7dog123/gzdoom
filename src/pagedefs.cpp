#include "doomerrors.h"

#include "d_gui.h"
#include "m_random.h"
#include "doomdef.h"
#include "doomstat.h"
#include "gstrings.h"
#include "w_wad.h"
#include "s_sound.h"
#include "v_video.h"
#include "intermission/intermission.h"
#include "f_wipe.h"
#include "m_argv.h"
#include "m_misc.h"
#include "menu/menu.h"
#include "c_console.h"
#include "c_dispatch.h"
#include "i_system.h"
#include "i_sound.h"
#include "i_video.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "wi_stuff.h"
#include "st_stuff.h"
#include "am_map.h"
#include "p_setup.h"
#include "r_utility.h"
#include "r_sky.h"
#include "d_main.h"
#include "d_dehacked.h"
#include "cmdlib.h"
#include "s_sound.h"
#include "m_swap.h"
#include "v_text.h"
#include "gi.h"
#include "b_bot.h"		//Added by MC:
#include "stats.h"
#include "gameconfigfile.h"
#include "sbar.h"
#include "decallib.h"
#include "version.h"
#include "v_text.h"
#include "st_start.h"
#include "templates.h"
#include "teaminfo.h"
#include "hardware.h"
#include "sbarinfo.h"
#include "d_net.h"
#include "g_level.h"
#include "d_event.h"
#include "d_netinf.h"
#include "v_palette.h"
#include "m_cheat.h"
#include "compatibility.h"
#include "m_joy.h"
#include "sc_man.h"
#include "po_man.h"
#include "resourcefiles/resourcefile.h"
#include "r_renderer.h"
#include "p_local.h"

#include "pagedefs.h"

//==========================================================================
//
// [GEC]
// New Lump PAGEDEFS
//
//==========================================================================

extern bool MenuReset; //[GEC]
extern bool DisableDim; //[GEC]

bool DrawCustomPage = false; //[GEC]
bool Force_Wipe = false; //[GEC]
bool Force_Clear = false; //[GEC]
int SkipPage = -1; //[GEC]
bool ForceSkipPage = false; //[GEC]
bool NoMenu = false; //[GEC]
bool DrawMenu = false; //[GEC]
bool ResetCount = false; //[GEC]
int RestarPage = -1; //[GEC]

extern int demosequence;
extern int pagetic;
extern bool advancedemo;

TArray<FPageDef *> mPages;
FPageDef *SetPage;

//==========================================================================
//
// [GEC]
// AddPage
//
// Adds a new pages to the array. If one with the same page_num as the
// new one already exists, it is replaced.
//
//==========================================================================

FPageDef *AddPage (FPageDef *mPage)
{
	// Search for existing duplicate.
	for (unsigned int i = 0; i < mPages.Size(); ++i)
	{
		if (mPages[i]->PageNum == mPage->PageNum)
		{
			// Found one!
			free (mPages[i]);
			mPages[i] = mPage;
			return mPage;
		}
	}
	// Didn't find one, so add it at the end.
	mPages.Push (mPage);
	return mPage;
}

//==========================================================================
//
// ParsePageDefs
//
//==========================================================================

static const char *CoreKeywords[]=
{
   "Page",
   NULL
};

void ParsePageDefs()
{
	int lump, lastlump = 0;

	GameStartupInfo.Skill = -1;

	struct WipeType
	{
		const char *Name;
		gamestate_t Type;
	}
	const FT[] = {
		{ "Crossfade", GS_FORCEWIPEFADE },
		{ "Melt", GS_FORCEWIPEMELT },
		{ "Burn", GS_FORCEWIPEBURN },
		{ "Default", GS_FORCEWIPE },
		{ "Melt64", GS_FORCEWIPEMELT64 },//[GEC]
		{ "FadeScreen", GS_FORCEWIPEFADESCREEN  },//[GEC]
		{ "LoadingScreen", GS_FORCEWIPELOADINGSCREEN  },//[GEC]
		{ "NullWipe", GS_FORCEWIPENONE  },//[GEC]
		{ NULL, GS_DEMOSCREEN }//GS_FORCEWIPE }
	};

	while ((lump = Wads.FindLump("PAGEDEFS", &lastlump)) != -1)
	{
		DrawCustomPage = true;//[GEC]

		FScanner sc(lump);
		sc.SetCMode(true);
			while (sc.GetString())
			{
				if (sc.Compare("page"))
				{
					//---------------------------------------------------------
					FPageDef *Pag = new FPageDef;
					Pag->SetDefaultPage();
					//---------------------------------------------------------

					sc.MustGetNumber();
					Pag->PageNum = sc.Number;
					Pag->NextPage = sc.Number;

					//Printf("Page %d\n",page->pagenum);

					sc.MustGetStringName("{");
					while (!sc.CheckString("}"))
					{
						if (sc.CheckString("pagetime"))
						{
							sc.MustGetToken('=');//pagetime
							sc.MustGetNumber();
							Pag->PageTime = sc.Number;
						}

						else if (sc.CheckString("nextpage"))
						{
							sc.MustGetToken('=');//nextpage
							sc.MustGetNumber();
							Pag->NextPage = sc.Number;
						}
						else if (sc.CheckString("skippage"))
						{
							sc.MustGetToken('=');//skippage
							sc.MustGetNumber();
							Pag->SkipPage = sc.Number;
						}
						else if (sc.CheckString("nomenu"))
						{
							Pag->NoMenu = true;//nomenu
						}
						else if (sc.CheckString("drawmenu"))
						{
							Pag->DrawMenu = true;//drawmenu
						}
						else if (sc.CheckString("disabledim"))
						{
							Pag->DisableDim = true;//disabledim
						}
						else if (sc.CheckString("clearmenu"))
						{
							Pag->ClearMenu = true;//clearmenu
						}
						else if (sc.CheckString("resetcount"))
						{
							Pag->ResetCount = true;//resetcount
						}
						else if (sc.CheckString("noclear"))
						{
							Pag->NoClear = true;//noclear
						}
						else if (sc.CheckString("forcewipe"))
						{
							sc.MustGetToken('=');//forcewipe
							sc.MustGetToken(TK_Identifier);
							int v = sc.MatchString(&FT[0].Name, sizeof(FT[0]));
							if (v != -1) Pag->ForceWipe = FT[v].Type;
						}
						else if (sc.CheckString("setrestarpage"))
						{
							RestarPage = Pag->PageNum;//setrestarpage
						}
						else if (sc.CheckString("resetanimations"))
						{
							Pag->ResetAnimation = true;//resetanimations
						}

						else if (sc.CheckString("rundemo"))
						{
							sc.MustGetToken('=');//demoname
							sc.MustGetString();
							Pag->DemoName = sc.String;
						}

						else if (sc.CheckString("runtitlemap"))
						{
							sc.MustGetToken('=');//mapname
							sc.MustGetString();
							Pag->TitleMap = sc.String;
						}

						else if (sc.CheckString("playsound"))
						{
							sc.MustGetToken('=');//soundname
							sc.MustGetString();
							Pag->SoundName = sc.String;
						}

						else if (sc.CheckString("playmusic"))
						{
							sc.MustGetToken('=');//musicname
							sc.MustGetString();
							Pag->MusicName = sc.String;
						}

						else if (sc.CheckString("fireout"))
						{
							Pag->FireOut = true;//fireout
						}

						else if (sc.CheckString("fireinit"))
						{
							Pag->FireInit = true;//fireinit
						}

						else if (sc.CheckString("runfire"))
						{
							Pag->RunFire = true;//runfire
						}

						else if (sc.CheckString("layerpic"))
						{
							if(Pag->LayerCount == MAXPAGELAYER)//layerpic
							{
								sc.ScriptError("Maximun Layers [%d]", MAXPAGELAYER);
							}

							sc.MustGetStringName("{");
							while (!sc.CheckString("}"))
							{
								if (sc.CheckString("pic"))
								{
									sc.MustGetToken('=');//name
									sc.MustGetString();
									Pag->Layers[Pag->LayerCount].BasePic = sc.String;
								}
								if (sc.CheckString("text"))
								{
									sc.MustGetToken('=');//name
									sc.MustGetString();
									Pag->Layers[Pag->LayerCount].BasePic = sc.String;
									Pag->Layers[Pag->LayerCount].DrawTex = true;
									sc.MustGetToken(',');//font name
									sc.MustGetString();
									Pag->Layers[Pag->LayerCount].FontName = sc.String;
									if(sc.CheckToken(',')) //Set Color
									{
										sc.MustGetToken(TK_StringConst);
										Pag->Layers[Pag->LayerCount].FontNameColor = V_FindFontColor(sc.String);
									}
								}
								else if (sc.CheckString("offset"))
								{
									sc.MustGetToken('=');//x
									sc.MustGetFloat();
									Pag->Layers[Pag->LayerCount].X = sc.Float;

									sc.MustGetToken(',');//y
									sc.MustGetFloat();
									Pag->Layers[Pag->LayerCount].Y = sc.Float;
								}

								else if (sc.CheckString("resolution"))
								{
									sc.MustGetToken('=');//resolution W
									sc.MustGetNumber();
									Pag->Layers[Pag->LayerCount].ResW = sc.Number;

									sc.MustGetToken(',');//resolution H
									sc.MustGetNumber();
									Pag->Layers[Pag->LayerCount].ResH = sc.Number;
								}

								else if (sc.CheckString("scroll"))
								{
									sc.MustGetToken('=');//scrollx
									sc.MustGetFloat();
									Pag->Layers[Pag->LayerCount].ScrollX = sc.Float;

									sc.MustGetToken(',');//scrolly
									sc.MustGetFloat();
									Pag->Layers[Pag->LayerCount].ScrollY = sc.Float;
								}

								else if (sc.CheckString("scrolltime"))
								{
									sc.MustGetToken('=');//scroll count
									sc.MustGetNumber();
									Pag->Layers[Pag->LayerCount].ScrollTime  = sc.Number;
									Pag->Layers[Pag->LayerCount].SScrollTime  = sc.Number;
								}

								else if (sc.CheckString("lightlevel"))
								{
									sc.MustGetToken('=');//lightlevel
									sc.MustGetNumber();
									Pag->Layers[Pag->LayerCount].Light  = sc.Number;
								}

								else if (sc.CheckString("lightto"))
								{
									sc.MustGetToken('=');//light to level
									sc.MustGetNumber();
									Pag->Layers[Pag->LayerCount].LightDest  = sc.Number;

									sc.MustGetToken(',');//light speed
									sc.MustGetNumber();
									Pag->Layers[Pag->LayerCount].LightSpeed = sc.Number;
								}

								else if (sc.CheckString("alphalevel"))
								{
									sc.MustGetToken('=');//alphalevel
									sc.MustGetFloat();
									Pag->Layers[Pag->LayerCount].Alpha  = FLOAT2FIXED(sc.Float);
								}

								else if (sc.CheckString("alphato"))
								{
									sc.MustGetToken('=');//alpha to level
									sc.MustGetFloat();
									Pag->Layers[Pag->LayerCount].AlphaDest  = FLOAT2FIXED(sc.Float);

									sc.MustGetToken(',');//light speed
									sc.MustGetFloat();
									Pag->Layers[Pag->LayerCount].AlphaSpeed = FLOAT2FIXED(sc.Float);
								}

								else if (sc.CheckString("delay"))
								{
									sc.MustGetToken('=');//delay
									sc.MustGetNumber();
									Pag->Layers[Pag->LayerCount].Delay  = sc.Number;
								}

								else if (sc.CheckString("drawfire"))
								{
									Pag->Layers[Pag->LayerCount].DrawFire = true;//drawfire
								}

								else if (sc.CheckString("checkepisode"))
								{
									Pag->Layers[Pag->LayerCount].CheckEpisode = true;//drawfire
								}

								else if (sc.CheckString("checkskill"))
								{
									Pag->Layers[Pag->LayerCount].CheckSkill = true;//drawfire
								}

								else
								{
									sc.ScriptError("Unknown keyword '%s'", sc.String);
									break;
								}
							}
							Pag->LayerCount += 1;//
						}
						else
						{
							sc.ScriptError("Unknown keyword '%s'", sc.String);
							break;
						}
					}
				AddPage(Pag);
				}
				else
				{
					sc.ScriptError("Unknown keyword '%s'", sc.String);
					break;
				}
			}
	}
}

//==========================================================================
//
// D_CustomPageTicker
//
//==========================================================================
#include "gl/renderer/gl_renderer.h"

void D_CustomPageTicker ()//[GEC]
{
	if(ForceSkipPage)
		D_AdvanceDemo();

	if(ResetCount && SetPage->ResetCount)
	{
		pagetic = SetPage->PageTime;
		ResetCount = false;
	}

	if(SetPage->RunFire)
		GLRenderer->R_FirePSXTicker_(SetPage->FireOut);

	for(int i = 0; i < SetPage->LayerCount; i++)
	{
		//if((SetPage->Layers[i].DelayCount >= SetPage->Layers[i].Delay))
		//Printf("delay %d\n",SetPage->Layers[i].DelayCount);
		if((SetPage->Layers[i].DelayCount >= SetPage->Layers[i].Delay))
		{
			if (SetPage->Layers[i].ScrollTime != 0)
			{
				SetPage->Layers[i].NewX += SetPage->Layers[i].ScrollX;
				SetPage->Layers[i].NewY += SetPage->Layers[i].ScrollY;
				SetPage->Layers[i].ScrollTime --;
			}

			if (SetPage->Layers[i].LightCount != 0)
			{
				SetPage->Layers[i].NewLight += SetPage->Layers[i].LightSpeed;
				SetPage->Layers[i].LightCount --;
			}

			if (SetPage->Layers[i].AlphaCount != 0)
			{
				SetPage->Layers[i].NewAlpha += SetPage->Layers[i].AlphaSpeed;
				SetPage->Layers[i].AlphaCount --;
			}
		}
		else
		{
			SetPage->Layers[i].DelayCount++;
		}
	}

	if (--pagetic < 0)
		D_AdvanceDemo ();
}

//==========================================================================
//
// D_CustomPageDrawer
//
//==========================================================================

void D_CustomPageDrawer ()//[GEC]
{
	if(Force_Clear && !SetPage->NoClear)
		screen->Clear (0, 0, SCREENWIDTH, SCREENHEIGHT, 0, 0);

	for(int i = 0; i < SetPage->LayerCount; i++)
	{
		if (SetPage->Layers[i].BasePic && (SetPage->Layers[i].DelayCount >= SetPage->Layers[i].Delay))
		{
			//Light
			int lightsrc = SetPage->Layers[i].Light;
			int lightdest = SetPage->Layers[i].LightDest;

			if (lightsrc < lightdest)
			{
				int temp = lightsrc;
				lightsrc = lightdest;
				lightdest = temp;
			}

			int light = clamp<int>((SetPage->Layers[i].Light + SetPage->Layers[i].NewLight),lightdest,lightsrc);

			//Alpha
			fixed_t alphasrc = SetPage->Layers[i].Alpha;
			fixed_t alphadest = SetPage->Layers[i].AlphaDest;

			if (alphasrc < alphadest)
			{
				int temp = alphasrc;
				alphasrc = alphadest;
				alphadest = temp;
			}

			fixed_t alpha = clamp<fixed_t>((SetPage->Layers[i].Alpha + SetPage->Layers[i].NewAlpha),alphadest,alphasrc);

			if(!SetPage->Layers[i].DrawTex)
			{
				FTexture *tex = TexMan(SetPage->Layers[i].BasePic.GetChars());
				double rx = double(SetPage->Layers[i].X + SetPage->Layers[i].NewX);
				double ry = double(SetPage->Layers[i].Y + SetPage->Layers[i].NewY);
				double rw = tex->GetScaledWidthDouble();
				double rh = tex->GetScaledHeightDouble();

				screen->VirtualToRealCoords(rx, ry, rw, rh, SetPage->Layers[i].ResW, SetPage->Layers[i].ResH, true);

				if(SetPage->Layers[i].DrawFire)
				{
					screen->DrawTexture(tex, rx, ry,
					DTA_DestWidthF, rw,
					DTA_DestHeightF, rh,
					DTA_Alpha, alpha,
					DTA_BilinearFilter, true,
					DTA_DrawFire, true,
					TAG_DONE);
				}
				else 
				{
					double X = double(SetPage->Layers[i].X + SetPage->Layers[i].NewX);
					double Y = double(SetPage->Layers[i].Y + SetPage->Layers[i].NewY);
					double Resw = SetPage->Layers[i].ResW;
					double Resh = SetPage->Layers[i].ResH;

					screen->DrawTexture (tex, X, Y, 
					DTA_Bottom320x200, false,// activate virtBottom
					DTA_VirtualWidthF, (double)Resw,
					DTA_VirtualHeightF, (double)Resh,
					DTA_Alpha, alpha,
					DTA_BilinearFilter, true,
					DTA_GLCOLOR,MAKEARGB(255,light,light,light),
					TAG_DONE);

					/*screen->DrawTexture(tex, rx, ry,
					DTA_DestWidthF, rw,
					DTA_DestHeightF, rh,
					DTA_Alpha, alpha,
					DTA_BilinearFilter, true,
					DTA_GLCOLOR,MAKEARGB(255,light,light,light),
					TAG_DONE);*/
				}
			}
			else if(SetPage->Layers[i].DrawTex)
			{
				FFont *Font = V_GetFont(SetPage->Layers[i].FontName.GetChars());

				const char *name = SetPage->Layers[i].BasePic.GetChars();
				if (*name == '$') name = GStrings(name+1);

				if(SetPage->Layers[i].CheckEpisode){name = AllEpisodes[GameStartupInfo.Episode].mEpisodeName.GetChars();}
				else if(SetPage->Layers[i].CheckSkill)
				{
					int skill = M_GetDefaultSkill();

					if(GameStartupInfo.Skill != -1)
						skill = GameStartupInfo.Skill;

					name = AllSkills[skill].MenuName.GetChars();
				}

				int X = int(SetPage->Layers[i].X + SetPage->Layers[i].NewX);
				int Y = int(SetPage->Layers[i].Y + SetPage->Layers[i].NewY);

				screen->DrawText (Font, SetPage->Layers[i].FontNameColor,
				X,Y, name, 
				DTA_ResWidthF, SetPage->Layers[i].ResW, 
				DTA_ResHeightF, SetPage->Layers[i].ResH, 
				DTA_Alpha, alpha,
				DTA_BilinearFilter, true,
				DTA_GLCOLOR,MAKEARGB(255,light,light,light),
				TAG_DONE);
			}
		}
	}
}

//==========================================================================
//
// D_DoCustomAdvanceDemo
//
//==========================================================================

void D_DoCustomAdvanceDemo ()//[GEC]
{
	pagetic = 0;
	advancedemo = false;
	Force_Wipe = false;
	Force_Clear = true;

	gamestate = GS_DEMOSCREEN;
	C_HideConsole ();

	//S_StopMusic(true);
	S_StopAllChannels ();

	//FPageDef *Pag = new FPageDef;/* = NULL*/;

	SetPage = new FPageDef;
	SetPage->SetDefaultPage();

	if(demosequence == -1)
	{
		//screen->Clear (0, 0, SCREENWIDTH, SCREENHEIGHT, 0, 0);
		// Search for existing next page.
		for (unsigned int i = 0; i < mPages.Size(); ++i)
		{
			if (mPages[i]->PageNum == SetPage->NextPage)
			{
				demosequence = i;
			}
		}
		return;
	}

	//Printf("demosequence %d, SkipPage %d\n", demosequence, SkipPage);

	if(ForceSkipPage)
	{
		// Search for existing next skip page.
		if(SkipPage != -1)
		{
			for (unsigned int i = 0; i < mPages.Size(); ++i)
			{
				if (mPages[i]->PageNum == SkipPage)
				{
					demosequence = i;
				}
			}
		}
		ForceSkipPage = false;
	}

	//Reset
	for(int i = 0; i < mPages[demosequence]->LayerCount; i++)
	{
			mPages[demosequence]->Layers[i].NewX = 0;
			mPages[demosequence]->Layers[i].NewY = 0;
			mPages[demosequence]->Layers[i].ScrollTime = mPages[demosequence]->Layers[i].SScrollTime;

			mPages[demosequence]->Layers[i].LightSpeed = abs(mPages[demosequence]->Layers[i].LightSpeed);
			mPages[demosequence]->Layers[i].NewLight = 0;
			mPages[demosequence]->Layers[i].LightCount = (mPages[demosequence]->Layers[i].LightDest - mPages[demosequence]->Layers[i].Light);

			if (mPages[demosequence]->Layers[i].LightCount < 0)
			{
				mPages[demosequence]->Layers[i].LightCount = -mPages[demosequence]->Layers[i].LightCount;
				mPages[demosequence]->Layers[i].LightSpeed = -mPages[demosequence]->Layers[i].LightSpeed;
			}

			mPages[demosequence]->Layers[i].AlphaSpeed = abs(mPages[demosequence]->Layers[i].AlphaSpeed);
			mPages[demosequence]->Layers[i].NewAlpha = 0;
			mPages[demosequence]->Layers[i].AlphaCount = (mPages[demosequence]->Layers[i].AlphaDest - mPages[demosequence]->Layers[i].Alpha);

			if (mPages[demosequence]->Layers[i].AlphaCount < 0)
			{
				mPages[demosequence]->Layers[i].AlphaCount = -mPages[demosequence]->Layers[i].AlphaCount;
				mPages[demosequence]->Layers[i].AlphaSpeed = -mPages[demosequence]->Layers[i].AlphaSpeed;
			}

			mPages[demosequence]->Layers[i].DelayCount = 0;
	}

	SetPage = mPages[demosequence];

	// Search for existing next page.
	for (unsigned int i = 0; i < mPages.Size(); ++i)
	{
		if (mPages[i]->PageNum == SetPage->NextPage)
		{
			demosequence = i;
		}
	}

	//Printf("NextPage demosequence %d\n", demosequence);

	pagetic = SetPage->PageTime;

	SkipPage = SetPage->SkipPage;
	NoMenu = SetPage->NoMenu;
	DrawMenu = SetPage->DrawMenu;

	if(SetPage->DisableDim){DisableDim = true;}
	else{DisableDim = false;}

	if(DrawMenu)
	{
		MenuReset = true;
		M_SetMenu(NAME_Mainmenu, -1);
		DrawMenu = false;
	}

	if(SetPage->ClearMenu)
	{
		M_ClearMenus();
	}

	if(SetPage->ResetAnimation){TexMan.ResetAnimations();}//[GEC]
	if(SetPage->MusicName){S_ChangeMusic (SetPage->MusicName.GetChars(), 0, false);}
	if(SetPage->SoundName){S_Sound (CHAN_VOICE | CHAN_UI, SetPage->SoundName.GetChars(), 1, ATTN_NORM);}
	if(SetPage->FireInit){GLRenderer->R_InitFirePSX_();}

	if(SetPage->ForceWipe != GS_DEMOSCREEN)
		wipegamestate = SetPage->ForceWipe;
	
	if(SetPage->TitleMap)
	{
		Force_Clear = false;
		if (P_CheckMapData(SetPage->TitleMap.GetChars()))
		{
			G_InitNew (SetPage->TitleMap.GetChars(), true);
			return;
		}
	}
	else if(SetPage->DemoName)
	{
		G_DeferedPlayDemo (SetPage->DemoName.GetChars());
	}
}