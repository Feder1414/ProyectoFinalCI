// ShadowMap.h
#pragma once

#include "RenderDevice.h"
#include "DeviceContext.h"
#include "RefCntAutoPtr.hpp"
#include "Texture.h"
#include "BasicMath.hpp"
#include "ShaderSourceFactoryUtils.hpp"
#include "Utilities/interface/DiligentFXShaderSourceStreamFactory.hpp"


namespace Diligent
{

struct ShadowConstantsData
{
    float4x4 g_LightViewProj;
    float4x4 g_World;
};


class ShadowMap
{
public:
    ShadowMap() = default;

    // Inicializa el shadow map con las dimensiones especificadas.
    void Initialize(IRenderDevice* pDevice, Uint32 Width, Uint32 Height)
    {
        TextureDesc Desc;
        Desc.Name   = "Shadow Map";
        Desc.Type   = RESOURCE_DIM_TEX_2D;
        Desc.Width  = 1024;
        Desc.Height = 1024;
        //Desc.MipLevels = 1;
        //Desc.Format = TEX_FORMAT_D32_FLOAT; // Formato de profundidad de 32 bits
        Desc.Format = TEX_FORMAT_D16_UNORM; // Formato de profundidad de 16 bits
        //Desc.Format = TEX_FORMAT_D16_UNORM;
        Desc.Usage = USAGE_DEFAULT;
        // Se usará tanto para depth-stencil como para shader resource.
        Desc.BindFlags = BIND_SHADER_RESOURCE | BIND_DEPTH_STENCIL;

        pDevice->CreateTexture(Desc, nullptr, &m_pShadowMap);

        // En Diligent, se utilizan ITextureView para todas las vistas
        m_pShadowDSV = m_pShadowMap->GetDefaultView(TEXTURE_VIEW_DEPTH_STENCIL);
        m_pShadowSRV = m_pShadowMap->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);

        // Crear el PSO para el shadow pass.
        InitializePSO(pDevice);
    }

    // Crea el PSO específico para el shadow pass.
    void InitializePSO(IRenderDevice* pDevice)
    {
        GraphicsPipelineStateCreateInfo PSOCreateInfo;
        PSOCreateInfo.PSODesc.Name         = "ShadowMap PSO";
        PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

        // Este pass no renderiza color, solo depth.
        PSOCreateInfo.GraphicsPipeline.NumRenderTargets = 0;
        // Establece el formato del depth buffer igual que el del shadow map.
        PSOCreateInfo.GraphicsPipeline.DSVFormat = m_pShadowMap->GetDesc().Format;
        // Usa topología de triángulos.
        PSOCreateInfo.GraphicsPipeline.RTVFormats[0]                  = TEX_FORMAT_UNKNOWN;
        PSOCreateInfo.GraphicsPipeline.PrimitiveTopology              = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode        = CULL_MODE_NONE;
        PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable   = True;
        
        //PSOCreateInfo.GraphicsPipeline.RasterizerDesc.DepthClipEnable = False;

        // Define el input layout de los vértices (ajústalo a tu formato)
        LayoutElement LayoutElems[] =
            {
            // Posición 
                LayoutElement{0, 0, 3, VT_FLOAT32, False},

                // Normales
                LayoutElement{1, 0, 3, VT_FLOAT32, False},

                // Coordenadas de textura
                LayoutElement{2, 0, 2, VT_FLOAT32, False},

                // Tangente
                LayoutElement{3, 0, 4, VT_FLOAT32, False},





            };

        PSOCreateInfo.GraphicsPipeline.InputLayout.LayoutElements = LayoutElems;
        PSOCreateInfo.GraphicsPipeline.InputLayout.NumElements    = _countof(LayoutElems);

        // Crear shaders específicos para el shadow pass.
        ShaderCreateInfo ShaderCI;
        ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
        ShaderCI.CompileFlags   = SHADER_COMPILE_FLAG_PACK_MATRIX_ROW_MAJOR;
        RefCntAutoPtr<IShader>                         pVS, pPS;
        RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
        ShaderCI.Desc.UseCombinedTextureSamplers = true;
        pDevice->GetEngineFactory()->CreateDefaultShaderSourceStreamFactory(nullptr, &pShaderSourceFactory);
        auto*                                          pFXRaw = &DiligentFXShaderSourceStreamFactory::GetInstance();
        RefCntAutoPtr<IShaderSourceInputStreamFactory> pFXFactory{pFXRaw};
        RefCntAutoPtr<IShaderSourceInputStreamFactory> pCompound = CreateCompoundShaderSourceFactory({pShaderSourceFactory, pFXFactory});

        ShaderCI.pShaderSourceStreamFactory = pCompound;

        // Vertex Shader para shadow mapping (transforma posiciones a la vista de la luz)
        ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
        ShaderCI.Desc.Name       = "ShadowMap VS";
        ShaderCI.FilePath        = "shadowMapVS.vsh";
        ShaderCI.EntryPoint      = "main";
        pDevice->CreateShader(ShaderCI, &pVS);


        /*ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.Desc.Name       = "ShadowMap PS";
        ShaderCI.FilePath        = "ShadowMapPS.psh";
        ShaderCI.EntryPoint      = "main";
        pDevice->CreateShader(ShaderCI, &pPS);*/

        PSOCreateInfo.pVS = pVS;
        PSOCreateInfo.pPS = nullptr;
      


        ShaderResourceVariableDesc Vars[] =
            {
                {SHADER_TYPE_VERTEX, "ShadowConstants", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE}};
        PSOCreateInfo.PSODesc.ResourceLayout.Variables    = Vars;
        PSOCreateInfo.PSODesc.ResourceLayout.NumVariables = _countof(Vars);
        // Crea el PSO para el shadow pass.
        pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pShadowPSO);

        {
            BufferDesc CBDesc;
            CBDesc.Name           = "Shadow constants CB";
            CBDesc.Size           = sizeof(ShadowConstantsData);
            CBDesc.Usage          = USAGE_DYNAMIC;
            CBDesc.BindFlags      = BIND_UNIFORM_BUFFER;
            CBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
            pDevice->CreateBuffer(CBDesc, nullptr, &m_ShadowCB);
        }

        m_pShadowPSO->CreateShaderResourceBinding(&m_pShadowSRB, true);

        m_pShadowSRB->GetVariableByName(SHADER_TYPE_VERTEX, "ShadowConstants")->Set(m_ShadowCB);
    }



    ITextureView*                 GetDSV() const { return m_pShadowDSV; }
    ITextureView*                 GetSRV() const { return m_pShadowSRV; }
    RefCntAutoPtr<IPipelineState> GetShadowPSO() const { return m_pShadowPSO; }
    IShaderResourceBinding*       GetSRB() const { return m_pShadowSRB; }
    IBuffer*                      GetCB() const { return m_ShadowCB; }

private:
    RefCntAutoPtr<ITexture>               m_pShadowMap;
    RefCntAutoPtr<ITextureView>           m_pShadowDSV;
    RefCntAutoPtr<ITextureView>           m_pShadowSRV;
    RefCntAutoPtr<IPipelineState>         m_pShadowPSO;
    RefCntAutoPtr<IShaderResourceBinding> m_pShadowSRB;
    RefCntAutoPtr<IBuffer>                m_ShadowCB;
};

} // namespace Diligent
