
#pragma once
#include "DungeonGenerator.h"
#include "Cubo.h" 
#include "MapHelper.hpp"

namespace Diligent
{


struct TileInstance
{
    float4x4 World;
    uint32_t MaterialId; // 0 = suelo, 1 = muro  
};

class DungeonScene
{
public:
    DungeonScene() = default; 

    DungeonScene(RefCntAutoPtr<IRenderDevice>  pDevice,
                 RefCntAutoPtr<IPipelineState> pPSO,
                 POMMaterial*                  pFloorMat,
                 POMMaterial*                  pWallMat,
                 float                         tileSize       = 2.0f,
                 float                         wallHeight     = 2.0f,
                 float                         floorThickness = 0.1f) :
        m_pDevice{pDevice},
        m_pPSO{pPSO},
        m_FloorMat{pFloorMat},
        m_WallMat{pWallMat},
        m_TileSize{tileSize},
        m_WallHeight{wallHeight},
        m_FloorThickness{floorThickness}
    {
        // 1 cubo base para TODA la escena
        m_CubeMesh = std::make_unique<Cubo>(pDevice, pPSO, /*id*/ 0);
    }

    /// Genera/actualiza la escena a partir de un DungeonGenerator ya construido
    void Build(const DungeonGenerator& dg)
    {
        m_Instances.clear();

        const int   W  = dg.GetWidth();
        const int   H  = dg.GetHeight();
        const float TS = m_TileSize;

        // desplazamos el origen para que el (0,0) quede centrado
        const float xOffset = -W * TS * 0.5f + TS * 0.5f;
        const float zOffset = -H * TS * 0.5f + TS * 0.5f;

        for (int y = 0; y < H; ++y)
        {
            for (int x = 0; x < W; ++x)
            {
                auto tile = dg.GetTile(x, y);
                if (tile == DungeonGenerator::Tile::Empty)
                    continue;

                float worldX = x * TS + xOffset;
                float worldZ = y * TS + zOffset;

                float4x4 S, T;
                if (tile == DungeonGenerator::Tile::Floor)
                {
                    // escalamos en Y para que sea finito (0.1 * altura muro aprox.)
                    S = float4x4::Scale(float3{TS * 0.5f, m_FloorThickness * 0.5f, TS * 0.5f});
                    T = float4x4::Translation(float3{worldX, -m_WallHeight * 0.5f + m_FloorThickness * 0.5f, worldZ});
                }
                else // Wall
                {
                    S = float4x4::Scale(float3{TS * 0.5f, m_WallHeight * 0.5f, TS * 0.5f});
                    T = float4x4::Translation(float3{worldX, 0.0f, worldZ});
                }

                TileInstance inst;
                inst.World      = S * T;
                inst.MaterialId = (tile == DungeonGenerator::Tile::Floor) ? 0u : 1u;
                m_Instances.push_back(inst);
            }
        }

        CreateInstanceBuffer();
    }

    ///// Dibuja toda la mazmorra (una llamada instanciada)
    //void Render(IDeviceContext*                       pCtx,
    //            const float4x4&                       viewProj,
    //            IShaderResourceBinding*               srb,
    //            RefCntAutoPtr<IShaderResourceBinding> materialSRB = nullptr)
    //{
    //    if (m_Instances.empty())
    //        return;

    //    // 1) actualizar constantes comunes
    //    //    (viewProj/camera ya lo haces fuera con tu CB global)

    //    // 2) elegir material (por ahora 2 draw-calls: suelo y muros)
    //    DrawSubset(pCtx, viewProj, srb, /*materialId*/ 0, m_FloorMat);
    //    DrawSubset(pCtx, viewProj, srb, /*materialId*/ 1, m_WallMat);
    //}

     const std::vector<TileInstance>& GetInstances() const noexcept
    {
        return m_Instances;
    }
    Cubo* GetCubeMesh() const
    {
        return m_CubeMesh.get();
    }





private:
    //----------------------------------------------------------
    void DrawSubset(IDeviceContext*         pCtx,
                    const float4x4&         viewProj,
                    IShaderResourceBinding* srb,
                    uint32_t                subsetId,
                    POMMaterial*            mat)
    {
        if (!mat) return;

        // **Filtrado burdo**: los índices de instancia con ese material
        std::vector<Uint32> instanceIndices;
        for (Uint32 i = 0; i < m_Instances.size(); ++i)
            if (m_Instances[i].MaterialId == subsetId)
                instanceIndices.push_back(i);
        if (instanceIndices.empty())
            return;

        pCtx->SetPipelineState(m_pPSO);
        mat->Bind(srb);
        mat->Upload(pCtx);
        pCtx->CommitShaderResources(srb, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        // 2) VB/IB del cubo
        IBuffer* pVB[]  = {m_CubeMesh->GetVertexBuffer()};
        Uint64   offs[] = {0};
        pCtx->SetVertexBuffers(0, 1, pVB, offs, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
        pCtx->SetIndexBuffer(m_CubeMesh->GetIndexBuffer(), 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        // 3) Instance buffer
        IBuffer* pInstVB[]  = {m_pInstanceBuffer};
        Uint64   instOffs[] = {0};
        //pCtx->SetVertexBuffers(1, 1, pInstVB, instOffs, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_KEEP);

        // 4) Draw instanced
        DrawIndexedAttribs draw;
        draw.IndexType             = VT_UINT32;
        draw.NumIndices            = m_CubeMesh->GetNumIndices();
        draw.NumInstances          = static_cast<Uint32>(instanceIndices.size());
        draw.Flags                 = DRAW_FLAG_VERIFY_ALL;
        draw.FirstInstanceLocation = 0; // asumiendo que el instance buffer está ordenado
        pCtx->DrawIndexed(draw);
    }
    /** Devuelve la lista completa de instancias que debes dibujar. */
   
    //----------------------------------------------------------
    void CreateInstanceBuffer()
    {
        if (m_Instances.empty())
            return;

        BufferDesc desc;
        desc.Name           = "Dungeon instance buffer";
        desc.Usage          = USAGE_DYNAMIC;
        desc.BindFlags      = BIND_VERTEX_BUFFER;
        desc.CPUAccessFlags = CPU_ACCESS_WRITE;
        desc.Size           = static_cast<Uint32>(m_Instances.size() * sizeof(TileInstance));

        BufferData data;
        data.pData    = m_Instances.data();
        data.DataSize = desc.Size;

        m_pDevice->CreateBuffer(desc, &data, &m_pInstanceBuffer);
    }




  
    

private:
    // --- resources -----------------------------------------------------
    std::unique_ptr<Cubo>         m_CubeMesh;
    RefCntAutoPtr<IBuffer>        m_pInstanceBuffer;
    RefCntAutoPtr<IRenderDevice>  m_pDevice;
    RefCntAutoPtr<IPipelineState> m_pPSO;

    POMMaterial* m_FloorMat = nullptr;
    POMMaterial* m_WallMat  = nullptr;

    // --- data ----------------------------------------------------------
    std::vector<TileInstance> m_Instances;
    float                     m_TileSize;
    float                     m_WallHeight;
    float                     m_FloorThickness;
};

} // namespace Diligent
