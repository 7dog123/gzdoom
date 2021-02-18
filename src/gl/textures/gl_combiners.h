#ifndef __GL_COMBINERS
#define __GL_COMBINERS

#include "gl/system/gl_system.h"
#include "gl/textures/gl_material.h"
#include "gl/scene/gl_drawinfo.h"

extern bool nolights;
extern bool psx_blend;
extern bool psx_super_bright;
//extern int flashcolor;
extern PalEntry	flashcolor;

extern bool Multitexture;

extern bool SpriteFullBright;
extern int SpriteLightlevel;
extern int SpriteRel;
extern int CheckShader;
extern int TextureMode;//[GEC]

void Modulate_RGB();
void SetLight64(int Level);
void SetColor(int r, int g, int b, int a);
void SetBlend(PalEntry FlashColor, int Mode);
void SetPSXSuperBright(int Level);
void SetCombineColor();
void SetCombineAlpha();
void Disable_texunits();
void Enable_texunit(int texnum);
int PSXDoomLightingEquation(float light, float Dist, int type);
void SetTextureUnit(int unit, bool enable);

void SetTextureMode(int Mode);

void ResetSpecials();
void SetInitSpecials(int texturemode, int lightmode, int lightlevel, int rellight, 
					 int lightlevel64, bool fullbright, float* fragcolor, float* dyncolor);
void SetNewSpecials(FMaterial * gltexture, int cm, bool Patch, bool weapon, bool flat = false);

struct SpriteSpecials
{
	int LightMode;		
	bool FullBright;
	int LightLevel64;
	int LightLevel;
	int RelLight;
	float FragColor[3];	// RGB
	float DynColor[3];	// Dynamic Light RGB
	//float SetColor;		//RGB
	int TextureMode;

	void SetFragColor(float r, float g, float b)//[GEC]
	{
		FragColor[0] = r;
		FragColor[1] = g;
		FragColor[2] = b;
	}
	void SetDynLight(float r, float g, float b)//[GEC]
	{
		DynColor[0] = r;
		DynColor[1] = g;
		DynColor[2] = b;
	}

	void Reset()//[GEC]
	{
		DynColor[0] = 1.0;
		DynColor[1] = 1.0;
		DynColor[2] = 1.0;
		FragColor[0] = 1.0;
		FragColor[1] = 1.0;
		FragColor[2] = 1.0;

		LightMode = 0;
		FullBright = false;
		LightLevel64 = 0;
		LightLevel = 0;
		RelLight = 0;
		TextureMode = 0;
	}
};
#endif