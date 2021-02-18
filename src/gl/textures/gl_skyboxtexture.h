

//-----------------------------------------------------------------------------
//
// This is not a real texture but will be added to the texture manager
// so that it can be handled like any other sky.
//
//-----------------------------------------------------------------------------

//
// [kex] sky definitions //NEW for [GEC]
//
enum 
{
    SKFS_CLOUD			= 0x001,
    SKFS_THUNDER		= 0x002,
    SKFS_FIRE			= 0x004,
    SKFS_BACKGROUND		= 0x008,
    SKFS_FADEBACK		= 0x010,
    SKFS_VOID			= 0x020,
	SKFS_SKYPSX			= 0x040, //NEW for [GEC]
	SKFS_FIREPSX		= 0x080, //NEW for [GEC]
	SKFS_RAISINGFIRE	= 0x100, //NEW for [GEC]
	SKFS_FIREPSX_GREY	= 0x200, //NEW for [GEC]
	SKFS_AJUSTTOPOFFSET	= 0x400, //NEW for [GEC]
};

class FSkyBox : public FTexture
{
public:

	FTexture * faces[6];
	bool fliptop;

	// D64 & DPSX sky styles [GEC]
	//{
	bool skyconsole;
	int  flags;
	int basecolor;
	int highcolor;
	int lowcolor;
	//}

	FSkyBox();
	~FSkyBox();
	const BYTE *GetColumn (unsigned int column, const Span **spans_out);
	const BYTE *GetPixels ();
	int CopyTrueColorPixels(FBitmap *bmp, int x, int y, int rotate, FCopyInfo *inf);
	bool UseBasePalette();
	void Unload ();
	void PrecacheGL();

	void SetSize()
	{
		if (faces[0]) 
		{
			Width=faces[0]->GetWidth();
			Height=faces[0]->GetHeight();
			CalcBitSize();
		}
	}

	bool Is3Face() const
	{
		return faces[5]==NULL;
	}

	bool IsFlipped() const
	{
		return fliptop;
	}
};
