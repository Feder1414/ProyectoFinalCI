#pragma once

#include "FigureBase.h"

namespace Diligent
{
class Cubo : public Objeto3D
{
public:
    Cubo(RefCntAutoPtr<IRenderDevice> device, RefCntAutoPtr<IPipelineState> m_pPSO, std::uint32_t id);

    //void Render(RefCntAutoPtr<IDeviceContext>  pContext, RefCntAutoPtr<IPipelineState> m_PSO, float4x4 viewProjection) override;
    void Actualizar(float deltaTime) override;
    void crearBufferVertices() override;
    void crearBufferIndices() override;
    void crearBufferConstante() override;
};
} // namespace Diligent