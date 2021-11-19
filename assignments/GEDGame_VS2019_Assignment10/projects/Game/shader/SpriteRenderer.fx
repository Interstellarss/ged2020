
Texture2D g_Sprites[2];

cbuffer cbChangesEveryFrame
{
    matrix g_ViewProjection;
    float3 g_CameraRight;
    float3 g_CameraUp;
}

// IO structs

struct SpriteVertex
{
    float3 pos : POSITION;
    float radius : RADIUS;
    int tex_index : TEX_INDEX;
};

struct SpriteFragment
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD0;
    int tex_index : TEX_INDEX;
};

// Other Stuff

SamplerState samAnisotropic
{
    Filter = ANISOTROPIC;
    AddressU = Wrap;
    AddressV = Wrap;
};

RasterizerState rsCullNone
{
    CullMode = None;
};

DepthStencilState EnableDepth
{
    DepthEnable = TRUE;
    DepthWriteMask = ALL;
    DepthFunc = LESS_EQUAL;
};

BlendState BlendMix
{
    BlendEnable[0] = TRUE;
    SrcBlend[0] = SRC_ALPHA;
    SrcBlendAlpha[0] = ONE;
    DestBlend[0] = INV_SRC_ALPHA;
    DestBlendAlpha[0] = INV_SRC_ALPHA;
};

// Shaders

SpriteVertex SpriteVS(in SpriteVertex vertex)
{
    return vertex;
}

[maxvertexcount(4)]
void SpriteGS(point SpriteVertex vertex[1], inout TriangleStream<SpriteFragment> stream)
{
    SpriteFragment sf;
    sf.tex_index = vertex[0].tex_index;
    
    // Upper left
    sf.pos = mul(float4(vertex[0].pos + (-g_CameraRight + g_CameraUp) * vertex[0].radius, 1), g_ViewProjection);
    sf.uv = float2(0, 0);
    stream.Append(sf);
    
    // Upper right
    sf.pos = mul(float4(vertex[0].pos + (g_CameraRight + g_CameraUp) * vertex[0].radius, 1), g_ViewProjection);
    sf.uv = float2(1,0);
    stream.Append(sf);
    
    // Lower left
    sf.pos = mul(float4(vertex[0].pos + (-g_CameraRight - g_CameraUp) * vertex[0].radius, 1), g_ViewProjection);
    sf.uv = float2(0, 1);
    stream.Append(sf);
    
    // Lower right
    sf.pos = mul(float4(vertex[0].pos + (g_CameraRight - g_CameraUp) * vertex[0].radius, 1), g_ViewProjection);
    sf.uv = float2(1, 1);
    stream.Append(sf);
}

float4 SpritePS(SpriteFragment pos) : SV_Target0
{
    switch (pos.tex_index)
    {
        case 0:
            return g_Sprites[0].Sample(samAnisotropic, pos.uv);
        case 1:
            return g_Sprites[1].Sample(samAnisotropic, pos.uv);
    }
    return float4(1, 0, 1, 1);
}

// Technique

technique11 Render
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_4_0, SpriteVS()));
        SetGeometryShader(CompileShader(gs_4_0, SpriteGS()));
        SetPixelShader(CompileShader(ps_4_0, SpritePS()));
        
        SetRasterizerState(rsCullNone);
        SetDepthStencilState(EnableDepth, 0);
        SetBlendState(BlendMix, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
    }
}