#include "BasicStructures.fxh"
#include "Shadows.fxh"
#include "SRGBUtilities.fxh"


cbuffer Constants
{
    float4x4 g_World;
    float4x4 g_ViewProj;
    float3 g_CameraPos;
    
    //float4x4 g_LightViewProj;
    // ... si quieres, más data (p.e. WorldInvTranspose para normales)
};

cbuffer LightConstants
{
    float4x4 g_ViewProjLight;
    float3 g_LightPos;  
};

cbuffer cbLightAttribs
{
    LightAttribs g_LightAttribs;
};

struct VSInput
{
    float3 pos : ATTRIB0;
    float3 normal : ATTRIB1;
    float2 uv : ATTRIB2;
    float4 tangent: ATTRIB3;
    //float3 bitangent: ATTRIB4;
};

struct VSOutput
{
    float4 posH : SV_POSITION; // posición en espacio Homogéneo (clip)
    float3 posWorld : TEXCOORD0; // posición en espacio mundial
    float3 normalWorld : TEXCOORD1; // normal en espacio mundial
    float3 fragmentLightPosDirLight : LIGHTSPACEPOS;

    //float4 color : COLOR0;
    float2 uv : TEXCOORD2;
    float3 viewDirTS : TEXCOORD3;
    float3 lightPosTS : TEXCOORD4;
    float4 fragmentLightPos : TEXCOORD5;  // posición en espacio tangente
    float3 lightPos : TEXCOORD6;
    float3 fragmentPosTS : TEXCOORD7;
    float3 directionalLightDirTS : TEXCOORD8; // dirección de la luz en espacio tangente
    float3 tangentViewPos : TEXCOORD9; // posición de la camara en espacio tangente
    
    
    //float posTS : TEXCOORD4; // posición espacio tangente

    
    //float4 posLightSpace : TEXCOORD2; // posición en espacio de la luz
};

VSOutput main(VSInput In)
{
    VSOutput Out;

    
     
    float4 wPos = mul(float4(In.pos, 1.0f), g_World);
    Out.posWorld = wPos.xyz;

 
    Out.normalWorld = normalize(mul(float4(In.normal, 0), g_World).xyz);
 
    
    Out.posH = mul(wPos, g_ViewProj);
    
    
    //Out.posH = mul(wPos, g_LightViewProj);

    // Calculamos la posición en espacio de la luz para el shadow map
    //Out.posLightSpace = mul(wPos, g_LightViewProj);
    
    //Montar la base ortonormal
    //float3 N = normalize(mul(In.normal, (float3x3) g_World));

    float3 N = normalize(mul(float4(In.normal, 0.0), g_World).xyz);
    //float3 T = normalize(mul(In.tangent.xyz, (float3x3) g_World));
    float3 T = normalize(mul(float4(In.tangent.xyz, 0.0), g_World).xyz);
    float3 B = In.tangent.w * cross(N, T); // LH  cross(N,T)
    
    float3x3 TBN = transpose(float3x3(T, B, N));
    
    //T = normalize(T - N * dot(N, T));
    //B = cross(N, T); // 100 % ortogonal
    
    //B = cross(T, N);
    
    float3 viewDirWS = normalize(g_CameraPos - Out.posWorld);
    
    Out.tangentViewPos = mul( g_CameraPos, TBN);
    
    
    //float3 viewDirWS = normalize(Out.posWorld - g_CameraPos);
    
    
    //Out.viewDirTS.x = dot(ViewDir).
    

    //Out.color = In.color;
    //Out.viewDirTS.x = float3(1.0f , 1.0f, 1.0f);
    
    //Out.viewDirTS.x = dot(viewDirWS, T);
    //Out.viewDirTS.y = dot(viewDirWS, B);
    //Out.viewDirTS.z = dot(viewDirWS, N);
    Out.viewDirTS.x = 0.0f;
    Out.viewDirTS.y = 0.0f;
    Out.viewDirTS.z = 1.0f;
    
    
    
    
    //float3 lightPosWS = float3(0.1f, 0.1f, 5.0f);
    
    float3 lightPosWS = g_LightPos;
    
    //Out.lightPosTS.x = dot(lightPosWS, T);
    //Out.lightPosTS.y = dot(lightPosWS, B);
    //Out.lightPosTS.z = dot(lightPosWS, N);
    
    Out.lightPosTS = mul(lightPosWS, TBN);
    
    Out.fragmentPosTS = mul(Out.posWorld, TBN);
    
    //Out.fragmentPosTS.x = dot(Out.posWorld, T);
    //Out.fragmentPosTS.y = dot(Out.posWorld, B);
    //Out.fragmentPosTS.z = dot(Out.posWorld, N);
    
    //Out.directionalLightDirTS.x = dot(g_LightAttribs.f4Direction.x, T);
    //Out.directionalLightDirTS.y = dot(g_LightAttribs.f4Direction.y, B);
    //Out.directionalLightDirTS.z = dot(g_LightAttribs.f4Direction.z, N);
    
    Out.directionalLightDirTS = mul(g_LightAttribs.f4Direction.xyz, TBN);
    
    
    
    
    Out.uv = In.uv;
    
    Out.fragmentLightPos = mul(wPos, g_ViewProjLight);
    
    Out.lightPos = lightPosWS;
    
    float4 lightDirPosClip = mul(wPos, g_LightAttribs.ShadowAttribs.mWorldToLightView);
    
    Out.fragmentLightPosDirLight = lightDirPosClip.xyz / lightDirPosClip.w;
    
    
    
    return Out;
}
