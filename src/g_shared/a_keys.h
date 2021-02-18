#ifndef A_KEYS_H
#define A_KEYS_H

#include "a_pickups.h"

class AKey : public AInventory
{
	DECLARE_CLASS (AKey, AInventory)
public:
	virtual bool HandlePickup (AInventory *item);

	BYTE KeyNumber;

protected:
	virtual bool ShouldStay ();
};

bool P_CheckKeys (AActor *owner, int keynum, bool remote);
void P_InitKeyMessages ();
void P_DeinitKeyMessages ();
int P_GetMapColorForLock (int lock);
int P_GetMapColorForKey (AInventory *key);

struct keyflash_t //[GEC]
{
	bool    tryopen;
    bool    active;
    bool    doDraw;
    int     delay;
    int     times;
	int     keynum;
};

//static keyflash_t flashCards[1];    /* INFO FOR FLASHING CARDS & SKULLS */
extern struct keyflash_t flashCards[255];    /* INFO FOR FLASHING CARDS & SKULLS */
void R_KeyTicker ();//[GEC]

#endif
