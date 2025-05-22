#pragma once
#include "RefCntAutoPtr.hpp"
#include "RenderDevice.h"
#include "DeviceContext.h"
#include "PipelineState.h"
#include "TextureUtilities.h"
#include "MapHelper.hpp"
#include "BasicMath.hpp" // float2
#include <cstring>       // std::memcpy


namespace Diligent
{

struct POMConstants
{
    float    HeightScale = 0.05f;
    float    _Pad0;
    float    _Pad1;
    uint32_t NumSteps = 16;
};

// -----------------------------------------------------------------------------
// Material POM: solo datos, NO crea su propio SRB
// -----------------------------------------------------------------------------
class POMMaterial
{
public:
    POMMaterial(IRenderDevice* pDevice,
                const char*    AlbedoPath,
                const char*    HeightPath,
                const char*    NormalPath,
                float          HeightScale = 0.05f,
                uint32_t       NumSteps    = 32,
                bool           sRGB        = true
                                       )
    {
        // Texturas -----------------------------------------------------------
        TextureLoadInfo li;
        li.IsSRGB = sRGB;

        RefCntAutoPtr<ITexture> tex;
        CreateTextureFromFile(AlbedoPath, li, pDevice, &tex);
        m_AlbedoSRV = tex->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
        OutputDebugStringA("Albedo texture loaded\n");

        RefCntAutoPtr<ITexture> tex2;
        CreateTextureFromFile(HeightPath, {}, pDevice, &tex2);
        m_HeightSRV = tex2->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
        OutputDebugStringA("Height texture loaded\n");

        RefCntAutoPtr<ITexture> tex3;
        CreateTextureFromFile(NormalPath, li, pDevice, &tex3);
        m_NormalSRV = tex3->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
        OutputDebugStringA("Normal texture loaded\n");


        // Constant buffer ----------------------------------------------------
        BufferDesc cbDesc;
        cbDesc.Name           = "POM constants";
        cbDesc.Size           = sizeof(POMConstants);
        cbDesc.BindFlags      = BIND_UNIFORM_BUFFER;
        cbDesc.Usage          = USAGE_DYNAMIC;
        cbDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
        pDevice->CreateBuffer(cbDesc, nullptr, &m_CB);

        m_Data.HeightScale = HeightScale;
        m_Data.NumSteps    = NumSteps;
    }

    // Cargar datos CPU -> GPU
    void Upload(IDeviceContext* ctx)
    {
        MapHelper<POMConstants> cb(ctx, m_CB, MAP_WRITE, MAP_FLAG_DISCARD);
        *cb = m_Data;
    }

    // Cambiar HeightScale
    void SetHeightScale(IDeviceContext* ctx, float hs)
    {
        m_Data.HeightScale = hs;
        Upload(ctx);
    }

    // Rellenar variables en el SRB existente
    void Bind(IShaderResourceBinding* srb) const
    {   
        srb->GetVariableByName(SHADER_TYPE_PIXEL, "g_Albedo")->Set(m_AlbedoSRV, SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
        srb->GetVariableByName(SHADER_TYPE_PIXEL, "g_HeightMap")->Set(m_HeightSRV, SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
        srb->GetVariableByName(SHADER_TYPE_PIXEL, "g_NormalMap")->Set(m_NormalSRV, SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
        srb->GetVariableByName(SHADER_TYPE_PIXEL, "cbPOM")->Set(m_CB, SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
    }

    public:
    void CreateSRB(IPipelineState* pPSO)
    {
        pPSO->CreateShaderResourceBinding(&m_SRB, true);
        Bind(m_SRB); //   hace los Set() una sola vez
    }

    void gettAlbedoSRV(ITextureView** srv) const
	{
		if (srv)
			*srv = m_AlbedoSRV;
	}
    void gettHeightSRV(ITextureView** srv) const
        {
        if (srv)
				*srv = m_HeightSRV;
		}

    void gettNormalSRV(ITextureView** srv) const
		{
		if (srv)
            *srv = m_NormalSRV;
        }



    IShaderResourceBinding* GetSRB() const { return m_SRB; }

private:
    RefCntAutoPtr<ITextureView> m_AlbedoSRV;
    RefCntAutoPtr<ITextureView> m_HeightSRV;
    RefCntAutoPtr<ITextureView> m_NormalSRV;
    RefCntAutoPtr<IBuffer>      m_CB;
    RefCntAutoPtr<IShaderResourceBinding> m_SRB;

    POMConstants m_Data = {};
};

} // namespace Diligent
