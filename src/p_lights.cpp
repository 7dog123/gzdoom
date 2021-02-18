// Emacs style mode select	 -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log:$
//
// DESCRIPTION:
//		Handle Sector base lighting effects.
//
//-----------------------------------------------------------------------------


#include "templates.h"
#include "m_random.h"

#include "doomdef.h"
#include "p_local.h"

#include "p_lnspec.h"

// State.
#include "r_state.h"
#include "statnums.h"
#include "farchive.h"

static FRandom pr_flicker ("Flicker");
static FRandom pr_lightflash ("LightFlash");
static FRandom pr_strobeflash ("StrobeFlash");
static FRandom pr_fireflicker ("FireFlicker");

static FRandom pr_lights ("Lights");//[GEC] D64

//-----------------------------------------------------------------------------
//
//
//
//-----------------------------------------------------------------------------

IMPLEMENT_CLASS (DLighting)

DLighting::DLighting ()
{
}

DLighting::DLighting (sector_t *sector)
	: DSectorEffect (sector)
{
	ChangeStatNum (STAT_LIGHT);
}

//-----------------------------------------------------------------------------
//
// FIRELIGHT FLICKER
//
//-----------------------------------------------------------------------------

IMPLEMENT_CLASS (DFireFlicker)

DFireFlicker::DFireFlicker ()
{
}

void DFireFlicker::Serialize (FArchive &arc)
{
	Super::Serialize (arc);
	arc << m_Count << m_MaxLight << m_MinLight << m_psxmode;
}


//-----------------------------------------------------------------------------
//
// T_FireFlicker
//
//-----------------------------------------------------------------------------

void DFireFlicker::Tick ()
{
	int amount;

	if (--m_Count == 0)
	{
		amount = (pr_fireflicker() & 3) << 4;

		// [RH] Shouldn't this be (m_MaxLight - amount < m_MinLight)?
		if (m_Sector->lightlevel - amount < m_MinLight)
			m_Sector->SetLightLevel(m_MinLight);
		else
			m_Sector->SetLightLevel(m_MaxLight - amount);

		if(m_psxmode)
			m_Count = 3;//[GEC]
		else
			m_Count = 4;
	}
}

//-----------------------------------------------------------------------------
//
// P_SpawnFireFlicker
//
//-----------------------------------------------------------------------------

DFireFlicker::DFireFlicker (sector_t *sector, bool psxmode)//[GEC]
	: DLighting (sector)
{
	m_MaxLight = sector->lightlevel;
	m_MinLight = sector_t::ClampLight(sector->FindMinSurroundingLight(sector->lightlevel) + 16);
	m_psxmode = psxmode;
	if(m_psxmode)
		m_Count = 3;
	else
		m_Count = 4;
}

DFireFlicker::DFireFlicker (sector_t *sector, int upper, int lower)
	: DLighting (sector)
{
	m_MaxLight = sector_t::ClampLight(upper);
	m_MinLight = sector_t::ClampLight(lower);
	m_Count = 4;
	m_psxmode = false;
}

//-----------------------------------------------------------------------------
//
// [RH] flickering light like Hexen's
//
//-----------------------------------------------------------------------------

IMPLEMENT_CLASS (DFlicker)

DFlicker::DFlicker ()
{
}

void DFlicker::Serialize (FArchive &arc)
{
	Super::Serialize (arc);
	arc << m_Count << m_MaxLight << m_MinLight;
}

//-----------------------------------------------------------------------------
//
//
//
//-----------------------------------------------------------------------------

void DFlicker::Tick ()
{
	if (m_Count)
	{
		m_Count--;	
	}
	else if (m_Sector->lightlevel == m_MaxLight)
	{
		m_Sector->SetLightLevel(m_MinLight);
		m_Count = (pr_flicker()&7)+1;
	}
	else
	{
		m_Sector->SetLightLevel(m_MaxLight);
		m_Count = (pr_flicker()&31)+1;
	}
}

//-----------------------------------------------------------------------------
//
//
//
//-----------------------------------------------------------------------------

DFlicker::DFlicker (sector_t *sector, int upper, int lower)
	: DLighting (sector)
{
	m_MaxLight = upper;
	m_MinLight = lower;
	sector->lightlevel = upper;
	m_Count = (pr_flicker()&64)+1;
}

//-----------------------------------------------------------------------------
//
//
//
//-----------------------------------------------------------------------------

void EV_StartLightFlickering (int tag, int upper, int lower)
{
	int secnum;
	FSectorTagIterator it(tag);
	while ((secnum = it.Next()) >= 0)
	{
		new DFlicker (&sectors[secnum], upper, lower);
	}
}


//-----------------------------------------------------------------------------
//
// BROKEN LIGHT FLASHING
//
//-----------------------------------------------------------------------------

IMPLEMENT_CLASS (DLightFlash)

DLightFlash::DLightFlash ()
{
}

void DLightFlash::Serialize (FArchive &arc)
{
	Super::Serialize (arc);
	arc << m_Count << m_MaxLight << m_MaxTime << m_MinLight << m_MinTime;
}

//-----------------------------------------------------------------------------
//
// T_LightFlash
// Do flashing lights.
//
//-----------------------------------------------------------------------------

void DLightFlash::Tick ()
{
	if (--m_Count == 0)
	{
		if (m_Sector->lightlevel == m_MaxLight)
		{
			m_Sector->SetLightLevel(m_MinLight);
			m_Count = (pr_lightflash() & m_MinTime) + 1;
		}
		else
		{
			m_Sector->SetLightLevel(m_MaxLight);
			m_Count = (pr_lightflash() & m_MaxTime) + 1;
		}
	}
}

//-----------------------------------------------------------------------------
//
// P_SpawnLightFlash
//
//-----------------------------------------------------------------------------

DLightFlash::DLightFlash (sector_t *sector)
	: DLighting (sector)
{
	// Find light levels like Doom.
	m_MaxLight = sector->lightlevel;
	m_MinLight = sector->FindMinSurroundingLight (sector->lightlevel);
	m_MaxTime = 64;
	m_MinTime = 7;
	m_Count = (pr_lightflash() & m_MaxTime) + 1;
}
	
DLightFlash::DLightFlash (sector_t *sector, int min, int max)
	: DLighting (sector)
{
	// Use specified light levels.
	m_MaxLight = sector_t::ClampLight(max);
	m_MinLight = sector_t::ClampLight(min);
	m_MaxTime = 64;
	m_MinTime = 7;
	m_Count = (pr_lightflash() & m_MaxTime) + 1;
}


//-----------------------------------------------------------------------------
//
// STROBE LIGHT FLASHING
//
//-----------------------------------------------------------------------------

IMPLEMENT_CLASS (DStrobe)

DStrobe::DStrobe ()
{
}

void DStrobe::Serialize (FArchive &arc)
{
	Super::Serialize (arc);
	arc << m_Count << m_MaxLight << m_MinLight << m_DarkTime << m_BrightTime;
}

//-----------------------------------------------------------------------------
//
// T_StrobeFlash
//
//-----------------------------------------------------------------------------

void DStrobe::Tick ()
{
	if (--m_Count == 0)
	{
		if (m_Sector->lightlevel == m_MinLight)
		{
			m_Sector->SetLightLevel(m_MaxLight);
			m_Count = m_BrightTime;
		}
		else
		{
			m_Sector->SetLightLevel(m_MinLight);
			m_Count = m_DarkTime;
		}
	}
}

//-----------------------------------------------------------------------------
//
// Hexen-style constructor
//
//-----------------------------------------------------------------------------

DStrobe::DStrobe (sector_t *sector, int upper, int lower, int utics, int ltics)
	: DLighting (sector)
{
	m_DarkTime = ltics;
	m_BrightTime = utics;
	m_MaxLight = sector_t::ClampLight(upper);
	m_MinLight = sector_t::ClampLight(lower);
	m_Count = 1;	// Hexen-style is always in sync
}

//-----------------------------------------------------------------------------
//
// Doom-style constructor
//
//-----------------------------------------------------------------------------

DStrobe::DStrobe (sector_t *sector, int utics, int ltics, bool inSync)
	: DLighting (sector)
{
	m_DarkTime = ltics;
	m_BrightTime = utics;

	m_MaxLight = sector->lightlevel;
	m_MinLight = sector->FindMinSurroundingLight (sector->lightlevel);

	if (m_MinLight == m_MaxLight)
		m_MinLight = 0;

	m_Count = inSync ? 1 : (pr_strobeflash() & 7) + 1;
}



//-----------------------------------------------------------------------------
//
// Start strobing lights (usually from a trigger)
// [RH] Made it more configurable.
//
//-----------------------------------------------------------------------------

void EV_StartLightStrobing (int tag, int upper, int lower, int utics, int ltics)
{
	int secnum;
	FSectorTagIterator it(tag);
	while ((secnum = it.Next()) >= 0)
	{
		sector_t *sec = &sectors[secnum];
		if (sec->lightingdata)
			continue;
		
		new DStrobe (sec, upper, lower, utics, ltics);
	}
}

void EV_StartLightStrobing (int tag, int utics, int ltics)
{
	int secnum;
	FSectorTagIterator it(tag);
	while ((secnum = it.Next()) >= 0)
	{
		sector_t *sec = &sectors[secnum];
		if (sec->lightingdata)
			continue;
		
		new DStrobe (sec, utics, ltics, false);
	}
}


//-----------------------------------------------------------------------------
//
// TURN LINE'S TAG LIGHTS OFF
// [RH] Takes a tag instead of a line
//
//-----------------------------------------------------------------------------

void EV_TurnTagLightsOff (int tag)
{
	int secnum;
	FSectorTagIterator it(tag);
	while ((secnum = it.Next()) >= 0)
	{
		sector_t *sector = sectors + secnum;
		int min = sector->lightlevel;

		for (int i = 0; i < sector->linecount; i++)
		{
			sector_t *tsec = getNextSector (sector->lines[i],sector);
			if (!tsec)
				continue;
			if (tsec->lightlevel < min)
				min = tsec->lightlevel;
		}
		sector->SetLightLevel(min);
	}
}


//-----------------------------------------------------------------------------
//
// TURN LINE'S TAG LIGHTS ON
// [RH] Takes a tag instead of a line
//
//-----------------------------------------------------------------------------

void EV_LightTurnOn (int tag, int bright)
{
	int secnum;
	FSectorTagIterator it(tag);
	while ((secnum = it.Next()) >= 0)
	{
		sector_t *sector = sectors + secnum;
		int tbright = bright; //jff 5/17/98 search for maximum PER sector

		// bright = -1 means to search ([RH] Not 0)
		// for highest light level
		// surrounding sector
		if (bright < 0)
		{
			int j;

			for (j = 0; j < sector->linecount; j++)
			{
				sector_t *temp = getNextSector (sector->lines[j], sector);

				if (!temp)
					continue;

				if (temp->lightlevel > tbright)
					tbright = temp->lightlevel;
			}
		}
		sector->SetLightLevel(tbright);

		//jff 5/17/98 unless compatibility optioned
		//then maximum near ANY tagged sector
		if (i_compatflags & COMPATF_LIGHT)
		{
			bright = tbright;
		}
	}
}

//-----------------------------------------------------------------------------
//
// killough 10/98
//
// EV_LightTurnOnPartway
//
// Turn sectors tagged to line lights on to specified or max neighbor level
//
// Passed the tag of sector(s) to light and a light level fraction between 0 and 1.
// Sets the light to min on 0, max on 1, and interpolates in-between.
// Used for doors with gradual lighting effects.
//
//-----------------------------------------------------------------------------

void EV_LightTurnOnPartway (int tag, fixed_t frac)
{
	frac = clamp<fixed_t> (frac, 0, FRACUNIT);

	// Search all sectors for ones with same tag as activating line
	int secnum;
	FSectorTagIterator it(tag);
	while ((secnum = it.Next()) >= 0)
	{
		sector_t *temp, *sector = &sectors[secnum];
		int j, bright = 0, min = sector->lightlevel;

		for (j = 0; j < sector->linecount; ++j)
		{
			if ((temp = getNextSector (sector->lines[j], sector)) != NULL)
			{
				if (temp->lightlevel > bright)
				{
					bright = temp->lightlevel;
				}
				if (temp->lightlevel < min)
				{
					min = temp->lightlevel;
				}
			}
		}
		sector->SetLightLevel(DMulScale16 (frac, bright, FRACUNIT-frac, min));
	}
}


//-----------------------------------------------------------------------------
//
// [RH] New function to adjust tagged sectors' light levels
//		by a relative amount. Light levels are clipped to
//		be within range for sector_t::lightlevel.
//
//-----------------------------------------------------------------------------

void EV_LightChange (int tag, int value)
{
	int secnum;
	FSectorTagIterator it(tag);
	while ((secnum = it.Next()) >= 0)
	{
		sectors[secnum].SetLightLevel(sectors[secnum].lightlevel + value);
	}
}

	
//-----------------------------------------------------------------------------
//
// Spawn glowing light
//
//-----------------------------------------------------------------------------

IMPLEMENT_CLASS (DGlow)

DGlow::DGlow ()
{
}

void DGlow::Serialize (FArchive &arc)
{
	Super::Serialize (arc);
	arc << m_Direction << m_MaxLight << m_MinLight;
}

//-----------------------------------------------------------------------------
//
//
//
//-----------------------------------------------------------------------------

void DGlow::Tick ()
{
	int newlight = m_Sector->lightlevel;

	switch (m_Direction)
	{
	case -1:
		// DOWN
		newlight -= GLOWSPEED;
		if (newlight <= m_MinLight)
		{
			newlight += GLOWSPEED;
			m_Direction = 1;
		}
		break;
		
	case 1:
		// UP
		newlight += GLOWSPEED;
		if (newlight >= m_MaxLight)
		{
			newlight -= GLOWSPEED;
			m_Direction = -1;
		}
		break;
	}
	m_Sector->SetLightLevel(newlight);
}

//-----------------------------------------------------------------------------
//
//
//
//-----------------------------------------------------------------------------

DGlow::DGlow (sector_t *sector)
	: DLighting (sector)
{
	m_MinLight = sector->FindMinSurroundingLight (sector->lightlevel);
	m_MaxLight = sector->lightlevel;
	m_Direction = -1;
}

//-----------------------------------------------------------------------------
//
// [RH] More glowing light, this time appropriate for Hexen-ish uses.
//
//-----------------------------------------------------------------------------

IMPLEMENT_CLASS (DGlow2)

DGlow2::DGlow2 ()
{
}

void DGlow2::Serialize (FArchive &arc)
{
	Super::Serialize (arc);
	arc << m_End << m_MaxTics << m_OneShot << m_Start << m_Tics;
}

//-----------------------------------------------------------------------------
//
//
//
//-----------------------------------------------------------------------------

void DGlow2::Tick ()
{
	if (m_Tics++ >= m_MaxTics)
	{
		if (m_OneShot)
		{
			m_Sector->SetLightLevel(m_End);
			Destroy ();
			return;
		}
		else
		{
			int temp = m_Start;
			m_Start = m_End;
			m_End = temp;
			m_Tics -= m_MaxTics;
		}
	}

	m_Sector->SetLightLevel(((m_End - m_Start) * m_Tics) / m_MaxTics + m_Start);
}

//-----------------------------------------------------------------------------
//
//
//
//-----------------------------------------------------------------------------

DGlow2::DGlow2 (sector_t *sector, int start, int end, int tics, bool oneshot)
	: DLighting (sector)
{
	m_Start = sector_t::ClampLight(start);
	m_End = sector_t::ClampLight(end);
	m_MaxTics = tics;
	m_Tics = -1;
	m_OneShot = oneshot;
}

//-----------------------------------------------------------------------------
//
//
//
//-----------------------------------------------------------------------------

void EV_StartLightGlowing (int tag, int upper, int lower, int tics)
{
	int secnum;

	// If tics is non-positive, then we can't really do anything.
	if (tics <= 0)
	{
		return;
	}

	if (upper < lower)
	{
		int temp = upper;
		upper = lower;
		lower = temp;
	}

	FSectorTagIterator it(tag);
	while ((secnum = it.Next()) >= 0)
	{
		sector_t *sec = &sectors[secnum];
		if (sec->lightingdata)
			continue;
		
		new DGlow2 (sec, upper, lower, tics, false);
	}
}

//-----------------------------------------------------------------------------
//
//
//
//-----------------------------------------------------------------------------

void EV_StartLightFading (int tag, int value, int tics)
{
	int secnum;
	FSectorTagIterator it(tag);
	while ((secnum = it.Next()) >= 0)
	{
		sector_t *sec = &sectors[secnum];
		if (sec->lightingdata)
			continue;

		if (tics <= 0)
		{
			sec->SetLightLevel(value);
		}
		else
		{
			// No need to fade if lightlevel is already at desired value.
			if (sec->lightlevel == value)
				continue;

			new DGlow2 (sec, sec->lightlevel, value, tics, true);
		}
	}
}


//-----------------------------------------------------------------------------
//
// [RH] Phased lighting ala Hexen, but implemented without the help of the Hexen source
// The effect is a little different, but close enough, I feel.
//
//-----------------------------------------------------------------------------

IMPLEMENT_CLASS (DPhased)

DPhased::DPhased ()
{
}

void DPhased::Serialize (FArchive &arc)
{
	Super::Serialize (arc);
	arc << m_BaseLevel << m_Phase;
}

//-----------------------------------------------------------------------------
//
//
//
//-----------------------------------------------------------------------------

void DPhased::Tick ()
{
	const int steps = 12;

	if (m_Phase < steps)
		m_Sector->SetLightLevel( ((255 - m_BaseLevel) * m_Phase) / steps + m_BaseLevel);
	else if (m_Phase < 2*steps)
		m_Sector->SetLightLevel( ((255 - m_BaseLevel) * (2*steps - m_Phase - 1) / steps
								+ m_BaseLevel));
	else
		m_Sector->SetLightLevel(m_BaseLevel);

	if (m_Phase == 0)
		m_Phase = 63;
	else
		m_Phase--;
}

//-----------------------------------------------------------------------------
//
//
//
//-----------------------------------------------------------------------------

int DPhased::PhaseHelper (sector_t *sector, int index, int light, sector_t *prev)
{
	if (!sector)
	{
		return index;
	}
	else
	{
		DPhased *l;
		int baselevel = sector->lightlevel ? sector->lightlevel : light;

		if (index == 0)
		{
			l = this;
			m_BaseLevel = baselevel;
		}
		else
			l = new DPhased (sector, baselevel);

		int numsteps = PhaseHelper (sector->NextSpecialSector (
				sector->special == LightSequenceSpecial1 ?
					LightSequenceSpecial2 : LightSequenceSpecial1, prev),
				index + 1, l->m_BaseLevel, sector);
		l->m_Phase = ((numsteps - index - 1) * 64) / numsteps;

		sector->special = 0;

		return numsteps;
	}
}

//-----------------------------------------------------------------------------
//
//
//
//-----------------------------------------------------------------------------

DPhased::DPhased (sector_t *sector, int baselevel)
	: DLighting (sector)
{
	m_BaseLevel = baselevel;
}

DPhased::DPhased (sector_t *sector)
	: DLighting (sector)
{
	PhaseHelper (sector, 0, 0, NULL);
}

DPhased::DPhased (sector_t *sector, int baselevel, int phase)
	: DLighting (sector)
{
	m_BaseLevel = baselevel;
	m_Phase = phase;
}

//============================================================================
//
// EV_StopLightEffect
//
// Stops a lighting effect that is currently running in a sector.
//
//============================================================================

void EV_StopLightEffect (int tag)
{
	TThinkerIterator<DLighting> iterator;
	DLighting *effect;

	while ((effect = iterator.Next()) != NULL)
	{
		if (tagManager.SectorHasTag(effect->GetSector(), tag))
		{
			effect->Destroy();
		}
	}
}


// [GEC] New LIGHTS D64 Psx

//-----------------------------------------------------------------------------
//
// [GEC] PSX Glow Mode
//
//-----------------------------------------------------------------------------

IMPLEMENT_CLASS (DPsxGlow)

DPsxGlow::DPsxGlow ()
{
}

void DPsxGlow::Serialize (FArchive &arc)
{
	Super::Serialize (arc);
	arc << m_Direction << m_MaxLight << m_MinLight;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

#define PSXGLOWSPEED	(3 * FRACUNIT) / 2 // Originalmente es "3" en psxdoom, ya que trabaja con "15 TICRATE"

void DPsxGlow::Tick ()
{
	int newlight = m_Sector->lightlevel * FRACUNIT;

	switch (m_Direction)
	{
		case -1:
			// DOWN
			newlight -= PSXGLOWSPEED;
			m_Sector->lightlevel = (newlight / FRACUNIT);
			if (m_Sector->lightlevel < m_MinLight)
			{
				m_Sector->lightlevel = m_MinLight;
				m_Direction = 1;
			}
			break;
		case 1:
			// UP
			newlight += PSXGLOWSPEED;
			m_Sector->lightlevel = (newlight / FRACUNIT);
			if(m_MaxLight < m_Sector->lightlevel)
			{
				m_Sector->lightlevel = m_MaxLight;
				m_Direction = -1;
			}
			break;
	}
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

DPsxGlow::DPsxGlow (sector_t *sector, int type)
	: DLighting (sector)
{
	switch(type)
	{
		case 0:	//special == 8
			m_MinLight = sector->FindMinSurroundingLight(sector->lightlevel);
			m_MaxLight = sector->lightlevel;
			m_Direction = -1;
			break;
		case 1:	//special == 200
			m_MinLight = 10;
			m_MaxLight = sector->lightlevel;
			m_Direction = -1;
			break;
		case 2:	//special == 201
			m_MinLight = sector->lightlevel;
			m_MaxLight = 255;
			m_Direction = 1;
			break;
	}
}

//-----------------------------------------------------------------------------
//
// [GEC] D64 FireFlicker
//
//-----------------------------------------------------------------------------

IMPLEMENT_CLASS (DFireFlicker64)

DFireFlicker64::DFireFlicker64 ()
{
}

void DFireFlicker64::Serialize (FArchive &arc)
{
	Super::Serialize (arc);
	arc << m_Count << m_Special;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void DFireFlicker64::Tick ()
{
	int    amount;

    if(--m_Count) 
	{
        return;
    }

    if(m_Special != m_Sector->special) 
	{
        m_Sector->lightlevel_64 = 0;
        Destroy ();
        return;
    }

    amount = (pr_lights() & 31);
    m_Sector->lightlevel_64 = amount;
    m_Count = 3;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

DFireFlicker64::DFireFlicker64 (sector_t *sector)
	: DLighting (sector)
{
	m_Special = sector->special;
	m_Count = 3;
}

//-----------------------------------------------------------------------------
//
// [GEC] D64 Light Flash
//
//-----------------------------------------------------------------------------

IMPLEMENT_CLASS (DLightFlash64)

DLightFlash64::DLightFlash64 ()
{
}

void DLightFlash64::Serialize (FArchive &arc)
{
	Super::Serialize (arc);
	arc << m_Count << m_Special;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void DLightFlash64::Tick ()
{
	if(--m_Count) 
	{
        return;
    }

	if(m_Special != m_Sector->special) 
	{
        m_Sector->lightlevel_64 = 0;
        Destroy ();
        return;
    }

	if(m_Sector->lightlevel_64 == 32) 
	{
        m_Sector->lightlevel_64 = 0;
        m_Count = (pr_lights() & 7) + 1;
    }
    else 
	{
        m_Sector->lightlevel_64 = 32;
        m_Count = (pr_lights() & 32) + 1;
    }
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

DLightFlash64::DLightFlash64 (sector_t *sector)
	: DLighting (sector)
{
	m_Special = sector->special;
	m_Count = (pr_lights() & 63) + 1;
}

//-----------------------------------------------------------------------------
//
// [GEC] D64 Strobe
//
//-----------------------------------------------------------------------------

IMPLEMENT_CLASS (DStrobe64)

DStrobe64::DStrobe64 ()
{
}

void DStrobe64::Serialize (FArchive &arc)
{
	Super::Serialize (arc);
	arc << m_Count << m_Maxlight << m_Darktime << m_Brighttime << m_Special;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void DStrobe64::Tick ()
{
	if(--m_Count) 
	{
        return;
    }

	if(m_Special != m_Sector->special) 
	{
        m_Sector->lightlevel_64 = 0;
        Destroy ();
        return;
    }

    if(m_Sector->lightlevel_64 != 0) 
	{
        m_Sector->lightlevel_64 = 0;
        m_Count = m_Darktime;
    }
    else 
	{
        m_Sector->lightlevel_64 = m_Maxlight;
        m_Count = m_Brighttime;
    }
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

DStrobe64::DStrobe64 (sector_t *sector, int speed, bool alternate)
	: DLighting (sector)
{
	if(!alternate)
	{
		m_Darktime = speed;
		m_Special = sector->special;
		m_Maxlight = 16;
		m_Brighttime = 3;
		m_Count = (pr_lights() & 7) + 1;
	}
	else
	{
		m_Darktime = speed;
		m_Special = sector->special;
		m_Maxlight = 127;
		m_Brighttime = 1;
		m_Count = 1;
	}
}

//-----------------------------------------------------------------------------
//
// [GEC] D64 Glow
//
//-----------------------------------------------------------------------------

IMPLEMENT_CLASS (DGlow64)

DGlow64::DGlow64 ()
{
}

void DGlow64::Serialize (FArchive &arc)
{
	Super::Serialize (arc);
	arc << m_Type <<  m_Count << m_Minlight << m_Direction << m_Maxlight  << m_Special;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void DGlow64::Tick ()
{
	if(--m_Count) 
	{
        return;
    }

    if(m_Special != m_Sector->special) 
	{
        m_Sector->lightlevel_64 = 0;
        Destroy ();
        return;
    }

    m_Count = 2;

    if(m_Direction == -1) 
	{
		m_Sector->lightlevel_64 -= 2;
        if(!(m_Sector->lightlevel_64 < m_Minlight))
		{
            return;
        }

        m_Sector->lightlevel_64 = m_Minlight;

        if(m_Type == D64PULSERANDOM)
		{
            m_Maxlight = (pr_lights() & 31) + 17;
        }

        m_Direction = 1;

        return;
    }
    else if(m_Direction == 1) 
	{
        m_Sector->lightlevel_64 += 2;
        if(!(m_Maxlight < m_Sector->lightlevel_64))
		{
            return;
        }

        if(m_Type == D64PULSERANDOM)
		{
            m_Minlight = (pr_lights() & 15);
        }

        m_Direction = -1;
    }
    else
	{
        return;
    }
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

DGlow64::DGlow64 (sector_t *sector, int type)
	: DLighting (sector)
{
	m_Count = 2;
	m_Direction = 1;
    m_Type = type;
    m_Special = sector->special;
    m_Minlight = 0;

    if(m_Type == D64PULSENORMAL) 
	{
        m_Maxlight = 32;
    }
    else if(m_Type == D64PULSERANDOM || m_Type == D64PULSESLOW) 
	{
        m_Maxlight = 48;
    }
}


//-----------------------------------------------------------------------------
//
// [GEC] D64 Sequence Glow
//
//-----------------------------------------------------------------------------

IMPLEMENT_CLASS (DSequenceGlow64)

DSequenceGlow64::DSequenceGlow64 ()
{
}

void DSequenceGlow64::Serialize (FArchive &arc)
{
	Super::Serialize (arc);
	arc << m_Headsector << m_Count << m_Start << m_Index << m_Special;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

#define SEQUENCELIGHTMAX    48

void DSequenceGlow64::Tick ()
{
	if(--m_Count) 
	{
        return;
    }

    if(m_Special != m_Sector->special) 
	{
        m_Sector->lightlevel_64 = 0;
        Destroy ();
        return;
    }

    m_Count = 1;

    if(m_Start == -1)
	{
        m_Sector->lightlevel_64 -= 2;
        if(m_Sector->lightlevel_64 > 0)
		{
            return;
        }

        m_Sector->lightlevel_64 = 0;

        if(m_Headsector == NULL)
		{
            m_Sector->special = 0;
			m_Sector->lightlevel_64 = 0;
			Destroy ();
            return;
        }

        m_Start = 0;
    }
    else if(m_Start == 0)
	{
        if(!m_Headsector->lightlevel_64)
		{
            return;
        }

        m_Start = 1;
    }
    else if(m_Start == 1)
	{
        m_Sector->lightlevel_64 += 2;
        if(m_Sector->lightlevel_64 < (SEQUENCELIGHTMAX + 1))
		{
            sector_t *next = NULL;
            int i = 0;

            if(m_Sector->lightlevel_64 != 8)
			{
                return;
            }

            if(m_Sector->linecount <= 0)
			{
                return;
            }

            for(i = 0; i < m_Sector->linecount; i++)
			{
                next = m_Sector->lines[i]->backsector;

                if(!next)
				{
                    continue;
                }
                if(next->special)
				{
                    continue;
                }
                //if(next->tag != (m_Sector->tag + 1))
				if(tagManager.GetFirstSectorTag(next) != (tagManager.GetFirstSectorTag(m_Sector) + 1))
				{
                    continue;
                }

                next->special = m_Sector->special;
				new DSequenceGlow64 (next, false);
                //P_SpawnSequenceLight(next, false);
            }
        }
        else
		{
            m_Sector->lightlevel_64 = SEQUENCELIGHTMAX;
            m_Start = -1;
        }
    }
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

DSequenceGlow64::DSequenceGlow64 (sector_t *sector, bool first)
	: DLighting (sector)
{
	sector_t *headsector = NULL;

	int i = 0;

    if(!sector->linecount) 
	{
        return;
    }

	if(first) 
	{
        for(i = 0; i < sector->linecount; i++)
		{
            headsector = sector->lines[i]->frontsector;
            if(!headsector)// this should never happen
			{  
                return;
            }

            if(headsector == sector) 
			{
                continue;
            }

            //if(headsector->tag == sector->tag) 
			if(tagManager.GetFirstSectorTag(headsector) == tagManager.GetFirstSectorTag(sector))
			{
                break;
            }
        }
    }

    m_Special = sector->special;
    m_Count = 1;
    m_Index = tagManager.GetFirstSectorTag(sector);
    m_Start = (headsector == NULL ? 1 : 0);
    m_Headsector = headsector;
}

//-----------------------------------------------------------------------------
//
// [GEC] D64 Combine
//
//-----------------------------------------------------------------------------

IMPLEMENT_CLASS (DCombine64)

DCombine64::DCombine64 ()
{
}

void DCombine64::Serialize (FArchive &arc)
{
	Super::Serialize (arc);
	arc << m_Combine;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void DCombine64::Tick ()
{
	if(m_Combine == NULL)
	{
		m_Sector->lightlevel_64 = 0;
        Destroy ();
        return;
	}
	if(m_Combine->special != m_Sector->special)
	{
		m_Sector->lightlevel_64 = 0;
        Destroy ();
        return;
    }

	m_Sector->lightlevel_64 = m_Combine->lightlevel_64;
	return;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

DCombine64::DCombine64 (sector_t *sector, sector_t *combine)
	: DLighting (sector)
{
	if(combine == NULL)
	{
		m_Sector->lightlevel_64 = 0;
        Destroy ();
        return;
	}

	m_Combine = combine;
	m_Combine->special = combine->special;
	m_Sector->special = combine->special;
}


#include "r_data/colormaps.h"//[GEC]

#define MAXBRIGHTNESS        100
CUSTOM_CVAR (Float, gl_d64brightness, 0.0f, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)
{
	if (self < 0.f)
	{
		self = 0.f;//1.f
	}
	else if (self > (int)MAXBRIGHTNESS)
	{
		self = (float)MAXBRIGHTNESS;
	}

	R_RefreshBrightness();
}

//-----------------------------------------------------------------------------
//
// [GEC] D64 Light Morph
//
//-----------------------------------------------------------------------------

IMPLEMENT_CLASS (DLightMorph)

DLightMorph::DLightMorph ()
{
}

void DLightMorph::Serialize (FArchive &arc)
{
	Super::Serialize (arc);
	arc << m_Dest << m_Src << m_RGB << m_NewRGB << m_Inc << m_color;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void DLightMorph::Tick ()
{
    float brightness = gl_d64brightness;
	float factor = (float)lightfactor;

	if(infraredFactor != 0)
		factor = (((infraredFactor > brightness) ? infraredFactor : brightness) + 100.0f);

	/*if((level.FadeInBrightness && lightfactor == 200) || //[GEC] Espera a que termine la funcion R_FadeInBrightness();
		(!level.FadeInBrightness && lightfactor == 0))*/
	{
		m_Inc += 4;

		if(m_Inc > 256)
		{
			if (m_color == 0){ m_Sector->SpecialColors[0] = m_Sector->SpecialColorsBase[0] = m_Dest;}
			else if (m_color == 1) {m_Sector->SpecialColors[1] = m_Sector->SpecialColorsBase[1] = m_Dest;}
			else if (m_color == 2) {m_Sector->SpecialColors[2] = m_Sector->SpecialColorsBase[2] = m_Dest;}
			else if (m_color == 3) {m_Sector->SpecialColors[3] = m_Sector->SpecialColorsBase[3] = m_Dest;}
			else if (m_color == 4) {m_Sector->SpecialColors[4] = m_Sector->SpecialColorsBase[4] = m_Dest;}
			//else if (m_color == 5) {m_Sector->SpecialColors[5] = m_Sector->SpecialColorsBase[5] = m_Dest;}
			else if (m_color == -1) {m_Sector->ColorMap->Color = m_Dest;}

			
			R_SetLightFactorSector(factor, m_Sector);//[GEC] Refresh Brightness

			Destroy ();
			return;
		}

		m_NewRGB.r = (m_RGB.r + ((m_Inc * (m_Src.r - m_RGB.r)) >> 8));
		m_NewRGB.g = (m_RGB.g + ((m_Inc * (m_Src.g - m_RGB.g)) >> 8));
		m_NewRGB.b = (m_RGB.b + ((m_Inc * (m_Src.b - m_RGB.b)) >> 8));

		if (m_color == 0){ m_Sector->SpecialColors[0] = m_Sector->SpecialColorsBase[0] = m_NewRGB;}
		else if (m_color == 1){ m_Sector->SpecialColors[1] = m_Sector->SpecialColorsBase[1] = m_NewRGB;}
		else if (m_color == 2){ m_Sector->SpecialColors[2] = m_Sector->SpecialColorsBase[2] = m_NewRGB;}
		else if (m_color == 3){ m_Sector->SpecialColors[3] = m_Sector->SpecialColorsBase[3] = m_NewRGB;}
		else if (m_color == 4){ m_Sector->SpecialColors[4] = m_Sector->SpecialColorsBase[4] = m_NewRGB;}
		//else if (m_color == 5){ m_Sector->SpecialColors[5] = m_Sector->SpecialColorsBase[5] = m_NewRGB;}
		else if (m_color == -1){ m_Sector->ColorMap->Color = m_NewRGB;}

		R_SetLightFactorSector(factor, m_Sector);//[GEC] Refresh Brightness

		return;
	}

	return;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

DLightMorph::DLightMorph (sector_t *sector, PalEntry destcolor, PalEntry srccolor, PalEntry destcolorbase, int color)
	: DLighting (sector)
{
	if(destcolor)
	{
		destcolor.r = srccolor.r;
        destcolor.g = srccolor.g;
        destcolor.b = srccolor.b;
    }

	m_color = color;
	m_NewRGB = 0;
    m_Inc = 0;
    m_Src = srccolor; // the light to morph to
    m_Dest = destcolor == NULL ? srccolor : destcolor; // the light to morph from
	m_RGB.r = destcolor == NULL ? 0 : destcolorbase.r;
    m_RGB.g = destcolor == NULL ? 0 : destcolorbase.g;
    m_RGB.b = destcolor == NULL ? 0 : destcolorbase.b;
}

//------------------------------------------------------------------------
//
// FADE IN BRIGHTNESS EFFECT
//
//------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// dfcmp
//-----------------------------------------------------------------------------

bool dfcmp(float f1, float f2) {
    float precision = 0.00001f;
    if(((f1 - precision) < f2) &&
            ((f1 + precision) > f2)) {
        return true;
    }
    else {
        return false;
    }
}
//
// R_LightGetHSV
// Set HSV values based on given RGB
//

void R_LightGetHSV(int r, int g, int b, int *h, int *s, int *v) {
    int min = r;
    int max = r;
    float delta = 0.0f;
    float j = 0.0f;
    float x = 0.0f;
    float xr = 0.0f;
    float xg = 0.0f;
    float xb = 0.0f;
    float sum = 0.0f;

    if(g < min) {
        min = g;
    }
    if(b < min) {
        min = b;
    }

    if(g > max) {
        max = g;
    }
    if(b > max) {
        max = b;
    }

    delta = ((float)max / 255.0f);

    if(dfcmp(delta, 0.0f)) {
        delta = 0;
    }
    else {
        j = ((delta - ((float)min / 255.0f)) / delta);
    }

    if(!dfcmp(j, 0.0f)) {
        xr = ((float)r / 255.0f);

        if(!dfcmp(xr, delta)) {
            xg = ((float)g / 255.0f);

            if(!dfcmp(xg, delta)) {
                xb = ((float)b / 255.0f);

                if(dfcmp(xb, delta)) {
                    sum = ((((delta - xg) / (delta - (min / 255.0f))) + 4.0f) -
                           ((delta - xr) / (delta - (min / 255.0f))));
                }
            }
            else {
                sum = ((((delta - xr) / (delta - (min / 255.0f))) + 2.0f) -
                       ((delta - (b / 255.0f)) /(delta - (min / 255.0f))));
            }
        }
        else {
            sum = (((delta - (b / 255.0f))) / (delta - (min / 255.0f))) -
                  ((delta - (g / 255.0f)) / (delta - (min / 255.0f)));
        }

        x = (sum * 60.0f);

        if(x < 0) {
            x += 360.0f;
        }
    }
    else {
        j = 0.0f;
    }

    *h = (int)((x / 360.0f) * 255.0f);
    *s = (int)(j * 255.0f);
    *v = (int)(delta * 255.0f);
}

//
// R_LightGetRGB
// Set RGB values based on given HSV
//

void R_LightGetRGB(int h, int s, int v, int *r, int *g, int *b) {
    float x = 0.0f;
    float j = 0.0f;
    float i = 0.0f;
    int table = 0;
    float xr = 0.0f;
    float xg = 0.0f;
    float xb = 0.0f;

    j = (h / 255.0f) * 360.0f;

    if(360.0f <= j) {
        j -= 360.0f;
    }

    x = (s / 255.0f);
    i = (v / 255.0f);

    if(!dfcmp(x, 0.0f)) {
        table = (int)(j / 60.0f);
        if(table < 6) {
            float t = (j / 60.0f);
            switch(table) {
            case 0:
                xr = i;
                xg = ((1.0f - ((1.0f - (t - (float)table)) * x)) * i);
                xb = ((1.0f - x) * i);
                break;
            case 1:
                xr = ((1.0f - (x * (t - (float)table))) * i);
                xg = i;
                xb = ((1.0f - x) * i);
                break;
            case 2:
                xr = ((1.0f - x) * i);
                xg = i;
                xb = ((1.0f - ((1.0f - (t - (float)table)) * x)) * i);
                break;
            case 3:
                xr = ((1.0f - x) * i);
                xg = ((1.0f - (x * (t - (float)table))) * i);
                xb = i;
                break;
            case 4:
                xr = ((1.0f - ((1.0f - (t - (float)table)) * x)) * i);
                xg = ((1.0f - x) * i);
                xb = i;
                break;
            case 5:
                xr = i;
                xg = ((1.0f - x) * i);
                xb = ((1.0f - (x * (t - (float)table))) * i);
                break;
            }
        }
    }
    else {
        xr = xg = xb = i;
    }

    *r = (int)(xr * 255.0f);
    *g = (int)(xg * 255.0f);
    *b = (int)(xb * 255.0f);
}

float Factor = 0.0;


//
// [GEC] R_SetLightFactorSector
//

void R_SetLightFactorSector(float lightfactor, sector_t *sec)
{
	Factor = lightfactor;

	float f = lightfactor / 100.0f;
	int h, s, v;
	int R, G, B;
	PalEntry Color;
	for(int j = 0; j < 5; j++)
	{
		Color = sec->SpecialColorsBase[j];

		R_LightGetHSV(Color.r, Color.g, Color.b, &h, &s, &v);

		v = MIN((int)((float)v * f), 255);
		
		R_LightGetRGB(h, s, v, (int*)&R, (int*)&G, (int*)&B);

		Color = PalEntry(BYTE(R), BYTE(G), BYTE(B));

		sec->SpecialColors[j] = Color;
	}
}

//
// [GEC] R_SetLightFactor
//

void R_SetLightFactor(float lightfactor)
{
	Factor = lightfactor;

	sector_t *sec;

	for(int i = 0; i < numsectors; i++)
	{
		sec = &sectors[i];

		float f = lightfactor / 100.0f;
		int h, s, v;
		int R, G, B;
		PalEntry Color;
		for(int j = 0; j < 5; j++)
		{
			Color = sec->SpecialColorsBase[j];

			R_LightGetHSV(Color.r, Color.g, Color.b, &h, &s, &v);

			v = MIN((int)((float)v * f), 255);
			
			R_LightGetRGB(h, s, v, (int*)&R, (int*)&G, (int*)&B);

			Color = PalEntry(BYTE(R), BYTE(G), BYTE(B));

			sec->SpecialColors[j] = Color;
		}
    }
}

//
// [GEC] R_RefreshBrightness
//
//int infraredFactor;

void R_RefreshBrightness()
{
    float factor;
    float brightness = gl_d64brightness;
	//float infraredFactor = 0;

    factor = (((infraredFactor > brightness) ? infraredFactor : brightness) + 100.0f);

	R_SetLightFactor(factor);
}

void R_FadeInBrightness()
{
	if(lightfactor < (gl_d64brightness + 100))
	{
		//Printf("R_SetLightFactor %d\n", lightfactor);
        R_SetLightFactor((float)lightfactor);
    }
    else
	{
		lightfactor = (int)(gl_d64brightness + 100);
    }
}



IMPLEMENT_CLASS (DFadeInBrightness)

DFadeInBrightness::DFadeInBrightness ()
{
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void DFadeInBrightness::Destroy()
{
	Super::Destroy();
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void DFadeInBrightness::Tick ()
{
	m_factor += 2;

	if(m_factor < (gl_d64brightness + 100))
	{
        R_SetLightFactor(m_factor);
		//Printf("R_SetLightFactor\n");
    }
    else
	{
		m_factor = (gl_d64brightness + 100);
		Destroy();
    }

	if(m_factor >= (gl_d64brightness + 100)){Destroy();}//Printf("Destroy\n");}
	return;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

DFadeInBrightness::DFadeInBrightness (float factor)
{
	m_factor = factor;
}