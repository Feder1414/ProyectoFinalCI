#pragma once

#include "RefCntAutoPtr.hpp"
#include "EngineFactoryD3D11.h"
#include "RenderDevice.h"
#include "DeviceContext.h"
#include "SwapChain.h"
#include "MapHelper.hpp"
#include "BasicMath.hpp"
#include <vector>
#include "POMMaterial.h"
#include <Windows.h>



namespace Diligent
{




class Objeto3D
{

protected:
    float4x4                               m_Transform; // Matriz de transformación (posición, rotación, escala)
    RefCntAutoPtr<IBuffer>                 m_VertexBuffer;
    RefCntAutoPtr<IBuffer>                 m_IndexBuffer;
    RefCntAutoPtr<IPipelineState>          m_PipelineState;
    RefCntAutoPtr<IShaderResourceBinding>  m_SRB;
    RefCntAutoPtr<IRenderDevice>           m_pDevice;
    RefCntAutoPtr<IBuffer>                 m_BufferConstante;
    RefCntAutoPtr<IBuffer>                 m_BufferMaterialConstants;
    float3                                 m_Posicion = float3{0, 0, 0};
    float3                                 m_Rotacion = float3{0, 0, 0};
    float3                                 m_Escala   = float3{1, 1, 1};
    std::uint32_t                          m_id;
    Objeto3D*                              m_Parent = nullptr;
    std::vector<std::unique_ptr<Objeto3D>> m_Children;
    std::uint32_t                          m_NumIndices;
    POMMaterial *                        m_Material = nullptr; // Material asociado al objeto





public:
    void      SetPosition(const float3& pos) { m_Posicion = pos; }
    void      SetRotation(const float3& rot) { m_Rotacion = rot; }
    void      SetScale(const float3& scale) { m_Escala = scale; }
    IBuffer*  GetVertexBuffer() { return m_VertexBuffer; }
    IBuffer*  GetIndexBuffer() { return m_IndexBuffer; }
    int       GetNumIndices() { return m_NumIndices; }

    const std::vector<std::unique_ptr<Objeto3D>>& getChildren() const { return m_Children; }
    float3                                        getPosition() { return m_Posicion; }
    float3                                        getRotation() { return m_Rotacion; }
    std::uint32_t                                 GetID() const { return m_id; }
    void                                          SetID(std::uint32_t id) { m_id = id; }
    virtual float4x4                              GetTransformMatrix() const
    {
        // 1) Scale

        auto S = float4x4::Scale(m_Escala);
        // 2) Rotation -> O uses RotationXYZ
        auto R = float4x4::RotationX(m_Rotacion.x) * float4x4::RotationY(m_Rotacion.y) * float4x4::RotationZ(m_Rotacion.z);
        // 3) Translation
        auto T = float4x4::Translation(m_Posicion);
        //return T * R * S;
        //return R * T * S;


        return R * T * S; // o R * T, si no necesitas escala
    }

    Objeto3D* BuscarPorID(int id)
    {
        // Si este nodo tiene el ID buscado, lo retorna
        if (m_id == id)
            return this;
        // Sino, recorre sus hijos y pregunta a cada uno
        for (auto& hijo : m_Children)
        {
            if (Objeto3D* encontrado = hijo->BuscarPorID(id))
                return encontrado;
        }
        // Si ninguno coincide, retorna nullptr
        return nullptr;
    }

    virtual float3 GetBoundingSphereCenter() const
    {
        float4 localCenter = float4(m_Posicion, 1.0f);
        float4 worldCenter = localCenter * GetWorldTransform();
        return float3{worldCenter.x, worldCenter.y, worldCenter.z};
    }

    virtual float GetBoundingSphereRadius() const
    {
        // Por defecto, retorna un valor fijo o calcula en función de la escala.
        // Por ejemplo:
        return 1.0f; // Ajusta según el tamaño del objeto.
    }


    virtual float4x4 giveChildTranform() const
    {
        // 1) Scale


        // 2) Rotation -> O uses RotationXYZ
        auto R = float4x4::RotationX(m_Rotacion.x) * float4x4::RotationY(m_Rotacion.y) * float4x4::RotationZ(m_Rotacion.z);
        // 3) Translation
        auto T = float4x4::Translation(m_Posicion);
        //return T * R * S;
        //return R * T * S;


        return R * T; // o R * T, si no necesitas escala
    }





    float4x4 GetWorldTransform() const
    {
        if (m_Parent)
            //return m_Parent->GetWorldTransform() * GetTransformMatrix();
            return GetTransformMatrix() * m_Parent->GetWorldTransform();
        //return GetTransformMatrix() * m_Parent->giveChildTranform();
        else
            return GetTransformMatrix();
    }

    void AddChild(std::unique_ptr<Objeto3D> child)
    {
        // El hijo sepa que su padre soy yo
        child->m_Parent = this;
        // Mover al vector
        m_Children.push_back(std::move(child));
    }
    void RenderAll(RefCntAutoPtr<IDeviceContext> pContext,
                   RefCntAutoPtr<IPipelineState> pPSO,
                   float4x4                      viewProj,
                   bool                          isShadowPass)
    {
        //// Calcular transform final
        //float4x4 wMatrix = GetWorldTransform();
        //float4x4 wvp     = wMatrix * viewProj;

        //// Render SOLO ESTE objeto
        //Render(pContext, pPSO, wvp, isShadowPass);

        //// Renderar recursivamente todos los hijos
        //for (auto& child : m_Children)
        //{
        //    child->RenderAll(pContext, pPSO, viewProj, isShadowPass);
        //}
    }
    virtual void Recorrer(std::function<void(Objeto3D*)> func)
    {
        func(this);
        for (auto& hijo : m_Children)
        {
            hijo->Recorrer(func);
        }
    }

    /*virtual void SetTexture(const std::string& textureName)
    {
        auto texture = Diligent::ResourceManager::GetInstance().GetTexture(textureName);
        m_SRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Texture")->Set(texture->GetSRV());
    }*/

  

    IBuffer* GetBufferConstante() { return m_BufferConstante; }

    virtual void crearBufferVertices()  = 0;
    virtual void crearBufferIndices()   = 0;
    virtual void crearBufferConstante() = 0;

    //virtual void Render(RefCntAutoPtr<IDeviceContext> pContext, RefCntAutoPtr<IPipelineState> m_PSO, float4x4 viewProjection) = 0; // Método virtual puro
    //virtual void Render(RefCntAutoPtr<IDeviceContext> pContext, RefCntAutoPtr<IPipelineState> m_PSO, float4x4 viewProjection) = 0; // Método virtual puro
    //void Render(IDeviceContext* pContext,
    //            IPipelineState* pPSO,
    //            float4x4        worldViewProjection,
    //            bool            isShadowPass)
    //{
    //    // Si tienes un PSO distinto para sombras, configúralo aquí:
    //    pContext->SetPipelineState(pPSO);

    //    if (!isShadowPass)
    //    {
    //        // Solo en el pass normal usamos materiales / texturas
    //        MaterialConstantsData matData = {};
    //        matData.g_UseTexture          = 0;
    //        if (m_Material != nullptr)
    //        {

    //            matData.g_UseTexture = 1;
    //            m_Material->SetMaterialConstantsData(matData);
    //            MapHelper<MaterialConstantsData> MatConstants(pContext, m_BufferMaterialConstants, MAP_WRITE, MAP_FLAG_DISCARD);
    //            *MatConstants = matData;
    //            m_SRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Texture")->Set(m_Material->GetTexture());
    //        }
    //        pContext->CommitShaderResources(m_SRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    //    }
    //    else
    //    {
    //        // En el shadow pass no se hace nada con texturas ni materiales
    //        // podrías incluso usar un SRB diferente si no hay variables
    //        // con BIND_SHADER_RESOURCE en el VS
    //    }


    //    // Ajustar vertex/index buffers:
    //    IBuffer* pVB[]    = {m_VertexBuffer};
    //    Uint64   offset[] = {0};
    //    pContext->SetVertexBuffers(0, 1, pVB, offset, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    //    pContext->SetIndexBuffer(m_IndexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    //    DrawIndexedAttribs drawAttrs;
    //    drawAttrs.IndexType  = VT_UINT32;
    //    drawAttrs.NumIndices = m_NumIndices;
    //    drawAttrs.Flags      = DRAW_FLAG_VERIFY_ALL;
    //    pContext->DrawIndexed(drawAttrs);
    //}

    void setMaterial(POMMaterial* material){
        m_Material = material;        
    }

    void commitMaterial(IShaderResourceBinding* srb, IDeviceContext* ctx) {
        m_Material->Bind(srb);
        m_Material->Upload(ctx);
    
    }

    POMMaterial* getMaterial() const {
		return m_Material;
	}

    virtual void Actualizar(float deltaTime) = 0; // Movimiento/animación
    void         SetTransform(float4x4 transform) { m_Transform = transform; }
    void         rotar(float3 rotacion)
    {
    }
    float4x4 GetTransform() const { return m_Transform; }
};


} // namespace Diligent