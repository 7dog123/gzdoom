
// Changing this constant gives results very similar to changing r_visibility.
// Default is 232, it seems to give exactly the same light bands as software renderer.
#define DOOMLIGHTFACTOR 232.0


#ifdef DYNLIGHT

// ATI does not like this inside an #ifdef so it will be prepended by the compiling code inside the .EXE now.
//#version 120
//#extension GL_EXT_gpu_shader4 : enable

uniform ivec3 lightrange;
#ifndef MAXLIGHTS128
uniform vec4 lights[256];
#else
uniform vec4 lights[128];
#endif

#endif

uniform int uPalLightLevels;//[GEC]
uniform int uSoftLightPsx;//[GEC]
uniform int uSetLightMode;//[GEC]
uniform float uPsxBrightLevel;//[GEC]
uniform float uLight64;//[GEC]
uniform vec4 uBlendColor;
uniform int uBlendMode;//[GEC]
uniform int uFadeLinear;//[GEC]
uniform int uPsxColor;//[GEC]

uniform int fogenabled;
uniform vec4 fogcolor;
uniform vec3 dlightcolor;
uniform vec3 camerapos;
varying vec4 pixelpos;
varying vec4 fogparm;
//uniform vec2 lightparms;
uniform float desaturation_factor;

uniform vec4 topglowcolor;
uniform vec4 bottomglowcolor;
varying vec2 glowdist;

uniform int texturemode;
uniform sampler2D tex;

vec4 Process(vec4 color);

varying float lightlevel;

#ifdef SOFTLIGHT
// Doom lighting equation ripped from EDGE.
// Big thanks to EDGE developers for making the only port
// that actually replicates software renderer's lighting in OpenGL.
// Float version.
// Basically replace int with float and divide all constants by 31.
float R_DoomLightingEquation_OLD(float light, float dist)
{
	/* L in the range 0 to 63 */
	float L = light * 63.0/31.0;

	float min_L = clamp(36.0/31.0 - L, 0.0, 1.0);

	// Fix objects getting totally black when close.
	if (dist < 0.0001)
		dist = 0.0001;

	float scale = 1.0 / dist;
	float index = (59.0/31.0 - L) - (scale * DOOMLIGHTFACTOR/31.0 - DOOMLIGHTFACTOR/31.0);

	/* result is colormap index (0 bright .. 31 dark) */
	return clamp(index, min_L, 1.0);
}

float R_DoomLightingEquation(float light, float dist)
{
	// Calculated from r_visibility. It differs between walls, floor and sprites.
	//
	// Wall: globVis = r_WallVisibility
	// Floor: r_FloorVisibility / abs(plane.Zat0 - ViewPos.Z)
	// Sprite: same as wall
	// All are calculated in R_SetVisibility and seem to be decided by the
	// aspect ratio amongst other things.
	//
	// 1706 is the value for walls on 1080p 16:9 displays.
	float globVis = 1706.0;

	/* L is the integer light level used in the game */
	float L = light * 255.0;

	/* z is the depth in view/eye space, positive going into the screen */
	float z = pixelpos.w;

	/* The zdoom light equation */
	float vis = globVis / z;
	float shade = 64.0 - (L + 12.0) * 32.0/128.0;
	float lightscale;
	if (uPalLightLevels != 0)
		lightscale = clamp(float(int(shade - min(24.0, vis))) / 32.0, 0.0, 31.0/32.0);
	else
		lightscale = clamp((shade - min(24.0, vis)) / 32.0, 0.0, 31.0/32.0);

	// Result is the normalized colormap index (0 bright .. 1 dark)
	return lightscale;
}
#endif

vec4 PSXClut(vec4 texel)
{
	float r = floor((texel.r * 255.0) / 8.0);
	float g = floor((texel.g * 255.0) / 8.0) * 32.0;
	float b = floor((texel.b * 255.0) / 8.0) * 1024.0;

	texel.r = ((r * 8.0)) / 255.0;
    texel.g = ((g / 32.0) * 8.0) / 255.0;
    texel.b = ((b / 1024.0) * 8.0) / 255.0;
	
	return texel;
}

float R_DoomLightingEquationPSX(float light)
{
	float Lightscale;
	float Dist = (gl_FragCoord.z / gl_FragCoord.w);
	float Factor = 0.0;
	
	Factor = (400.0 - Dist)/(1000.0);
	if(uSetLightMode == 1)//Walls
		Factor = (400.0 - Dist)/(1000.0);
	else if(uSetLightMode == 2)//Floor
		Factor = (200.0 - (Dist))/(1040.0);
	else if(uSetLightMode == 3)//Sprite
		Factor = (36.0/255.0);
	
	Factor = clamp(Factor, 0.0, (50.0/255.0)) * 255.0;

	float L = (light * 255.0);
	L = clamp(L - 8.0, 0.0, 255.0)/2.0;
	
	float Shade = clamp((L * (32.0 + Factor) + 32.0 / 2.0) / 32.0, L, 255.0);
	
	if (uPalLightLevels == 1 && uPsxColor == 0)
		Lightscale = clamp((float(int(abs(Shade / 4.0))) * 4.0)/ 255.0, 0.0, 1.0);
	else
		Lightscale = clamp(Shade / 255.0, 0.0, 1.0);

	return Lightscale;
}

//===========================================================================
//
// Desaturate a color
//
//===========================================================================

vec4 desaturate(vec4 texel)
{
	#ifndef NO_DESATURATE
		float gray = (texel.r * 0.3 + texel.g * 0.56 + texel.b * 0.14);	
		return mix (vec4(gray,gray,gray,texel.a), texel, desaturation_factor);
	#else
		return texel;
	#endif
}

//===========================================================================
//
// Calculate light
//
//===========================================================================

vec4 getLightColor(float fogdist, float fogfactor)
{
	vec4 color = gl_Color;
	#ifdef SOFTLIGHT
		if(uSoftLightPsx == 1)//[GEC]
		{
			float newlightlevel = R_DoomLightingEquationPSX(lightlevel);//DOOM PSX
			if(uSetLightMode == 0)//None
			{
				newlightlevel = lightlevel;
			}
			color.rgb *= clamp(vec3(newlightlevel) + dlightcolor, 0.0, 1.0);
		}
		else
		{
			float newlightlevel = 1.0 - R_DoomLightingEquation(lightlevel, gl_FragCoord.z);
			color.rgb *= clamp(vec3(newlightlevel) + dlightcolor, 0.0, 1.0);
		}
	#endif
	#ifndef NO_FOG
	//
	// apply light diminishing	
	//
	if (fogenabled > 0)
	{
		#if (!defined(NO_SM4) || defined(DOOMLIGHT)) && !defined SOFTLIGHT
			// special lighting mode 'Doom' not available on older cards for performance reasons.
			if (fogdist < fogparm.y) 
			{
				color.rgb *= fogparm.x - (fogdist / fogparm.y) * (fogparm.x - 1.0);
			}
		#endif
		
		//color = vec4(color.rgb * (1.0 - fogfactor), color.a);
		color.rgb = mix(vec3(0.0, 0.0, 0.0), color.rgb, fogfactor);
	}
	#endif
	
	#ifndef NO_GLOW
	//
	// handle glowing walls
	//
	if (topglowcolor.a > 0.0 && glowdist.x < topglowcolor.a)
	{
		color.rgb += desaturate(topglowcolor * (1.0 - glowdist.x / topglowcolor.a)).rgb;
	}
	if (bottomglowcolor.a > 0.0 && glowdist.y < bottomglowcolor.a)
	{
		color.rgb += desaturate(bottomglowcolor * (1.0 - glowdist.y / bottomglowcolor.a)).rgb;
	}
	color = min(color, 1.0);
	#endif
	
	// calculation of actual light color is complete.
	return color;
}

//===========================================================================
//
// Gets a texel and performs common manipulations
//
//===========================================================================

vec4 getTexel(vec2 st)
{
	vec4 texel = texture2D(tex, st);
	
	#ifndef NO_TEXTUREMODE
	//
	// Apply texture modes
	//
	if (texturemode == 2) 
	{
		texel.a = 1.0;
	}
	else if (texturemode == 1) 
	{
		texel.rgb = vec3(1.0,1.0,1.0);
	}
	#endif
	
	//[GEC] Light 64
	texel.rgb = vec3(clamp(texel.r + uLight64, 0.0, 1.0),
					 clamp(texel.g + uLight64, 0.0, 1.0),
					 clamp(texel.b + uLight64, 0.0, 1.0));
	//[GEC] Blends
	if(uBlendMode == 0)
	{
		texel.rgb = vec3((uBlendColor.r * uBlendColor.a)+(texel.r * (1.0-uBlendColor.a)),
						(uBlendColor.g * uBlendColor.a)+(texel.g * (1.0-uBlendColor.a)),
						(uBlendColor.b * uBlendColor.a)+(texel.b * (1.0-uBlendColor.a)));
	}
	
	//[GEC] Blends ADD Doom64 Mode
	/*if(uBlendMode == 1)
	{
		texel.rgb = vec3((uBlendColor.r * uBlendColor.a) + (texel.r * 1.0),
					(uBlendColor.g * uBlendColor.a) + (texel.g * 1.0),
					(uBlendColor.b * uBlendColor.a) + (texel.b * 1.0));
	}*/

	return desaturate(texel);
}

//===========================================================================
//
// Applies colored fog
//
//===========================================================================

#ifndef NO_FOG
vec4 applyFog(vec4 frag, float fogfactor)
{
	return vec4(mix(fogcolor.rgb, frag.rgb, fogfactor), frag.a);
}
#endif


//===========================================================================
//
// Main shader routine
//
//===========================================================================

void main()
{
	float fogdist = 0.0;
	float fogfactor = 0.0;
	
	#ifdef DYNLIGHT
		vec4 dynlight = vec4(0.0,0.0,0.0,0.0);
		vec4 addlight = vec4(0.0,0.0,0.0,0.0);
	#endif

	#ifndef NO_FOG
	//
	// calculate fog factor
	//
	if (fogenabled != 0)
	{
		#ifndef NO_SM4
			if (fogenabled == 1 || fogenabled == -1) 
			{
				fogdist = pixelpos.w;
			}
			else 
			{
				fogdist = max(16.0, distance(pixelpos.xyz, camerapos));
			}
		#elif !defined(FOG_RADIAL)
			fogdist = pixelpos.w;
		#else
			fogdist = max(16.0, distance(pixelpos.xyz, camerapos));
		#endif
		
		if (uFadeLinear != 0)//[GEC]
		{
			fogdist *= 1.0;
			float position = (fogdist / 1000.0);

			if(position <= 0.0)
			{
				position = 0.00001;
			}

			float min = (5.0 / position) / 10.0;
			float max = (30.0 / position) / 10.0;
			
			fogfactor = ((max) - position) / ((max - min) * 1.2);
		}
		else
		{
			fogfactor = exp2 (fogparm.z * fogdist);
		}
	}
	#endif
	
	vec4 frag = getLightColor(fogdist, fogfactor);
	
	
	#ifdef DYNLIGHT
		for(int i=0; i<lightrange.x; i+=2)
		{
			vec4 lightpos = lights[i];
			vec4 lightcolor = lights[i+1];
			
			lightcolor.rgb *= max(lightpos.w - distance(pixelpos.xyz, lightpos.xyz),0.0) / lightpos.w;
			dynlight += lightcolor;
		}
		for(int i=lightrange.x; i<lightrange.y; i+=2)
		{
			vec4 lightpos = lights[i];
			vec4 lightcolor = lights[i+1];
			
			lightcolor.rgb *= max(lightpos.w - distance(pixelpos.xyz, lightpos.xyz),0.0) / lightpos.w;
			dynlight -= lightcolor;
		}
		for(int i=lightrange.y; i<lightrange.z; i+=2)
		{
			vec4 lightpos = lights[i];
			vec4 lightcolor = lights[i+1];
			
			lightcolor.rgb *= max(lightpos.w - distance(pixelpos.xyz, lightpos.xyz),0.0) / lightpos.w;
			addlight += lightcolor;
		}
		frag.rgb = clamp(frag.rgb + dynlight.rgb, 0.0, 1.4);
	#endif
		
	frag = Process(frag);

	#ifdef DYNLIGHT
		frag.rgb += addlight.rgb;
	#endif

	//[GEC] Blends ADD Doom64 Mode
	if(uBlendMode == 1 || uBlendMode == 2)//Weapon use 2
	{
		vec4 BlendColor = uBlendColor;

		#ifndef NO_FOG
		if(uBlendMode == 1)
		{
			BlendColor = applyFog(BlendColor, fogfactor);
			
			if(BlendColor.r == 0.0 && BlendColor.g == 0.0 && BlendColor.b == 0.0)
			{
				BlendColor = uBlendColor;
			}
		}
		#else
			BlendColor = uBlendColor;
		#endif
			

		frag.rgb = vec3((BlendColor.r * BlendColor.a) + (frag.r * 1.0),
					(BlendColor.g * BlendColor.a) + (frag.g * 1.0),
					(BlendColor.b * BlendColor.a) + (frag.b * 1.0));
	}
					
	if(uPsxBrightLevel != 0.0)//[GEC]
	{
		frag.r = clamp(frag.r * uPsxBrightLevel, 0.0, 1.0);
		frag.g = clamp(frag.g * uPsxBrightLevel, 0.0, 1.0);
		frag.b = clamp(frag.b * uPsxBrightLevel, 0.0, 1.0);
	}

	#ifndef NO_FOG
		if (fogenabled < 0) 
		{
			frag = applyFog(frag, fogfactor);
		}
	#endif
	
	if (uPsxColor == 1)//[GEC]
	{
		frag = PSXClut(frag);
	}
	
	gl_FragColor = frag;
}
