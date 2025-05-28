/*
 *  Copyright 2019-2022 Diligent Graphics LLC
 *  Copyright 2015-2019 Egor Yusov
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  In no event and under no legal theory, whether in tort (including negligence),
 *  contract, or otherwise, unless required by applicable law (such as deliberate
 *  and grossly negligent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental,
 *  or consequential damages of any character arising as a result of this License or
 *  out of the use or inability to use the software (including but not limited to damages
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and
 *  all other commercial damages or losses), even if such Contributor has been advised
 *  of the possibility of such damages.
 */

#pragma once

#include "SampleBase.hpp"
#include "BasicMath.hpp"
#include "FirstPersonCamera.hpp"
#include "Cubo.h"
#include "POMMaterial.h"
#include "ShadowMap.h"
#include "FigureBase.h"
#include <vector>
#include "ShadowMapManager.hpp"
#include "BasicStructures.fxh"
#include "Utilities/interface/DiligentFXShaderSourceStreamFactory.hpp"
#include "GLTFLoader.hpp"
//#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "DungeonGenerator.h"
#include "DungeonScene.h"
#include "TiledMap.h"
#include "TiledScene.h"





namespace Diligent
{
struct ConstantsData
{
    float4x4 g_World;
    float4x4 g_ViewProj;
    float3 g_CameraPos;
    //float4x4 g_LightViewProj;
};

struct LightConstants
{
    float4x4 g_ViewProjLight;
    float3   g_LightPos;

};

struct parallaxConstants{

    int  parallaxMode; // 0 = parallax simple, 1 = parallax steeping, 2 = parallax occlusion
    float _pPad;
    float _pPd1;
    float generalHeightScale;

};

struct materialConstants
{
    int materialId;
    float _mPad;
    float _mPd1;
    float _mPd2;

};

class Tutorial03_Texturing final : public SampleBase
{
public:
    virtual void Initialize(const SampleInitInfo& InitInfo) override final;

    virtual void Render() override final;
    virtual void RenderizarObjetos() final;

    virtual void RenderShadowPass();
    virtual void Update(double CurrTime, double ElapsedTime) override final;

    virtual void WindowResize(Uint32 Width, Uint32 Height) override final;


    virtual const Char* GetSampleName() const override final { return "Tutorial03: Texturing"; }

private:
    void RenderizarObjeto(Objeto3D* objeto, bool isShadowPass, float4x4 cascadeProj = float4x4::Translation(float3(0.0f,0.0f,0.0f)));
    void RenderizarObjeto(GLTF::Model* objeto, bool isShadowPass, float4x4 cascadeproj = float4x4::Translation(float3(0.0f, 0.0f, 0.0f)), float4x4 worldMatrix = float4x4::Translation(float3(0.0f, 0.0f, 0.0f)));
    void RenderizarDungeon();
    void CreatePipelineState();
    void CreateVertexBuffer();
    void CreateIndexBuffer();
    void LoadTexture();
    void PrintFloat4x4(const float4x4& m, const std::string id);
    void createShadowMapManager();
    void UpdateUI();
    void InitializeScene();
    void LoadMaterials();
    void CreatePipelineStateGLTF();
    void InitializeTileScene();
    void RenderizarTileScene(bool isShadowPass, float4x4 cascadeProj = float4x4::Translation(float3(0.0f, 0.0f, 0.0f)));
    void ReConstruirTileScene(std::string mapaEscena = "mapaMazmorra.json");

    // helper cómodo
    void SelectMaterial(const std::string& key, POMMaterial*& dst)
    {
        auto it = m_POMCatalog.find(key);
        if (it != m_POMCatalog.end())
            dst = it->second;
    }



    RefCntAutoPtr<IPipelineState>         m_pPSO;
    RefCntAutoPtr<IPipelineState>         m_pPSOGLTF;
    RefCntAutoPtr<IBuffer>                m_VSConstants;
    RefCntAutoPtr<ITextureView>           m_TextureSRV;
    RefCntAutoPtr<IShaderResourceBinding> m_SRB;
    RefCntAutoPtr<IShaderResourceBinding> m_SRBGLTF;


    POMMaterial* m_pFloorMat = nullptr; // textura usada cuando MaterialId == 0
    POMMaterial* m_pWallMat  = nullptr; // textura usada cuando MaterialId == 1
    int          m_FloorSel  = 0;       // índices para los combos de ImGui
    int          m_WallSel   = 0;



    float4x4                              m_WorldViewProjMatrix;
    RefCntAutoPtr<IBuffer>                m_BufferConstantsObjects;
    RefCntAutoPtr<IBuffer>                m_BufferLightConstants;
    FirstPersonCamera                     m_Camera;
    FirstPersonCamera					 m_LightCamera;
    std::unique_ptr<Cubo>         m_Cubo;
    std::unique_ptr<Cubo>         m_Piso;
    std::unique_ptr<GLTF::Model>         m_Barril;
    std::unique_ptr<GLTF::Model>         m_Cage;
    std::unique_ptr<GLTF::Model>         m_Skull;
    std::unique_ptr<GLTF::Model>         m_Chest;
    std::unique_ptr<GLTF::Model>         m_Cauldron;
    std::unordered_map<std::string, POMMaterial*> m_POMCatalog; 
    std::vector<std::string>                      m_POMNames;  


    std::unique_ptr<POMMaterial>          m_Brick; // en tu .hpp
    std::unique_ptr<POMMaterial>          m_Brick2; // en tu .hpp
    std::unique_ptr<POMMaterial>          m_Rock; // en tu .hpp
    std::unique_ptr<POMMaterial>          m_RockPath; // en tu .hpp
    std::unique_ptr<POMMaterial>          m_Rocks2; // en tu .hpp
    std::unique_ptr<POMMaterial>          m_LeatherPadded; // en tu .hpp
    std::unique_ptr<POMMaterial>          m_DungeonStone; // en tu .hpp
    std::unique_ptr<POMMaterial>          m_DungeonFloor; // en tu .hpp
    std::unique_ptr<POMMaterial>          m_glossyMarble; // en tu .hpp
    std::unique_ptr<ShadowMap>  m_ShadowMap; 
    std::unique_ptr<GLTF::Model>          m_Model;
    std::vector<std::unique_ptr<Objeto3D>> m_ModelListGLTF;
    std::vector<std::unique_ptr<Objeto3D>> m_ModelList;
   

    DungeonGenerator m_DungeonGenerator;
    DungeonScene m_DungeonScene;

    TiledMap m_TiledMap;
    TileScene m_TiledScene;

    std::unordered_map<std::string, std::unique_ptr<GLTF::Model>> m_modelsGLTF;




    struct FloorMesh
    {
        RefCntAutoPtr<IBuffer> VertexBuffer;
        RefCntAutoPtr<IBuffer> IndexBuffer;
        uint32_t NumIndices;


    };

    FloorMesh m_FloorMesh; // piso de la escena

    //ShadowMapManager m_ShadowMapManager;

    
    struct ShadowSettings
    {
        bool           SnapCascades         = true;
        bool           StabilizeExtents     = true;
        bool           EqualizeExtents      = true;
        bool           SearchBestCascade    = true;
        bool           FilterAcrossCascades = true;
        int            Resolution           = 1024;
        float          PartitioningFactor   = 0.95f;
        TEXTURE_FORMAT Format               = TEX_FORMAT_D16_UNORM;
        int            iShadowMode          = SHADOW_MODE_PCF;

        bool Is32BitFilterableFmt = true;
    } m_ShadowSettings;


    
    std::vector<std::unique_ptr<Objeto3D>> m_Objetos3D;


    //ShadowSettings   m_ShadowSettings = {};
    LightAttribs     m_LightAttribs   = {};
    parallaxConstants m_ParallaxAttribs = {};
    ShadowMapManager m_ShadowMapMgr;

    RefCntAutoPtr<IBuffer>  m_CameraAttribsCB; // VS   (matrices view/proj)
    RefCntAutoPtr<IBuffer>  m_LightAttribsCB;  // VS+PS (dirección luz, cascadas)
    RefCntAutoPtr<IBuffer>  m_ParallaxAttribsCB; // VS+PS (dirección luz, cascadas)
    RefCntAutoPtr<IBuffer>  m_MaterialAttribsCB; // VS+PS (dirección luz, cascadas)
    RefCntAutoPtr<ISampler> m_pCmpSampler;     // PCF
    RefCntAutoPtr<ISampler> m_pAnisoSampler;   // VSM / EVSM

    bool                    m_PackMatrixRowMajor = true;


         



};

} // namespace Diligent
