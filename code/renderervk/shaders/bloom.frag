#version 450

layout(set = 0, binding = 0) uniform sampler2D texture0;

layout(location = 0) in vec2 frag_tex_coord;

layout(location = 0) out vec4 out_color;

//layout(constant_id = 0) const float gamma = 1.0;
//layout(constant_id = 1) const float obScale = 2.0;
//layout(constant_id = 2) const float greyscale = 0.0;
layout(constant_id = 3) const float threshold = 0.6;
//layout(constant_id = 4) const float factor = 0.5;
layout(constant_id = 5) const int extract_mode = 0;
layout(constant_id = 6) const int base_modulate = 0;

//const vec3 sRGB = { 0.2126, 0.7152, 0.0722 };

#ifdef Q360_XENOS
// Q360: the extract modes as weights rather than nested if/else, which xost doesn't merge into the output.
void main() {
	vec3 base = texture(texture0, frag_tex_coord).rgb;
	const vec3 luma = { 0.2126, 0.7152, 0.0722 };
	float v = dot( luma, base.rgb );
	float mode1 = float( extract_mode == 1 );
	float mode2 = float( extract_mode == 2 );
	float level = mode1 * ( (base.r + base.g + base.b) * 0.33333333 ) + mode2 * v
		+ (1.0 - mode1 - mode2) * max( max( base.r, base.g ), base.b );
	vec3 modulated = mix( base, mix( base * base, base * v, float( base_modulate != 1 ) ), float( base_modulate != 0 ) );
	out_color = mix( vec4( 0.0 ), vec4( modulated, 1.0 ), float( level >= threshold ) );
}
#else
void main() {
	vec3 base = texture(texture0, frag_tex_coord).rgb;

	if ( extract_mode == 1 ) // (r+g+b)/3 >= threshold
	{
		if ( (base.r + base.g + base.b) * 0.33333333 >= threshold)
		{
			if ( base_modulate != 0 )
			{
				if ( base_modulate == 1 )
				{
					base *= base;
				}
				else
				{
					const vec3 luma = { 0.2126, 0.7152, 0.0722 };
					base *= dot( luma, base.rgb );
				}
			}
			out_color = vec4( base, 1.0 );
		}
		else
		{
			out_color = vec4( 0.0 );
		}
	}
	else if ( extract_mode == 2 ) // luma(r,g,b) >= threshold
	{
		const vec3 luma = { 0.2126, 0.7152, 0.0722 };
		const float v = dot( luma, base.rgb );
		if ( v >= threshold )
		{
			if ( base_modulate != 0 )
			{
				if ( base_modulate == 1 )
				{
					base *= base;
				}
				else
				{
					base *= v;
				}
			}
			out_color = vec4( base, 1.0 );
		}
		else
		{
			out_color = vec4( 0.0 );
		}
	}
	else // default case
	{
		if ( base.r >= threshold || base.g >= threshold || base.b >= threshold ) // (r|g|b) >= threshold
		{
			if ( base_modulate != 0 )
			{
				if ( base_modulate == 1 )
				{
					base *= base;
				}
				else
				{
					const vec3 luma = { 0.2126, 0.7152, 0.0722 };
					base *= dot( luma, base.rgb );
				}
			}
			out_color = vec4( base, 1.0 );
		}
		else
		{
			out_color = vec4( 0.0 );
		}
	}
}
#endif
