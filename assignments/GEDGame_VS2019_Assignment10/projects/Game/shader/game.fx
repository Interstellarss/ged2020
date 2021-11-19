//--------------------------------------------------------------------------------------
// Shading constants
//--------------------------------------------------------------------------------------

static const float3 light = float3(1.000000, 0.948336, 0.880797); // Sun color
static const float3 ambient = float3(0.025525, 0.045511, 0.088005); // Sky color
static const float mesh_specularity = 0.2;

//--------------------------------------------------------------------------------------
// Shader resources
//--------------------------------------------------------------------------------------

Texture2D g_DiffuseTex; // Material albedo for diffuse lighting
Texture2D g_NormalTex;
Texture2D g_SpecularTex;
Texture2D g_GlowTex;
Buffer<float> g_HeightMap;

//--------------------------------------------------------------------------------------
// Constant buffers
//--------------------------------------------------------------------------------------

cbuffer cbConstant
{
    float4 g_LightDir; // Object space
    int g_TerrainRes;
};

cbuffer cbChangesEveryFrame
{
    matrix g_World;
    matrix g_WorldViewProjection;
    matrix g_WorldNormals;
    float4 g_cameraPosWorld;
    float g_Time;
};

cbuffer cbUserChanges
{
};

//--------------------------------------------------------------------------------------
// Structs
//--------------------------------------------------------------------------------------

struct PosNorTex
{
    float4 Pos : SV_POSITION;
    float4 Nor : NORMAL;
    float2 Tex : TEXCOORD;
};

struct PosTexLi
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD;
    float Li : LIGHT_INTENSITY;
    float3 normal : NORMAL;
};

struct PosTex
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD;
};

struct T3dVertexVSIn
{
    float3 Pos : POSITION; //Position in object space     
    float2 Tex : TEXCOORD; //Texture coordinate     
    float3 Nor : NORMAL; //Normal in object space     
    float3 Tan : TANGENT; //Tangent in object space (not used in Ass. 5) 
};
	
struct T3dVertexPSIn
{
    float4 Pos : SV_POSITION; //Position in clip space     
    float2 Tex : TEXCOORD; //Texture coordinate     
    float3 PosWorld : POSITION; //Position in world space     
    float3 NorWorld : NORMAL; //Normal in world space     
    float3 TanWorld : TANGENT; //Tangent in world space (not used in Ass. 5) 
};

//--------------------------------------------------------------------------------------
// Samplers
//--------------------------------------------------------------------------------------

SamplerState samAnisotropic
{
    Filter = ANISOTROPIC;
    AddressU = Wrap;
    AddressV = Wrap;
};

SamplerState samLinearClamp
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Clamp;
    AddressV = Clamp;
};

//--------------------------------------------------------------------------------------
// Rasterizer states
//--------------------------------------------------------------------------------------

RasterizerState rsDefault
{
};

RasterizerState rsCullFront
{
    CullMode = Front;
};

RasterizerState rsCullBack
{
    CullMode = Back;
};

RasterizerState rsCullNone
{
    CullMode = None;
};

RasterizerState rsLineAA
{
    CullMode = None;
    AntialiasedLineEnable = true;
};


//--------------------------------------------------------------------------------------
// DepthStates
//--------------------------------------------------------------------------------------
DepthStencilState EnableDepth
{
    DepthEnable = TRUE;
    DepthWriteMask = ALL;
    DepthFunc = LESS_EQUAL;
};

BlendState NoBlending
{
    AlphaToCoverageEnable = FALSE;
    BlendEnable[0] = FALSE;
};

//--------------------------------------------------------------------------------------
// Helpers
//--------------------------------------------------------------------------------------

inline float4 dehom(float4 co)
{
    return co / co.w;
}

//--------------------------------------------------------------------------------------
// Shaders
//--------------------------------------------------------------------------------------

PosTexLi SimpleVS(PosNorTex Input)
{
    PosTexLi output = (PosTexLi) 0;

    // Transform position from object space to homogenious clip space
    output.Pos = mul(Input.Pos, g_WorldViewProjection);

    // Pass trough normal and texture coordinates
    output.Tex = Input.Tex;

    // Calculate light intensity
    output.normal = normalize(mul(Input.Nor, g_World).xyz); // Assume orthogonal world matrix
    output.Li = saturate(dot(output.normal, g_LightDir.xyz));
        
    return output;
}

float4 SimplePS(PosTexLi Input) : SV_Target0
{
    // Perform lighting in object space, so that we can use the input normal "as it is"
    //float4 matDiffuse = g_Diffuse.Sample(samAnisotropic, Input.Tex);
    float4 matDiffuse = g_DiffuseTex.Sample(samLinearClamp, Input.Tex);
    return float4(matDiffuse.rgb * Input.Li, 1);
	//return float4(Input.normal, 1);
}

PosTex TerrainVS(uint VertexID : SV_VertexID)
{
    PosTex output = (PosTex) 0;
	
    output.Tex.x = VertexID % g_TerrainRes;
    output.Tex.y = (uint) (VertexID / g_TerrainRes);
	
    output.Tex /= g_TerrainRes - 1;
	
    output.Pos.x = output.Tex.x - 0.5f;
    output.Pos.y = g_HeightMap[VertexID];
    output.Pos.z = output.Tex.y - 0.5f;
    output.Pos.w = 1;
    output.Pos = mul(output.Pos, g_WorldViewProjection);
	
    return output;
}

float4 TerrainPS(PosTex i) : SV_Target0
{
    float3 normal;
    normal.xz = g_NormalTex.Sample(samAnisotropic, i.Tex).rg * 1.98f - 0.99f;
	// Use max() to fix some shading bugs due to normalmap compression
    normal.y = sqrt(max(1.0f - normal.x * normal.x - normal.z * normal.z, 0.01));
    normal = normalize(mul(float4(normal, 0.0f), g_WorldNormals).xyz);
	
    float3 diff = g_DiffuseTex.Sample(samAnisotropic, i.Tex).rgb;
    float3 diff_light = saturate(dot(normal, g_LightDir.xyz)) * light + ambient;

    float tmp = normal.y;
    return float4(diff * diff_light, 1.0f);
}

T3dVertexPSIn MeshVS(T3dVertexVSIn input)
{
    T3dVertexPSIn output;
	
    output.Pos = mul(float4(input.Pos, 1), g_WorldViewProjection);
    output.Tex = input.Tex;
    output.PosWorld = dehom(mul(float4(input.Pos, 1), g_World)).xyz;
    output.NorWorld = mul(float4(input.Nor, 0), g_WorldNormals).xyz;
    output.TanWorld = mul(float4(input.Tan, 0), g_World).xyz;
	
    return output;
}

float4 MeshPS(T3dVertexPSIn input) : SV_Target0
{
    float3 n = normalize(input.NorWorld);
    float3 v = normalize(g_cameraPosWorld.xyz - input.PosWorld);
	
    float4 diff = g_DiffuseTex.Sample(samAnisotropic, input.Tex);
    float4 spec = g_SpecularTex.Sample(samAnisotropic, input.Tex);
    float4 glow = g_GlowTex.Sample(samAnisotropic, input.Tex);
	
    float3 diff_light = saturate(dot(n, g_LightDir.xyz)) * float3(1, 1, 1) * light + ambient;
	// Reflections not only reflect lightsources
    float3 spec_light = pow(saturate(dot(reflect(g_LightDir.xyz * -1, n), v)), 128) * light + ambient;
	
    float3 diffuse = diff.rgb * diff_light;
    float3 specular = spec.rgb * spec_light;
	
    return float4(diffuse * (1.0f - mesh_specularity) + specular * mesh_specularity + glow.rgb, 1);

}

//--------------------------------------------------------------------------------------
// Techniques
//--------------------------------------------------------------------------------------
technique11 Render
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_4_0, TerrainVS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_4_0, TerrainPS()));
        
        SetRasterizerState(rsCullNone);
        SetDepthStencilState(EnableDepth, 0);
        SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
    }

    pass P1_Mesh
    {
        SetVertexShader(CompileShader(vs_4_0, MeshVS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_4_0, MeshPS()));
        
        SetRasterizerState(rsCullBack);
        SetDepthStencilState(EnableDepth, 0);
        SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
    }
}
