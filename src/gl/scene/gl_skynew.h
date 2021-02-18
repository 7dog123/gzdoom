#ifndef __GL_SKYNEW
#define __GL_SKYNEW

#include "gl/system/gl_system.h"
#include "gl/textures/gl_material.h"

// Returns a number from 0 to 255,
// from a lookup table.
extern byte rndtable[256];
extern int rndindex;
extern int prndindex;
extern int fndindex;

int M_Random_(void);

// As M_Random, but used only by the play simulation.
int P_Random_(void);

// Fix randoms for demos.
void M_ClearRandom_(void);

extern bool     skyfadeback;

//extern  int leveltime2;
void R_DrawSky(int CM_Index);
void R_SkyTicker(void);
void R_SetupSky(void);
void R_ShowSky(bool enable);

#endif