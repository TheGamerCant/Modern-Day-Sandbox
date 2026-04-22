Code
[[

]]


PixelShader = {
Code 
[[

float4 colourBurn(float4 baseColour, float4 burnColour) 
{
	if(baseColour.a !=0) {
		float3 returnColour = 1 - baseColour.rgb;
		returnColour = 1 - (returnColour / burnColour.rgb);
		return float4(returnColour.rgb, burnColour.a);
	}
	
	return (0.0f, 0.0f, 0.0f, 0.0f);
}

float srgb_to_linear(float c)
{	
	if (c < 0.04045f) return c * 0.773993808f;
	
	return pow(c * 0.9478672986f + 0.052137014f, 2.4);
}

float3 srgb_to_linear(float3 c)
{
    c.r = srgb_to_linear(c.r);
    c.g = srgb_to_linear(c.g);
    c.b = srgb_to_linear(c.b);
	
	return c;
}
]] 
