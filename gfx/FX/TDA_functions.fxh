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

float linear_to_gamma(float c)
{	
	if (c < 0.0031308) return c * 12.92;

	return 1.055 * pow(c, 0.41666) - 0.055;
}

float3 linear_to_gamma(float3 c)
{
    c.r = linear_to_gamma(c.r);
    c.g = linear_to_gamma(c.g);
    c.b = linear_to_gamma(c.b);
	
	return c;
}

float gamma_to_linear(float c)
{
	if (c < 0.04045) return c * 0.0773993808;

    return pow(c * 0.9478672986 + 0.0521327014, 2.4);
}

float3 gamma_to_linear(float3 c)
{
    c.r = gamma_to_linear(c.r);
    c.g = gamma_to_linear(c.g);
    c.b = gamma_to_linear(c.b);

	return c;
}
]] 
