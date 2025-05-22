#pragma once


#include "FigureBase.h"
#include "GLTFLoader.hpp"
namespace Diligent
{
class FigureGLTF : public Objeto3D
{
public:
    void crearBufferVertices() override{
        return ;
    
    };

    void crearBufferIndices() override{
		return ;
	
	};

    void crearBufferConstante() override{
		return ;
	
	};

    GLTF::Model* getModelGLTF() {
        return m_Model.get();

    }

    void cargarModelo(const char* path, IDeviceContext* m_pImmediateContext)
	{
		GLTF::ModelCreateInfo CI;
		CI.FileName = path;

		static constexpr GLTF::VertexAttributeDesc MyVertexAttrs[] =
            {
                // Nombre        , BufferId, Tipo, Componentes
                {GLTF::PositionAttributeName, 0, VT_FLOAT32, 3},
                {GLTF::NormalAttributeName, 0, VT_FLOAT32, 3},
                {GLTF::Texcoord0AttributeName, 0, VT_FLOAT32, 2},
                {GLTF::TangentAttributeName, 0, VT_FLOAT32, 4} // ojo: tangente es float4 (xyz + w)
            };


        static constexpr GLTF::TextureAttributeDesc MyTexAttrs[] =
            {
                {GLTF::BaseColorTextureName, GLTF::DefaultBaseColorTextureAttribId},
                {GLTF::NormalTextureName, GLTF::DefaultNormalTextureAttribId},
                // …otros que quieras…
                {"heightTexture", /*elige un índice único, p.e.*/ 5}};

        CI.VertexAttributes    = MyVertexAttrs;
        CI.NumVertexAttributes = _countof(MyVertexAttrs);

        CI.TextureAttributes    = MyTexAttrs;
        CI.NumTextureAttributes = _countof(MyTexAttrs);

        m_Model = std::make_unique<GLTF::Model>(m_pDevice, m_pImmediateContext, CI);

        m_Model->PrepareGPUResources(m_pDevice, m_pImmediateContext);


        int normSlot = m_Model->GetTextureAttributeIndex(GLTF::NormalTextureName);
        for (auto& mat : m_Model->Materials)
        {
            bool active = mat.IsTextureAttribActive(normSlot);
            OutputDebugStringA(("Normal slot: " + std::to_string(normSlot) + " isActive: " + (active ? "true\n" : "false\n")).c_str());
        }

         //GLTF::Model::ImageData img {};

        //if (LoadHeightMap("MyHeight.png", img))
        //{
        //    // Devuelve un índice en Model.Textures[]
        //    int heightTexId = m_Model->AddTexture(m_pDevice,
        //                                         /*pTextureCache*/ nullptr,
        //                                         /*pResourceMgr*/ nullptr,
        //                                         img,
        //                                         /*GltfSamplerId*/ 0,
        //                                         "HeightMap");

        //    // Asigna ese texture ID al slot 5 de cada material
        //    int slot = m_Model->GetTextureAttributeIndex("heightTexture");
        //    for (auto& M : m_Model->Materials)
        //        M.SetTextureId(slot, heightTexId);

        //    // Ya está en GPU; libera la copia CPU
        //    stbi_image_free((void*)img.pData);
        //}
        //else
        //{
        //    OutputDebugStringA("Error: no pudimos cargar MyHeight.png\n");
        //    // Error: no pudimos cargar MyHeight.png
        //}




	}

private:
    std::unique_ptr<GLTF::Model> m_Model;





};


}; // namespace Diligent