#pragma once

const char* FRAGMENT_SHADER = R"(
#version 140

in vec4 fragColor;
in vec2 uvCoords;

out vec4 color;

uniform sampler2D mainTex;
uniform bool font;

void main()
{
	vec4 t = texture(mainTex, uvCoords);
	if(font)
	{	
		color = vec4(1, 1, 1, t.r) * fragColor;
	}
	else
	{
		color = t * fragColor;
	}
}
)";

const char* VERTEX_SHADER = R"(
#version 140

in vec2 position;
in vec4 color;
in vec2 uv;

out vec4 fragColor;
out vec2 uvCoords;

uniform mat4 MVP;

void main()
{
	gl_Position = MVP * vec4(position, 0, 1.0);
	
	fragColor = color;
	uvCoords = uv;
}
)";
