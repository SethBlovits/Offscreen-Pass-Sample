struct VSInput
{
    float3 position   : POSITION;
    float2 uv    : TEXCOORD0;
};

struct PSInput
{
    float4 position   : SV_POSITION;
    float2 uv    : TEXCOORD0;
};

cbuffer MVPConstantBuffer : register(b0) { 
    column_major float4x4 mvpMatrix; 
};
Texture2D albedo : register(t0);
SamplerState g_sampler : register(s0);

PSInput VSMain(VSInput input)
{
    PSInput result;

    result.position = mul(mvpMatrix,float4(input.position.xyz,1.0f));

    result.uv = input.uv;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{   

    return albedo.Sample(g_sampler,input.uv); 
   
}