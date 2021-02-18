#ifndef __PAGEDEFS
#define __PAGEDEFS

#define MAXPAGELAYER 128
struct FPageDef
{
	int		PageNum;
	int		PageTime;
	int		NextPage;
	bool	ResetAnimation;
	bool	FireInit;
	bool	RunFire;
	bool	FireOut;
	int		SkipPage;
	bool	NoMenu;
	bool	DrawMenu;
	bool	DisableDim;
	bool	ClearMenu;
	bool	ResetCount;
	bool	NoClear;
	gamestate_t	ForceWipe;

	FName DemoName;
	FName TitleMap;
	FName SoundName;
	FName MusicName;

	int LayerCount;
	struct FPageLayer
	{
		bool	DrawTex;		//
		FName	FontName;
		EColorRange FontNameColor;
		FName	BasePic;
		double	X, Y;			// XY Posición
		int		ResW, ResH;		// Resulucion De La Imagen
		int		Light;			// Light Bright
		int		NewLight;		// New Light Bright
		int		LightDest;		// Light Dest
		int		LightSpeed;		// Light Speed
		int		LightCount;		// Light Count
		double	ScrollX;		// Speed Scroll X
		double	ScrollY;		// Speed Scroll Y
		double	NewX;			// New X
		double	NewY;			// New Y
		int		ScrollTime;		// Time Scroll
		int		SScrollTime;	// Save Time Scroll
		bool	DrawFire;		// Draw Firepic
		int		Delay;			// Delay draw
		int		DelayCount;		// Delay draw
		bool	CheckEpisode;
		bool	CheckSkill;

		//fixed_t ItemFlash;
		fixed_t	Alpha;			// Alpha
		fixed_t	NewAlpha;		// NewAlpha
		fixed_t	AlphaDest;		// AlphaDest
		fixed_t	AlphaSpeed;		// AlphaSpeed
		fixed_t	AlphaCount;		// AlphaCount
	} Layers[MAXPAGELAYER];

	void SetDefaultPage()
	{
		this->ForceWipe = GS_DEMOSCREEN;
		this->LayerCount = 0;
		this->NextPage = 0;
		this->PageNum = 0;
		this->PageTime = 0;
		this->ResetAnimation = false;
		this->FireInit = false;
		this->RunFire = false;
		this->FireOut = false;
		this->DemoName = NULL;
		this->TitleMap = NULL;
		this->SoundName = NULL;
		this->MusicName = NULL;
		this->SkipPage = -1;
		this->NoMenu = false;
		this->DrawMenu = false;
		this->DisableDim = false;
		this->ClearMenu = false;
		this->ResetCount = false;
		this->NoClear = false;

		for(int i = 0; i < MAXPAGELAYER; i++)
		{
			this->Layers[i].FontName = "BigFont";
			this->Layers[i].FontNameColor = CR_UNTRANSLATED;
			this->Layers[i].DrawTex = false;
			this->Layers[i].BasePic = NULL;
			this->Layers[i].Light = 255;
			this->Layers[i].NewLight = 0;
			this->Layers[i].LightDest = 0;
			this->Layers[i].LightCount = 0;
			this->Layers[i].LightSpeed = 0;
			this->Layers[i].ResW = 320;
			this->Layers[i].ResH = 240;
			this->Layers[i].ScrollX = 0;
			this->Layers[i].ScrollY = 0;
			this->Layers[i].NewX = 0;
			this->Layers[i].NewY = 0;
			this->Layers[i].ScrollTime = 0;
			this->Layers[i].SScrollTime = 0;
			this->Layers[i].X = 0;
			this->Layers[i].Y = 0;
			this->Layers[i].DrawFire = false;
			this->Layers[i].Delay = 0;
			this->Layers[i].NewAlpha = 0;
			this->Layers[i].Alpha = FRACUNIT;
			this->Layers[i].AlphaDest = 0;
			this->Layers[i].AlphaSpeed = 0;
			this->Layers[i].AlphaCount = 0;
			this->Layers[i].CheckEpisode = false;
			this->Layers[i].CheckSkill = false;
		}
	}
};

void D_CustomPageDrawer();
void D_CustomPageTicker();
void D_DoCustomAdvanceDemo();
void ParsePageDefs();

#endif