#pragma once

vec2 uvToNdc(vec2 uv) 
{
	return vec2(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0);
}

vec2 ndcToUv(vec2 ndc)
{
	return vec2(ndc.x * 0.5 + 0.5, 0.5 - ndc.y * 0.5);
}
