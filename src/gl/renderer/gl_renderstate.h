#ifndef __GL_RENDERSTATE_H
#define __GL_RENDERSTATE_H

#include <string.h>
#include "c_cvars.h"
#include "r_defs.h"

EXTERN_CVAR(Bool, gl_direct_state_change)

struct FStateAttr
{
	static int ChangeCounter;
	int mLastChange;

	FStateAttr()
	{
		mLastChange = -1;
	}

	bool operator == (const FStateAttr &other)
	{
		return mLastChange == other.mLastChange;
	}

	bool operator != (const FStateAttr &other)
	{
		return mLastChange != other.mLastChange;
	}

};

struct FStateVec3 : public FStateAttr
{
	float vec[3];

	bool Update(FStateVec3 *other)
	{
		if (mLastChange != other->mLastChange)
		{
			*this = *other;
			return true;
		}
		return false;
	}

	void Set(float x, float y, float z)
	{
		vec[0] = x;
		vec[1] = z;
		vec[2] = y;
		mLastChange = ++ChangeCounter;
	}
};

struct FStateVec4 : public FStateAttr
{
	float vec[4];

	bool Update(FStateVec4 *other)
	{
		if (mLastChange != other->mLastChange)
		{
			*this = *other;
			return true;
		}
		return false;
	}

	void Set(float r, float g, float b, float a)
	{
		vec[0] = r;
		vec[1] = g;
		vec[2] = b;
		vec[3] = a;
		mLastChange = ++ChangeCounter;
	}
};


enum EEffect
{
	EFF_NONE,
	EFF_FOGBOUNDARY,
	EFF_SPHEREMAP,
};

class FRenderState
{
	bool mTextureEnabled;
	bool mFogEnabled;
	bool mGlowEnabled;
	bool mLightEnabled;
	bool mBrightmapEnabled;
	bool mColorMask[4];
	bool currentColorMask[4];
	int mSpecialEffect;
	int mTextureMode;
	float mDynLight[3];
	float mLightParms[2];
	int mNumLights[3];
	float *mLightData;
	int mSrcBlend, mDstBlend;
	int mAlphaFunc;
	float mAlphaThreshold;
	bool mAlphaTest;
	int mBlendEquation;
	bool m2D;

	FStateVec3 mCameraPos;
	FStateVec4 mGlowTop, mGlowBottom;
	FStateVec4 mGlowTopPlane, mGlowBottomPlane;
	PalEntry mFogColor;
	float mFogDensity;

	int mEffectState;
	int mColormapState;
	float mWarpTime;

	int stAlphaFunc;
	float stAlphaThreshold;
	int stSrcBlend, stDstBlend;
	bool stAlphaTest;
	int stBlendEquation;

	bool ffTextureEnabled;
	bool ffFogEnabled;
	int ffTextureMode;
	int ffSpecialEffect;
	PalEntry ffFogColor;
	float ffFogDensity;

	bool ApplyShader();

	float mFragColor[4];//[GEC]
	int	mSetLightMode;//[GEC]
	float mPsxBrightLevel;//[GEC]
	float mLight64;//[GEC]
	PalEntry mBlendColor;//[GEC]
	int	mBlendMode;//[GEC]
	int	mWrapS, mWrapT;//[GEC] Only shader Brightmaps

public:
	FRenderState()
	{
		Reset();
	}

	void Reset();

	int SetupShader(bool cameratexture, int &shaderindex, int &cm, float warptime);
	void Apply(bool forcenoshader = false);
	void ApplyColorMask();

	void ResetSpecials()//[GEC]
	{
		mLight64 = 0.0;
		mSetLightMode = 0;
		mPsxBrightLevel = 1.0;

		mBlendColor = PalEntry(0,0,0,0);
		mBlendMode = 0;
		mWrapS = 0;
		mWrapT = 0;
	}

	void SetLightMode(int mode)//[GEC]
	{
		mSetLightMode = mode;
	}

	void SetPsxBrightLevel(int Level)//[GEC]
	{
		mPsxBrightLevel = ((float)(255 + Level) / 255.0f);
	}

	void SetLight64(int Level)//[GEC]
	{
		mLight64 = ((float)(Level) / 255.0f);
	}

	void SetBlendColor(PalEntry BlendColor, int mode)//[GEC]
	{
		mBlendColor = BlendColor;
		mBlendMode = mode;
	}

	void SetWarpTexture(int WrapS, int WrapT)//[GEC]
	{
		mWrapS = WrapS;
		mWrapT = WrapT;
	}

	void GetColorMask(bool& r, bool &g, bool& b, bool& a) const
	{
		r = mColorMask[0];
		g = mColorMask[1];
		b = mColorMask[2];
		a = mColorMask[3];
	}

	void SetColorMask(bool r, bool g, bool b, bool a)
	{
		mColorMask[0] = r;
		mColorMask[1] = g;
		mColorMask[2] = b;
		mColorMask[3] = a;
	}

	void ResetColorMask()
	{
		for (int i = 0; i < 4; ++i)
			mColorMask[i] = true;
	}

	void SetTextureMode(int mode)
	{
		mTextureMode = mode;
	}

	void EnableTexture(bool on)
	{
		mTextureEnabled = on;
	}

	void EnableFog(bool on)
	{
		mFogEnabled = on;
	}

	void SetEffect(int eff)
	{
		mSpecialEffect = eff;
	}

	void EnableGlow(bool on)
	{
		mGlowEnabled = on;
	}

	void EnableLight(bool on)
	{
		mLightEnabled = on;
	}

	void EnableBrightmap(bool on)
	{
		mBrightmapEnabled = on;
	}

	void SetCameraPos(float x, float y, float z)
	{
		mCameraPos.Set(x,y,z);
	}

	void SetGlowParams(float *t, float *b)
	{
		mGlowTop.Set(t[0], t[1], t[2], t[3]);
		mGlowBottom.Set(b[0], b[1], b[2], b[3]);
	}

	void SetGlowPlanes(const secplane_t &top, const secplane_t &bottom)
	{
		mGlowTopPlane.Set(FIXED2FLOAT(top.a), FIXED2FLOAT(top.b), FIXED2FLOAT(top.ic), FIXED2FLOAT(top.d));
		mGlowBottomPlane.Set(FIXED2FLOAT(bottom.a), FIXED2FLOAT(bottom.b), FIXED2FLOAT(bottom.ic), FIXED2FLOAT(bottom.d));
	}

	void SetDynLight(float r, float g, float b)
	{
		mDynLight[0] = r;
		mDynLight[1] = g;
		mDynLight[2] = b;
	}

	void GetDynLight(float * r, float * g, float * b)//[GEC]
	{
		*r = mDynLight[0];
		*g = mDynLight[1];
		*b = mDynLight[2];
	}

	void SetFragColor(float r, float g, float b, float a)//[GEC]
	{
		mFragColor[0] = r;
		mFragColor[1] = g;
		mFragColor[2] = b;
		mFragColor[3] = a;
	}

	void GetFragColor(float * r, float * g, float * b,  float * a)//[GEC]
	{
		*r = mFragColor[0];
		*g = mFragColor[1];
		*b = mFragColor[2];
		*a = mFragColor[3];
	}

	void GetTextureMode(int *mode)//[GEC]
	{
		*mode = mTextureMode;
	}

	void SetFog(PalEntry c, float d)
	{
		mFogColor = c;
		if (d >= 0.0f) mFogDensity = d;
	}

	void SetLightParms(float f, float d)
	{
		mLightParms[0] = f;
		mLightParms[1] = d;
	}

	void SetLights(int *numlights, float *lightdata)
	{
		mNumLights[0] = numlights[0];
		mNumLights[1] = numlights[1];
		mNumLights[2] = numlights[2];
		mLightData = lightdata;	// caution: the data must be preserved by the caller until the 'apply' call!
	}

	void SetFixedColormap(int cm)
	{
		mColormapState = cm;
	}

	PalEntry GetFogColor() const
	{
		return mFogColor;
	}

	void BlendFunc(int src, int dst)
	{
		if (!gl_direct_state_change)
		{
			mSrcBlend = src;
			mDstBlend = dst;
		}
		else
		{
			glBlendFunc(src, dst);
		}
	}

	void AlphaFunc(int func, float thresh)
	{
		if (!gl_direct_state_change)
		{
			mAlphaFunc = func;
			mAlphaThreshold = thresh;
		}
		else
		{
			::glAlphaFunc(func, thresh);
		}
	}

	void EnableAlphaTest(bool on)
	{
		if (!gl_direct_state_change)
		{
			mAlphaTest = on;
		}
		else
		{
			if (on) glEnable(GL_ALPHA_TEST);
			else glDisable(GL_ALPHA_TEST);
		}
	}

	void BlendEquation(int eq)
	{
		if (!gl_direct_state_change)
		{
			mBlendEquation = eq;
		}
		else
		{
			glBlendEquation(eq);
		}
	}

	void Set2DMode(bool on)
	{
		m2D = on;
	}
};

extern FRenderState gl_RenderState;

#endif
