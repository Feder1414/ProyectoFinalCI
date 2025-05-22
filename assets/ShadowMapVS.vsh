//#include "BasicStrctures.fxh"
#include "BasicStructures.fxh"
#include "Shadows.fxh"
#include "SRGBUtilities.fxh"


cbuffer ShadowConstants
{
    float4x4 g_WorldLightViewProj;
    float4x4 g_World;
    // Opcionalmente un float g_ShadowBias, 
    // pero normalmente el bias se maneja en el pass principal o se aplica al z antes de escribir profundidad.
};

struct VSInput
{
    float3 pos : ATTRIB0;
    float2 normal : ATTRIB1; // Si tu VB tiene más atributos (normal, uv), puedes declararlos, pero aquí no se usan.
    float2 uv : ATTRIB2;
    float3 tangente: ATTRIB3; // Si tu VB tiene más atributos (normal, uv), puedes declararlos, pero aquí no se usan.
    //float3 binormal: ATTRIB4; // Si tu VB tiene más atributos (normal, uv), puedes declararlos, pero aquí no se usan.
    // Si tu VB tiene más atributos (normal, uv), puedes declararlos, pero aquí no se usan.
};

struct VSOutput
{
    float4 pos : SV_POSITION; // El pipeline usará este valor para la prueba de profundidad
};

VSOutput main(VSInput In)
{
    VSOutput Out;
    
    float4 wPos = mul(float4(In.pos, 1.0f), g_World);
    Out.pos = mul(wPos, g_WorldLightViewProj);

    return Out;
}
