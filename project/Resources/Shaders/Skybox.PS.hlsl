TextureCube<
float32_t4 gTexture : register(t0);

float4 main() : SV_TARGET
{
	return float4(1.0f, 1.0f, 1.0f, 1.0f);
}