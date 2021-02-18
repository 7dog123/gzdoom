//===========================================================================
// 
//	Texture Combiners
//  Functions
//
//===========================================================================

#include "gl/system/gl_system.h"
#include "c_console.h"
#include "v_palette.h"
#include "templates.h"
#include "gl/textures/gl_material.h"
#include "gl/renderer/gl_renderstate.h"
#include "gl/system/gl_interface.h"
#include "gl/data/gl_data.h"
#include "gl/scene/gl_drawinfo.h"

EXTERN_CVAR(Int, blend_type);//[GEC]

extern bool nolights = false;
extern bool psx_blend = true;
extern bool psx_super_bright = false;
//extern PalEntry flashcolor = PalEntry(0xff, 0, 32, 4);
//extern PalEntry flashcolor = PalEntry((int)(0.25*255), 0, 255, 0);
//extern PalEntry flashcolor = 0;

extern PalEntry flashcolor
= 0;
//= PalEntry(0xff, 0, 32, 4);
//= PalEntry((int)(0.25*255), 0, 255, 0);

//extern bool psx_blend = false;
//extern PalEntry flashcolor = PalEntry(0xff, 0x80, 0x80, 0x80);

extern bool Multitexture = false;

extern bool SpriteFullBright = false;
extern int SpriteLightlevel = 0; 
extern int SpriteRel = 0;
extern int CheckShader = 0;
extern int TextureMode = 0;//[GEC]

void Modulate_RGB()
{
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
}

void SetLight64(int Level)
{
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	float f[4];
	f[0] = f[1] = f[2] = ((float)Level / 255.0f);
	glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, f);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_ADD);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_CONSTANT);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
}

void SetColor(int r, int g, int b, int a)
{
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	float f[4];
	f[0] = ((float)r / 255.0f);
	f[1] = ((float)g / 255.0f);
	f[2] = ((float)b / 255.0f);
	f[3] = ((float)a / 255.0f);
	glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, f);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);   //Modulate RGB with RGB
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_CONSTANT);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
}

void SetBlend(PalEntry FlashColor, int Mode)
{
	float blend[4];
	blend[0] = (float)(FlashColor.r & 0xff) / 255.0f;//Red
	blend[1] = (float)(FlashColor.g & 0xff) / 255.0f;//Green
	blend[2] = (float)(FlashColor.b & 0xff) / 255.0f;//Blue

	if(Mode == GL_INTERPOLATE)//PSX alpha
		blend[3] = (float)((256 - FlashColor.a) & 0xff) / 255.0f;
	else
		blend[3] = (float)(FlashColor.a & 0xff) / 255.0f;

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, blend);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, Mode);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_CONSTANT);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);

	//flashcolor = 0;//[GEC]
}

void SetPSXSuperBright(int Level)
{
	float f[4];
	f[0] = f[1] = f[2] = ((float)(256 + Level) / 255.0f);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, f);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_CONSTANT);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
}

void SetCombineColor()
{
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PRIMARY_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
}

void SetCombineAlpha()
{
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_TEXTURE0);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA, GL_PRIMARY_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);
}

void Disable_texunits()
{
	for(int texunit = 0; texunit < 16; texunit++)
	{
		glActiveTexture(GL_TEXTURE0+texunit);
		glDisable(GL_TEXTURE_2D);
		//glBindTexture(GL_TEXTURE_2D,0+texunit);
	}

    glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
   
	//glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

void Enable_texunit(int texunit)
{
	glActiveTexture(GL_TEXTURE0+texunit);
	glEnable( GL_TEXTURE_2D );
}

int PSXDoomLightingEquation(float light, float Dist, int type)
{
	float Lightscale;

	float Factor;
	if(type == 0)//WALL
	{
		Factor = (200.0 - Dist)/(500.0);//WALL
	}
	else if(type == 1)//PLANE
	{
		Factor = (200.0 - Dist)/(1000.0);
	}
	
	Factor = clamp<float>(Factor, 0.0, (50.0/255.0)) * 255.0;

	float L = (light * 255.0);
	L = clamp<float>(L - 8.0, 0.0, 255.0)/2.0;
	
	float Shade = clamp<float>((L * (32.0 + Factor) + 32.0 / 2.0) / 32.0, L, 256.0);
	
	//if (uPalLightLevels != 0)
		Lightscale = clamp<float>((float(int(fabs(Shade / 4.0))) * 4.0)/ 255.0, 0.0, 1.0);
	//else
		//Lightscale = clamp<float>(Shade / 255.0, 0.0, 1.0);

	return int(Lightscale * 255.0);
}


static int curunit = -1;

//
// GL_SetTextureUnit
//

void SetTextureUnit(int unit, bool enable) 
{
    /*if(!has_GL_ARB_multitexture) {
        return;
    }

    if(r_fillmode.value <= 0) {
        return;
    }*/

    if(unit > 16) 
	{
        return;
    }

    if(curunit == unit) 
	{
        return;
    }

    curunit = unit;

   // dglActiveTextureARB(GL_TEXTURE0_ARB + unit);
   // GL_SetState(GLSTATE_TEXTURE0 + unit, enable);

	glActiveTexture(GL_TEXTURE0 + unit);
	if(enable)
		glEnable(GL_TEXTURE_2D);
	else
		glDisable(GL_TEXTURE_2D);
}

void SetTextureMode(int Mode)
{
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	float f[4];
	f[0] = f[1] = f[2] = ((float)255 / 255.0f);
	f[3] = 0.0f;

	if(Mode == 0)//TM_MODULATE
		Mode = GL_MODULATE;
	else if(Mode == 1)//TM_MASK
		Mode = GL_INTERPOLATE;
	else
		Mode = GL_MODULATE;

	glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, f);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, Mode);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE0);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_CONSTANT);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
}

int Texturemode		= 0;
int LightMode		= 0;
int LightLevel		= 0;
int RelLight		= 0;
int LightLevel64	= 0;
bool FullBright		= false;
float FragColor[3]	= {0,0,0};	// RGB
float DynColor[3]	= {0,0,0};	// Dynamic Light RGB

void ResetSpecials()
{
	Texturemode = 0;
	LightMode = 0;
	LightLevel = 255;
	RelLight = 0;
	LightLevel64 = 0;
	FullBright = false;
	FragColor[0] = 1.0;
	FragColor[1] = 1.0;
	FragColor[2] = 1.0;
	DynColor[0] = 0.0;
	DynColor[1] = 0.0;
	DynColor[2] = 0.0;
}

void SetInitSpecials(int texturemode, int lightmode, int lightlevel, int rellight, 
					 int lightlevel64, bool fullbright, float* fragcolor, float* dyncolor)
{
	Texturemode = texturemode;
	LightMode = lightmode;
	LightLevel = lightlevel;
	RelLight = rellight;
	LightLevel64 = lightlevel64;
	FullBright = fullbright;
	FragColor[0] = fragcolor[0];
	FragColor[1] = fragcolor[1];
	FragColor[2] = fragcolor[2];
	DynColor[0] = dyncolor[0];
	DynColor[1] = dyncolor[1];
	DynColor[2] = dyncolor[2];
}


/*static GLuint SoftLightFlat = -1;
static int lastenvcolor = 0;
static int lastlightlevel = 0;

static unsigned int generate_softlightflat(int color, int distan)
{
    int i;
	int W = 1;
	int H = 400;
	byte *buffer;
	int li = 0;
	int l = 0;
	int k = 0;
	byte lightpalette[400];

	if(lastenvcolor == color) 
	{
		glBindTexture( GL_TEXTURE_2D, SoftLightFlat);
        return SoftLightFlat;
    }

	glDeleteTextures(1, &SoftLightFlat);

    lastenvcolor    = color;

	buffer = (byte *)calloc(sizeof(byte),(W*H));
	memset(buffer, 0xff, sizeof(byte) * (W*H));

	li = 200;
	for (i = 0;i < 200; i++)
	{
		lightpalette[i] = PSXDoomLightingEquation(color/255.0, li, 1);
		li--;
	}
	li = 0;
	for (i = 200;i < 400; i++)//H
	{
		lightpalette[i] = PSXDoomLightingEquation(color/255.0, li, 1);
		li++;
	}

	int count = 0;
	for(i = 0; i < H; i++)
	{
		buffer[count] = (unsigned char)(lightpalette[i] & 0xff);
		count++;
	}

	if(SoftLightFlat == 0)
		glGenTextures(1, &SoftLightFlat);

    glBindTexture( GL_TEXTURE_2D, SoftLightFlat);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, W, H, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, buffer);

	free (buffer);
	return SoftLightFlat;
}*/

void SetNewSpecials(FMaterial * gltexture, int cm, bool Patch, bool weapon, bool flat)//[GEC]
{
	float DynR, DynG, DynB;
	float FragR, FragG, FragB, FragA;
	int newR, newG, newB;
	int texnum = 0;

	//Pass 1: Light64
	{
		if(!gl_fixedcolormap)
			gl_RenderState.SetLight64(LightLevel64);//[GEC]
		else
			gl_RenderState.SetLight64(0);//[GEC]
	}

	//Pass 2: Blend Psx if true
	{
		if(psx_blend)
		{
			gl_RenderState.SetBlendColor(flashcolor, 0);
		}
	}

	//Pass 4: Light Software
	{
		if(LightMode == 16)
		{
			if(!gl_fixedcolormap)
			{
				if(weapon) 
					gl_RenderState.SetLightMode(0);
				else if(flat) 
					gl_RenderState.SetLightMode(2);
				else//Sprite
					gl_RenderState.SetLightMode(3);
			}
		}
	}

	//Pass 5: Blend D64 if false
	{
		if(!psx_blend)
		{
			if(weapon)
				gl_RenderState.SetBlendColor(flashcolor, 2);
			else
				gl_RenderState.SetBlendColor(flashcolor, 1);
		}
	}

	//Pass 6: SetSuperBright
	{
		if(LightMode == 16)
		{
			if (!gl_fixedcolormap || FullBright)
			{
				gl_RenderState.SetPsxBrightLevel(FullBright ? 64 : 255);//[GEC]
			}
		}
	}
	gl_RenderState.Apply();//[GEC]

	if ((gl.shadermodel < 4))
	{
		//Pass 0: Base Textrue
		SetTextureUnit(0, true);

		texnum ++;

		if(Patch)
			gltexture->BindPatch(cm,0,0,texnum);
		else
			gltexture->Bind(cm,0,0,0,texnum);

		SetTextureUnit(texnum, true);
		SetTextureMode(Texturemode);

		//Pass 1: Light64
		{
			texnum ++;

			if(Patch)
				gltexture->BindPatch(cm,0,0,texnum);
			else
				gltexture->Bind(cm,0,0,0,texnum);

			SetTextureUnit(texnum, true);
			if(!gl_fixedcolormap)
				SetLight64(LightLevel64);
			else
				SetLight64(0);
		}

		//Pass 2: Blend Psx if true
		{
			if(psx_blend)
			{
				texnum ++;

				if(Patch)
					gltexture->BindPatch(cm,0,0,texnum);
				else
					gltexture->Bind(cm,0,0,0,texnum);

				SetTextureUnit(texnum, true);
				SetBlend(flashcolor, GL_INTERPOLATE);
				if(APART(flashcolor) == 0)(SetBlend(flashcolor, GL_ADD));
			}
		}

		//Pass 3: SetColor
		{
			texnum ++;

			if(Patch)
				gltexture->BindPatch(cm,0,0,texnum);
			else
				gltexture->Bind(cm,0,0,0,texnum);

			SetTextureUnit(texnum, true);

			if(weapon)
				SetCombineColor();
			else if(LightMode == 16)
				SetColor(0xff, 0xff, 0xff, 0xff);
			else
				SetCombineColor();
		}

		//Pass 4: Light Software
		{
			if(weapon)
			{
				texnum ++;
				if(Patch)
					gltexture->BindPatch(cm,0,0,texnum);
				else
					gltexture->Bind(cm,0,0,0,texnum);
				SetTextureUnit(texnum, true);
				SetColor(0xff, 0xff, 0xff, 0xff);
			}
			else if(LightMode == 16)
			{
				texnum ++;
				int level = (!gl_fixedcolormap)? PSXDoomLightingEquation(LightLevel / 255.0, 64.0, 1) : 255;
				if(flat) {level = (!gl_fixedcolormap)? PSXDoomLightingEquation(LightLevel / 255.0, 255.0, 1) : 255; }

				if(Patch)
					gltexture->BindPatch(cm,0,0,texnum);
				else
					gltexture->Bind(cm,0,0,0,texnum);

				SetTextureUnit(texnum, true);
			
				gl_RenderState.GetFragColor(&FragR, &FragG, &FragB, &FragA);//Get FragColor
				gl_RenderState.GetDynLight(&DynR, &DynG, &DynB);//Get DynLight Color

				FragR *= level/255.f;
				FragG *= level/255.f;
				FragB *= level/255.f;

				newR = (int)(255.0f * clamp<float>(DynR + FragR, 0, 1.0f)) & 0xff;
				newG = (int)(255.0f * clamp<float>(DynG + FragG, 0, 1.0f)) & 0xff;
				newB = (int)(255.0f * clamp<float>(DynB + FragB, 0, 1.0f)) & 0xff;
				SetColor(newR, newG, newB, 0xff);
			}
		}

		//Pass 5: Blend D64 if false
		{
			if(!psx_blend)
			{
				texnum ++;

				if(Patch)
					gltexture->BindPatch(cm,0,0,texnum);
				else
					gltexture->Bind(cm,0,0,0,texnum);

				SetTextureUnit(texnum, true);
				SetBlend(flashcolor, GL_ADD);
			}
		}

		//Pass 6: SetSuperBright
		{
			if(LightMode == 16)
			{
				if (!gl_fixedcolormap || FullBright)
				{
					texnum ++;

					if(Patch)
						gltexture->BindPatch(cm,0,0,texnum);
					else
						gltexture->Bind(cm,0,0,0,texnum);

					SetTextureUnit(texnum, true);
					SetPSXSuperBright(FullBright ? 64 : 255);
				}
			}
		}

		SetCombineAlpha();
		SetTextureUnit(0, true);
	}
}
