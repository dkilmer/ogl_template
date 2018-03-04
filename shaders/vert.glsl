#version 330 core

in vec3 position;
in vec4 color;
in vec4 normal;
in vec2 uv_coord;

out vec4 Color;
out vec3 Normal;
out vec2 TexCoord;

uniform mat4 vp;

void main()
{
	Color = color;
	Normal = normal.xyz;
	TexCoord = uv_coord;
  gl_Position = vp * vec4(position, 1.0);
}
