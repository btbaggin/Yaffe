#pragma once

const char* FRAGMENT_SHADER = R"(
#version 410 core

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 uvCoords;

layout(location = 0) out vec4 color;

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
#version 410 core

layout(location = 0) in vec2 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 uv;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 uvCoords;

uniform mat4 MVP;

void main()
{
	gl_Position = MVP * vec4(position, 0, 1.0);
	
	fragColor = color;
	uvCoords = uv;
}
)";
