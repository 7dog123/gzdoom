#ifndef __INTERMISSION_H
#define __INTERMISSION_H

#include "doomdef.h"
#include "dobject.h"
#include "m_fixed.h"
#include "textures/textures.h"
#include "s_sound.h"
#include "v_font.h"

struct event_t;

#define DECLARE_SUPER_CLASS(cls,parent) \
private: \
	typedef parent Super; \
	typedef cls ThisClass;

struct FIntermissionPatch
{
	FName mCondition;
	FString mName;
	double x, y;
};

struct FIIntermissionPatch
{
	FName mCondition;
	FTextureID mPic;
	double x, y;
};

struct FCastSound
{
	BYTE mSequence;
	BYTE mIndex;
	FString mSound;
};

struct FICastSound
{
	BYTE mSequence;
	BYTE mIndex;
	FSoundID mSound;
};

enum EFadeType
{
	FADE_In,
	FADE_Out,
};

enum EScrollDir
{
	SCROLL_Left,
	SCROLL_Right,
	SCROLL_Up,
	SCROLL_Down,
};

// actions that don't create objects
#define WIPER_ID ((const PClass*)intptr_t(-1))
#define TITLE_ID ((const PClass*)intptr_t(-2))

//==========================================================================

struct FIntermissionAction
{
	int mSize;
	const PClass *mClass;
	FString mMusic;
	FString mNextMusic;//[GEC]
	int mMusicOrder;
	int mCdTrack;
	int mCdId;
	int mDuration;
	FString mBackground;
	FString mPalette;
	FString mSound;
	bool mFlatfill;
	bool mMusicLooping;
	TArray<FIntermissionPatch> mOverlays;

	FIntermissionAction();
	virtual ~FIntermissionAction() {}
	virtual bool ParseKey(FScanner &sc);
};

struct FIntermissionActionFader : public FIntermissionAction
{
	typedef FIntermissionAction Super;

	EFadeType mFadeType;

	FIntermissionActionFader();
	virtual bool ParseKey(FScanner &sc);
};

struct FIntermissionActionWiper : public FIntermissionAction
{
	typedef FIntermissionAction Super;

	gamestate_t mWipeType;

	FIntermissionActionWiper();
	virtual bool ParseKey(FScanner &sc);
};

struct FIntermissionActionTextscreen : public FIntermissionAction
{
	typedef FIntermissionAction Super;

	FString mText;
	int mTextDelay;
	int mTextSpeed;
	int mTextX, mTextY;
	EColorRange mTextColor;

	// [GEC]
	bool		m_TextAdd;				//TextAdd
	int			m_TextSpeed;			//TextSpeed
	int			m_TextDelay;			//TextDelay
	int			m_TextResW, m_TextResH;	//ResolutionText
	int			m_TexX, m_TexY;			//TexPosition
	int			m_PicResW, m_PicResH;	//ResolutionPic
	int			m_PicX, m_PicY;			//PicPosition
	bool		m_CenterText;			//CenterText
	bool		m_FadeText;				//FadeTex
	bool		m_PlainText;			//PlainText
	bool		m_PlainSync;			//PlainSync
	bool		m_ScrollText;			//ScrollText
	int			m_ScrollTextSpeed;		//ScrollTextSpeed
	int			m_ScrollTextDirection;	//ScrollTextDirection
	int			m_ScrollTextTime;		//m_ScrollTextTime
	int			m_RowPadding;			//RowPadding
	FName		m_FontName;				//FontName
	gamestate_t	m_WipeOut;				//WipeOut
	bool		m_NoSkip;				//Noskip
	bool		m_AutoSkip;				//AutoSkip

	FIntermissionActionTextscreen();
	virtual bool ParseKey(FScanner &sc);
};

struct FIntermissionActionCast : public FIntermissionAction
{
	typedef FIntermissionAction Super;

	FString mName;
	FName mCastClass;
	TArray<FCastSound> mCastSounds;

	FName	mFontName;//FontName
	int		mYpos, mResW, mResH;//[GEC]
	FString mText;
	int		mYpos2, mXpos2;//[GEC]
	FString mShotSound;//[GEC]
	bool	mcastrotation;//[GEC]
	int		mlightmax;//[GEC]
	int		mlightplus;//[GEC]
	EColorRange mTextColor;//[GEC]
	int		mYposSprite;//[GEC]
	bool	mCenterXSprite;//[GEC]

	FIntermissionActionCast();
	virtual bool ParseKey(FScanner &sc);
};

struct FIntermissionActionScroller : public FIntermissionAction
{
	typedef FIntermissionAction Super;

	FString mSecondPic;
	int mScrollDelay;
	int mScrollTime;
	int mScrollDir;

	FIntermissionActionScroller();
	virtual bool ParseKey(FScanner &sc);
};

struct FIntermissionDescriptor
{
	FName mLink;
	TDeletingArray<FIntermissionAction *> mActions;
};

typedef TMap<FName, FIntermissionDescriptor*> FIntermissionDescriptorList;

extern FIntermissionDescriptorList IntermissionDescriptors;

//==========================================================================

class DIntermissionScreen : public DObject
{
	DECLARE_CLASS (DIntermissionScreen, DObject)

protected:
	int mDuration;
	FTextureID mBackground;
	bool mFlatfill;
	TArray<FIIntermissionPatch> mOverlays;

	bool CheckOverlay(int i);

public:
	int mTicker;
	bool mPaletteChanged;

	bool Run_mScrollTicker;		//[GEC] Run_mScrollTicker
	int  mScrollTicker;			//[GEC] mScrollTicker
	bool Run_mAlphaTicker;		//[GEC] Run_mAlphaTicker
	fixed_t  mAlphaTicker;		//[GEC] mAlphaTicker
	float  mAlpha;				//[GEC] mAlpha Colector

	DIntermissionScreen() {}
	virtual void Init(FIntermissionAction *desc, bool first);
	virtual int Responder (event_t *ev);
	virtual int Ticker ();
	virtual void Drawer ();
	void Destroy();
	FTextureID GetBackground(bool *fill)
	{
		*fill = mFlatfill;
		return mBackground;
	}
	void SetBackground(FTextureID tex, bool fill)
	{
		mBackground = tex;
		mFlatfill = fill;
	}
};

class DIntermissionScreenFader : public DIntermissionScreen
{
	DECLARE_CLASS (DIntermissionScreenFader, DIntermissionScreen)

	EFadeType mType;

public:

	DIntermissionScreenFader() {}
	virtual void Init(FIntermissionAction *desc, bool first);
	virtual int Responder (event_t *ev);
	virtual int Ticker ();
	virtual void Drawer ();
};

class DIntermissionScreenText : public DIntermissionScreen
{
	DECLARE_CLASS (DIntermissionScreenText, DIntermissionScreen)

	const char *mText;
	int mTextSpeed;
	int mTextX, mTextY;
	int mTextCounter;
	int mTextDelay;
	int mTextLen;
	EColorRange mTextColor;

	// [GEC]
	fixed_t* mAlpharows;	// [GEC]
	int* mTextCount;		// [GEC]
	int* mTextCount2;		// [GEC]
	int mCount;				// [GEC]
	int mNumRows;			// [GEC]
	int mRowPadding;		// [GEC]

	FName	mFontName;				//FontName
	bool	mTextAdd;				//TextAdd
	int		mTextResW, mTextResH;	//ResolutionText
	bool	mCenterText;			//CenterText
	bool	mFadeText;				//Fade
	bool	mPlainText;				//PlainText
	bool	mPlainSync;				//PlainSync
	bool	mScrollText;			//ScrollText
	int		mScrollTextSpeed;		//ScrollTextSpeed
	int		mScrollTextDirection;	//ScrollTextDirection
	int		mScrollTextTime;		//ScrollTextDirection

	bool mWaitScroll;	// [GEC]
	bool mWait_Aplha;	// [GEC]
	bool mNoSkip;		// [GEC]
	bool mskip;			// [GEC]
	bool mButtonPressed;// [GEC]
	bool mAutoSkip;		// [GEC]

public:

	DIntermissionScreenText() {}
	virtual void Init(FIntermissionAction *desc, bool first);
	virtual int Responder (event_t *ev);
	virtual void Drawer ();
};

class DIntermissionScreenCast : public DIntermissionScreen
{
	DECLARE_CLASS (DIntermissionScreenCast, DIntermissionScreen)

	const char *mName;
	const PClass *mClass;
	AActor *mDefaults;
	TArray<FICastSound> mCastSounds;

	int mYpos, mResW, mResH;//[GEC]
	FName	mFontName; //FontName
	EColorRange mTextColor;//[GEC]

	FString mText;
	int		mYpos2, mXpos2, mResW2, mResH2;//[GEC]
	FString mShotSound;//[GEC]
	int		mYposSprite;//[GEC]
	bool	mCenterXSprite;//[GEC]

	bool	mcastrotationenable;//[GEC]
	int		mcastrotation;//[GEC]
	int		mcastlight;//[GEC]
	int		mlightplus;//[GEC]
	int		mlightmax;//[GEC]

	

	int 			casttics;
	const FRemapTable *casttranslation;	// [RH] Draw "our hero" with their chosen suit color
	FState*			caststate;
	FState*			basestate;
	FState*			advplayerstate;
	bool	 		castdeath;
	bool	 		castattacking;
	int 			castframes;
	int 			castonmelee;
	void PlayAttackSound();

public:

	DIntermissionScreenCast() {}
	virtual void Init(FIntermissionAction *desc, bool first);
	virtual int Responder (event_t *ev);
	virtual int Ticker ();
	virtual void Drawer ();
};

class DIntermissionScreenScroller : public DIntermissionScreen
{
	DECLARE_CLASS (DIntermissionScreenScroller, DIntermissionScreen)

	FTextureID mFirstPic;
	FTextureID mSecondPic;
	int mScrollDelay;
	int mScrollTime;
	int mScrollDir;

public:

	DIntermissionScreenScroller() {}
	virtual void Init(FIntermissionAction *desc, bool first);
	virtual int Responder (event_t *ev);
	virtual void Drawer ();
};

enum
{
	FSTATE_EndingGame = 0,
	FSTATE_ChangingLevel = 1,
	FSTATE_InLevel = 2
};

class DIntermissionController : public DObject
{
	DECLARE_CLASS (DIntermissionController, DObject)
	HAS_OBJECT_POINTERS

	FIntermissionDescriptor *mDesc;
	TObjPtr<DIntermissionScreen> mScreen;
	bool mDeleteDesc;
	bool mFirst;
	bool mAdvance, mSentAdvance;
	BYTE mGameState;
	int mIndex;

	bool NextPage();

public:
	static DIntermissionController *CurrentIntermission;

	DIntermissionController(FIntermissionDescriptor *mDesc = NULL, bool mDeleteDesc = false, BYTE state = FSTATE_ChangingLevel);
	bool Responder (event_t *ev);
	void Ticker ();
	void Drawer ();
	void Destroy();

	friend void F_AdvanceIntermission();
};


// Interface for main loop
bool F_Responder (event_t* ev);
void F_Ticker ();
void F_Drawer ();
void F_StartIntermission(FIntermissionDescriptor *desc, bool deleteme, BYTE state);
void F_StartIntermission(FName desc, BYTE state);
void F_EndFinale ();
void F_AdvanceIntermission();

#include "g_level.h"
// Create an intermission from old cluster data
void F_StartFinale (const char *music, int musicorder, int cdtrack, unsigned int cdid, const char *flat, 
					const char *text, INTBOOL textInLump, INTBOOL finalePic, INTBOOL lookupText, 
					bool ending, FName endsequence = NAME_None, cluster_info_t * cluster = NULL);



#endif
