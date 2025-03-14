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
]] 
}