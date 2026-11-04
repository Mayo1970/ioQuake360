#version 450

layout(location = 0) out vec2 frag_tex_coord;

#ifdef Q360_XENOS
// Q360: the corners come from vk.c's quad buffer (vk_create_quad_buffer), as xost has no gl_VertexIndex.
layout(location = 0) in vec2 in_position;
layout(location = 2) in vec2 in_tex_coord;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	gl_Position = vec4( in_position, 0.0f, 1.0f );
	frag_tex_coord = in_tex_coord;
}
#else
const vec2 v[4] = vec2[4](
	vec2(-1.0f, 1.0f),
	vec2(-1.0f,-1.0f),
	vec2( 1.0f, 1.0f),
	vec2( 1.0f,-1.0f)
);

const vec2 t[4] = vec2[4](
	vec2( 0.0f, 1.0f),
	vec2( 0.0f, 0.0f),
	vec2( 1.0f, 1.0f),
	vec2( 1.0f, 0.0f)
);

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	gl_Position = vec4( v[ gl_VertexIndex ], 0.0f, 1.0f );
	frag_tex_coord = t[ gl_VertexIndex ];
}
#endif
