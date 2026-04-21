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
float3 srgb_to_linear(float3 c)
{
    float3 low  = c / 12.92;
    float3 high = pow((c + 0.055) / 1.055, 2.4);

    return lerp(high, low, step(c, 0.04045));
}
]] 
}