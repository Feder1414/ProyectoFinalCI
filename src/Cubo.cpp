// Cubo.cpp
#include "Cubo.h"
#include <vector>
#include <cmath>

namespace Diligent
{

struct RawVertex
{
    float3 pos;
    float3 normal;
    float2 uv;
};

struct TangentVertex
{
    float3 pos;
    float3 normal;
    float2 uv;
    float4 tangent;
    //float3 bitangent;
};

Cubo::Cubo(RefCntAutoPtr<IRenderDevice>  device,
           RefCntAutoPtr<IPipelineState> pPSO,
           std::uint32_t                 id) 
{
    m_id = id;
    m_pDevice = device;
    // Recuerda actualizar tu InputLayout para incluir:
    //   ATTRIB3: float3 tangent
    //   ATTRIB4: float3 bitangent
    pPSO->CreateShaderResourceBinding(&m_SRB, true);

    crearBufferVertices();
    crearBufferIndices();
    
}

void Cubo::crearBufferVertices()
{
    // 1) Define los vértices “raw” sin tangentes
    RawVertex rawVerts[24] =
        {
            // +Z frontal
            {{-1, +1, +1}, {0, 0, 1}, {1, 0}},
            {{+1, +1, +1}, {0, 0, 1}, {0, 0}},
            {{+1, -1, +1}, {0, 0, 1}, {0, 1}},
            {{-1, -1, +1}, {0, 0, 1}, {1, 1}},
            // -Z posterior
            {{+1, +1, -1}, {0, 0, -1}, {1, 0}},
            {{-1, +1, -1}, {0, 0, -1}, {0, 0}},
            {{-1, -1, -1}, {0, 0, -1}, {0, 1}},
            {{+1, -1, -1}, {0, 0, -1}, {1, 1}},
            // -X izquierda
            {{-1, +1, -1}, {-1, 0, 0}, {1, 0}},
            {{-1, +1, +1}, {-1, 0, 0}, {0, 0}},
            {{-1, -1, +1}, {-1, 0, 0}, {0, 1}},
            {{-1, -1, -1}, {-1, 0, 0}, {1, 1}},
            // +X derecha
            {{+1, +1, +1}, {+1, 0, 0}, {1, 0}},
            {{+1, +1, -1}, {+1, 0, 0}, {0, 0}},
            {{+1, -1, -1}, {+1, 0, 0}, {0, 1}},
            {{+1, -1, +1}, {+1, 0, 0}, {1, 1}},
            // +Y superior
            {{-1, +1, -1}, {0, 1, 0}, {0, 1}},
            {{+1, +1, -1}, {0, 1, 0}, {1, 1}},
            {{+1, +1, +1}, {0, 1, 0}, {1, 0}},
            {{-1, +1, +1}, {0, 1, 0}, {0, 0}},
            // -Y inferior
            {{-1, -1, +1}, {0, -1, 0}, {0, 1}},
            {{+1, -1, +1}, {0, -1, 0}, {1, 1}},
            {{+1, -1, -1}, {0, -1, 0}, {1, 0}},
            {{-1, -1, -1}, {0, -1, 0}, {0, 0}}};

    // 2) Índices del cubo
    Uint32 indices[] =
        {
            0, 1, 2, 0, 2, 3,       // frontal
            4, 5, 6, 4, 6, 7,       // posterior
            8, 9, 10, 8, 10, 11,    // izquierda
            12, 13, 14, 12, 14, 15, // derecha
            16, 17, 18, 16, 18, 19, // superior
            20, 21, 22, 20, 22, 23  // inferior
        };

        std::vector<TangentVertex> verts(24);
    std::vector<float3>        bitanAccum(24, float3{0, 0, 0});

    for (int i = 0; i < 24; ++i)
    {
        verts[i].pos     = rawVerts[i].pos;
        verts[i].normal  = rawVerts[i].normal;
        verts[i].uv      = rawVerts[i].uv;
        verts[i].tangent = float4{0, 0, 0, 0};
    }

    /*----------------------------------------------------------*
     * 3. Acumular T y B                                        *
     *----------------------------------------------------------*/
    for (int t = 0; t < 36; t += 3)
    {
        Uint32 i0 = indices[t], i1 = indices[t + 1], i2 = indices[t + 2];

        auto& v0 = verts[i0];
        auto& v1 = verts[i1];
        auto& v2 = verts[i2];

        float3 edge1 = v1.pos - v0.pos;
        float3 edge2 = v2.pos - v0.pos;
        float2 dUV1  = v1.uv - v0.uv;
        float2 dUV2  = v2.uv - v0.uv;

        float det = dUV1.x * dUV2.y - dUV2.x * dUV1.y;
        if (std::fabs(det) < 1e-6f) det = 1e-6f;
        float invDet = 1.0f / det;

        float3 T = {
            invDet * (dUV2.y * edge1.x - dUV1.y * edge2.x),
            invDet * (dUV2.y * edge1.y - dUV1.y * edge2.y),
            invDet * (dUV2.y * edge1.z - dUV1.y * edge2.z)};

        float3 B = {
            invDet * (-dUV2.x * edge1.x + dUV1.x * edge2.x),
            invDet * (-dUV2.x * edge1.y + dUV1.x * edge2.y),
            invDet * (-dUV2.x * edge1.z + dUV1.x * edge2.z)};

        // Añadir a la tangente acumulada SIN swizzle
        float4 t4{T.x, T.y, T.z, 0.0f};
        verts[i0].tangent += t4;
        verts[i1].tangent += t4;
        verts[i2].tangent += t4;

        bitanAccum[i0] += B;
        bitanAccum[i1] += B;
        bitanAccum[i2] += B;
    }

    /*----------------------------------------------------------*
     * 4. Ortonormalizar y poner handedness en w                *
     *----------------------------------------------------------*/
    for (int i = 0; i < 24; ++i)
    {
        auto& v = verts[i];

        float3 N = normalize(v.normal);

        float3 TanXYZ{v.tangent.x, v.tangent.y, v.tangent.z};
        float3 T = normalize(TanXYZ - N * dot(N, TanXYZ)); // Gram-Schmidt
        float3 B = normalize(cross(N, T));

        float sign = (dot(B, bitanAccum[i]) < 0.0f) ? -1.0f : 1.0f;
        v.tangent  = float4{T.x, T.y, T.z, sign};
    }

    /*----------------------------------------------------------*
     * 5. Crear el vertex buffer                                *
     *----------------------------------------------------------*/
    BufferDesc vbDesc;
    vbDesc.Name      = ("Cubo VB " + std::to_string(m_id)).c_str();
    vbDesc.Usage     = USAGE_IMMUTABLE;
    vbDesc.BindFlags = BIND_VERTEX_BUFFER;
    vbDesc.Size      = static_cast<Uint32>(verts.size() * sizeof(TangentVertex));

    BufferData vbData{verts.data(), vbDesc.Size};
    m_pDevice->CreateBuffer(vbDesc, &vbData, &m_VertexBuffer);
}

void Cubo::crearBufferIndices()
{
    constexpr Uint32 Indices[] =
        {
            0, 1, 2, 0, 2, 3,
            4, 5, 6, 4, 6, 7,
            8, 9, 10, 8, 10, 11,
            12, 13, 14, 12, 14, 15,
            16, 17, 18, 16, 18, 19,
            20, 21, 22, 20, 22, 23};

    BufferDesc IndBuffDesc;
    IndBuffDesc.Name      = ("Cubo index buffer" + std::to_string(m_id)).c_str();
    IndBuffDesc.Usage     = USAGE_IMMUTABLE;
    IndBuffDesc.BindFlags = BIND_INDEX_BUFFER;
    IndBuffDesc.Size      = sizeof(Indices);

    BufferData IBData;
    IBData.pData    = Indices;
    IBData.DataSize = sizeof(Indices);
    m_pDevice->CreateBuffer(IndBuffDesc, &IBData, &m_IndexBuffer);

    m_NumIndices = _countof(Indices);
}


void Cubo::crearBufferConstante()
{}

void Cubo::Actualizar(float deltaTime)
{

}


} // namespace Diligent
