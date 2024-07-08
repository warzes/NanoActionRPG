#version 460

out gl_PerVertex { vec4 gl_Position; };
out vec2 TexCoords;

// full screen quad vertices
const vec2 verts[] = { vec2(-1.0f, 1.0f), vec2(1.0f, 1.0f), vec2(1.0f,-1.0f), vec2(-1.0f,-1.0f) };
const vec2 uvs[] = { vec2(0.0f, 1.0f), vec2(1.0f, 1.0f), vec2(1.0f, 0.0f), vec2(0.0f, 0.0f) };
const uint index[] = { 0, 3, 2, 2, 1, 0 };

void main()
{
	TexCoords = uvs[index[gl_VertexID]];
	gl_Position = vec4(verts[index[gl_VertexID]], 0.0, 1.0);
}