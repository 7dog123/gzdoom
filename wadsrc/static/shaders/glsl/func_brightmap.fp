uniform sampler2D texture2;
uniform int uWrapS;//[GEC]
uniform int uWrapT;//[GEC]

vec4 Process(vec4 color)
{
	//TEXTURE_COORD
	float s = gl_TexCoord[0].s;
	float t = gl_TexCoord[0].t;
	
	//GL_TEXTURE_WRAP_S
	if(uWrapS != 0)
	{
		s = fract(s * 0.5) * 2.0;
		s = 1.0 - abs(s - 1.0);
	}
	
	//GL_TEXTURE_WRAP_T
	if(uWrapT != 0)
	{
		t = fract(t * 0.5) * 2.0;
		t = 1.0 - abs(t - 1.0);
	}
	vec2 v_texcoord = vec2(s, t);
	
	vec4 brightpix = desaturate(texture2D(texture2, v_texcoord));
	vec4 texel = getTexel(gl_TexCoord[0].st);
	return vec4(texel.rgb * min (color.rgb + brightpix.rgb, 1.0), texel.a*color.a);
}